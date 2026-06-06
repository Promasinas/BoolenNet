/**
 * cascade_128.c — 128 Single-Bit Boolean Router Cascade
 *
 * Each router is ONE BIT (decision gate). 128 routers form a deep cascade
 * that progressively narrows the address space toward the correct memory slot.
 *
 * Architecture per conversation:
 *   Byte Input → Conv1D → [1-bit Router₀ → 1-bit Router₁ → ... → 1-bit Router₁₂₇]
 *                       → Memory Layers → Output
 *
 * Trainable: 128 bits + 3 conv kernel bits = 131 bits total
 * Each router is a single-bit XOR: output[i] = input[i] ^ bit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/conv1d_circular/conv1d_circular.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BITS   128   /* cascade depth: 128 single-bit routers */
#define N_BYTES  (N_BITS/8)  /* 16 bytes */
#define N_CELLS  8     /* output classification cells */

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════╗\n");
    printf("║  128 Single-Bit Router Cascade      ║\n");
    printf("║  131 trainable bits total           ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    /* Build */
    clock_t t0 = clock();
    int total_layers = 1 + N_BITS + 2; /* conv + 128 routers + 2 memory */
    BoolNet *net = boolnet_create(1, N_BYTES, total_layers);

    /* Conv1D: local feature extraction */
    Conv1D *c1 = conv1d_create(1, N_BYTES, 3, 1, 1);
    boolnet_add_layer(net, LAYER_CONV1D, 1, c1,
        conv1d_forward_layer, (int(*)(void*,const char*))conv1d_save);

    /* 128 single-bit routers (1 bit = ceil(1/8)=1 byte allocation each) */
    uint8_t zb[1] = {0};
    for (int i = 0; i < N_BITS; i++) {
        BoolRouter *r = bool_router_create((LayerUID)(i + 2), 1, zb);
        boolnet_add_layer(net, LAYER_ROUTER, (LayerUID)(i + 2), r,
            bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    }

    /* Hidden memory: nonlinearity at cascade output */
    MemIntLayer *m1 = mem_int_create(N_BITS + 2, ML_PRECISION_UINT8, N_CELLS, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, N_BITS + 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    /* Output memory: cell values for classification */
    MemIntLayer *m2 = mem_int_create(N_BITS + 3, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, N_BITS + 3, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    printf("Layers: %u  |  Trainable: %d bits  |  Build: %.0f ms\n\n",
           net->num_layers, N_BITS + 3, 1000.0*(clock()-t0)/CLOCKS_PER_SEC);

    /* Data: 4 patterns → 4 output cells */
    #define N_PATS 4
    uint8_t tr_in[N_PATS * N_BYTES], tr_tg[N_PATS * N_CELLS];
    memset(tr_in, 0, sizeof(tr_in)); memset(tr_tg, 0, sizeof(tr_tg));
    for (int p = 0; p < N_PATS; p++) {
        tr_in[p * N_BYTES + p * 4] = 0xFF;
        tr_tg[p * N_CELLS + p]     = 0xFF;
    }

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 50000, N_CELLS * 255);
    TrainConfig cfg = { 30, 1000, 50000, N_CELLS*255, 10, 10, 50, "." };

    tsetlin_train_full(t, &cfg, tr_in, tr_tg, N_PATS,
                       tr_in, tr_tg, N_PATS, N_BYTES, N_CELLS, reset_mem);

    /* Eval */
    printf("\n═══ Inference ═══\n");
    int ok = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[N_CELLS];
        reset_mem(net);
        boolnet_forward(net, &tr_in[p*N_BYTES], out);
        int mx = 0; for (int i=1;i<N_CELLS;i++) if(out[i]>out[mx])mx=i;
        if (mx == p) ok++;
        printf("  P%d: max=cell[%d]=%3d  %s\n", p, mx, out[mx], mx==p?"✅":"❌");
    }

    /* Count flipped bits */
    int flips = 0;
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            if (r->bits[0]) flips++;
        }

    uint32_t st,ac; int32_t br; uint32_t ep; int32_t bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);
    printf("\nSteps:%u  Ok:%u (%.0f%%)  Acc:%d/%d  Flips:%d/128\n",
           st, ac, st?100.0f*ac/st:0, ok, N_PATS, flips);

    tsetlin_export_model(net, "cascade128_model.bin");
    tsetlin_destroy(t); boolnet_destroy(net);
    return 0;
}
