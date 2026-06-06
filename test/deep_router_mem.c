/**
 * deep_router_mem.c — Deep Router→Memory Cascade
 *
 * Key insight: Router XORs compose linearly. Insert Memory (nonlinear threshold)
 * between routers to create true deep cascade.
 *
 * Architecture (128 alternating pairs = 256 layers):
 *   Input → [Router₀→Mem₀] → [Router₁→Mem₁] → ... → [Router₁₂₇→Mem₁₂₇(output)]
 *
 * Each Memory layer breaks XOR linearity via trigger threshold (sig>0→255, sig=0→decay).
 * Trainable: 128 routers × 128 bits = 16384 bits
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_PAIRS  8     /* number of Router→Memory pairs */
#define N_BYTES  4     /* 32 bits = 4 bytes (uniform dimension) */
#define N_CELLS  N_BYTES  /* same as byte count for clean cascade */

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Deep Router→Memory Cascade             ║\n");
    printf("║  %d Router→Memory pairs (%d layers)    ║\n", N_PAIRS, N_PAIRS*2);
    printf("║  Trainable: %d router bits             ║\n", N_PAIRS * N_BYTES * 8);
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Build: N_PAIRS × (Router → Memory) */
    BoolNet *net = boolnet_create(1, N_BYTES, N_PAIRS * 2);
    uint8_t zb[N_BYTES]; memset(zb, 0, N_BYTES);

    for (int i = 0; i < N_PAIRS; i++) {
        /* Router: 128 bits, processes all 16 bytes */
        BoolRouter *r = bool_router_create((LayerUID)(i*2 + 1), N_BYTES * 8, zb);
        boolnet_add_layer(net, LAYER_ROUTER, i*2 + 1, r,
            bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

        /* Memory: nonlinear threshold between routers (decay=1 for hidden, 0 for output) */
        int is_output = (i == N_PAIRS - 1);
        MemIntLayer *m = mem_int_create(i*2 + 2, ML_PRECISION_UINT8, N_CELLS,
                                        255, is_output ? 0 : 1);
        boolnet_add_layer(net, LAYER_MEMORY, i*2 + 2, m,
            is_output ? mem_int_forward_output : mem_int_forward_layer,
            (int(*)(void*,const char*))mem_int_save);
    }

    printf("Layers: %u  |  Routers: %d × 128b = %d trainable bits\n",
           net->num_layers, N_PAIRS, N_PAIRS * 128);

    /* Data */
    #define N_PATS 4
    uint8_t tr_in[N_PATS * N_BYTES], tr_tg[N_PATS * N_CELLS];
    memset(tr_in, 0, sizeof(tr_in)); memset(tr_tg, 0, sizeof(tr_tg));
    for (int p = 0; p < N_PATS; p++) {
        tr_in[p * N_BYTES + p * 4] = 0xFF;
        tr_tg[p * N_CELLS + p]     = 0xFF;
    }

    printf("Patterns: %d  |  Output cells: %d\n\n", N_PATS, N_CELLS);

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 200000, N_CELLS * 255);
    TrainConfig cfg = { 100, 2000, 200000, N_CELLS*255, 30, 20, 50, "." };

    clock_t t1 = clock();
    tsetlin_train_full(t, &cfg, tr_in, tr_tg, N_PATS,
                       tr_in, tr_tg, N_PATS, N_BYTES, N_CELLS, reset_mem);
    double sec = (double)(clock() - t1) / CLOCKS_PER_SEC;

    /* Eval */
    printf("\n═══ Final ═══\n");
    int ok = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[N_CELLS];
        reset_mem(net);
        boolnet_forward(net, &tr_in[p*N_BYTES], out);
        int mx = 0; for (int i=1;i<N_CELLS;i++) if(out[i]>out[mx])mx=i;
        if (mx == p) ok++;
        printf("  P%d: max=cell[%d]=%3d  %s\n", p, mx, out[mx], mx==p?"✅":"❌");
    }

    uint32_t st,ac; int32_t br; uint32_t ep; int32_t bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);

    int flips = 0;
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            for (uint32_t b = 0; b < r->num_bits; b++)
                if ((r->bits[b/8] >> (b&7)) & 1) flips++;
        }

    printf("\nSteps:%u  Ok:%u  Acc:%d/%d  Flips:%d/%d  Time:%.1fs\n",
           st, ac, ok, N_PATS, flips, N_PAIRS*128, sec);

    tsetlin_export_model(net, "deep_rm_model.bin");
    tsetlin_destroy(t); boolnet_destroy(net);
    return 0;
}
