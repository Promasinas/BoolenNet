/**
 * optimized_cascade.c — Optimized Deep Router→Memory Cascade
 *
 * Key optimizations from history design:
 *   1. ALL Memory layers use cell VALUES (not binary query) — preserves info
 *   2. Uniform 4-cell dimension throughout cascade
 *   3. Each Router=32bits, each Memory=4cells
 *   4. Deep cascade: N_PAIRS Router→Memory pairs
 *
 * Architecture: Input → [R₀→M₀] → [R₁→M₁] → ... → [Rₙ→Mₙ] → Output
 * Each pair: Router(XOR) breaks linearity via Memory(cell values as next signal)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

/* Network config */
#define N_PAIRS  16    /* Router→Memory pairs (scale to 128) */
#define N_BYTES  4     /* 32 bits per router */
#define N_CELLS  N_BYTES

/* Training config */
#define N_PATS   4
#define EPOCHS   100
#define STEPS_EP 2000

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

static BoolNet* build_optimized_cascade(void)
{
    BoolNet *net = boolnet_create(1, N_BYTES, N_PAIRS * 2);
    uint8_t zb[N_BYTES]; memset(zb, 0, N_BYTES);

    for (int i = 0; i < N_PAIRS; i++) {
        /* Router: N_BYTES*8 bits = 32 bits per router */
        BoolRouter *r = bool_router_create((LayerUID)(i*2 + 1), N_BYTES * 8, zb);
        boolnet_add_layer(net, LAYER_ROUTER, i*2 + 1, r,
            bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

        /* Memory: ALL layers use cell VALUES (not binary query)
         * This preserves 0-255 value information through the cascade.
         * decay=0 for output, decay=1 for hidden (subtle nonlinearity) */
        int is_last = (i == N_PAIRS - 1);
        MemIntLayer *m = mem_int_create(i*2 + 2, ML_PRECISION_UINT8, N_CELLS,
                                        255, is_last ? 0 : 1);
        boolnet_add_layer(net, LAYER_MEMORY, i*2 + 2, m,
            mem_int_forward_output,  /* *** ALL layers use cell values *** */
            (int(*)(void*,const char*))mem_int_save);
    }

    return net;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Optimized Router→Memory Cascade        ║\n");
    printf("║  %d pairs (%d layers)                   ║\n", N_PAIRS, N_PAIRS*2);
    printf("║  All Memory: cell VALUES (0-255)        ║\n");
    printf("║  Trainable: %d bits                     ║\n", N_PAIRS * N_BYTES * 8);
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Build */
    clock_t t0 = clock();
    BoolNet *net = build_optimized_cascade();
    printf("Build: %u layers, %.0f ms\n", net->num_layers,
           1000.0*(clock()-t0)/CLOCKS_PER_SEC);

    /* Data: one-hot patterns → shifted one-hot targets */
    uint8_t tr_in[N_PATS * N_BYTES], tr_tg[N_PATS * N_CELLS];
    memset(tr_in, 0, sizeof(tr_in)); memset(tr_tg, 0, sizeof(tr_tg));
    for (int p = 0; p < N_PATS; p++) {
        tr_in[p * N_BYTES + p] = 0xFF;
        tr_tg[p * N_CELLS + ((p+1) % N_PATS)] = 0xFF;  /* shifted target */
    }
    printf("Data: %d patterns (%d bytes each), targets shifted by 1\n\n", N_PATS, N_BYTES);

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 100000, N_CELLS * 255);
    TrainConfig cfg = { EPOCHS, STEPS_EP, 100000, N_CELLS*255, 25, 25, 100, "weights" };

    clock_t t1 = clock();
    tsetlin_train_full(t, &cfg, tr_in, tr_tg, N_PATS,
                       tr_in, tr_tg, N_PATS, N_BYTES, N_CELLS, reset_mem);
    double sec = (double)(clock() - t1) / CLOCKS_PER_SEC;

    /* Evaluate */
    printf("\n═══ Final ═══\n");
    int ok = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[N_CELLS];
        reset_mem(net);
        boolnet_forward(net, &tr_in[p*N_BYTES], out);

        /* Find max cell */
        int mx = 0;
        for (int i = 1; i < N_CELLS; i++)
            if (out[i] > out[mx]) mx = i;

        int expected = (p+1) % N_PATS;
        ok += (mx == expected);
        printf("  P%d: max=cell[%d]=%3d (want cell[%d])  %s\n",
               p, mx, out[mx], expected, mx==expected?"✅":"❌");
    }

    /* Stats */
    uint32_t st, ac; int32_t br; uint32_t ep; int32_t bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);

    int flips = 0;
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            for (uint32_t b = 0; b < r->num_bits; b++)
                if ((r->bits[b/8] >> (b&7)) & 1) flips++;
        }

    printf("\n═══ Stats ═══\n");
    printf("  Layers: %u  Params: %d bits  Flips: %d\n",
           net->num_layers, N_PAIRS*N_BYTES*8, flips);
    printf("  Steps: %u  Accepted: %u (%.1f%%)  BestVal: %d\n",
           st, ac, st?100.0f*ac/st:0, bv);
    printf("  Accuracy: %d/%d  Time: %.1fs (%.0f step/s)\n",
           ok, N_PATS, sec, st/(sec+0.001));

    /* Show per-pair flipped bits */
    printf("\n  Per-pair router flips:\n");
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            int pf = 0;
            for (uint32_t b = 0; b < r->num_bits; b++)
                if ((r->bits[b/8] >> (b&7)) & 1) pf++;
            printf("  R%02u: %2d/32", i/2, pf);
            if ((i/2+1) % 4 == 0) printf("\n");
        }
    }
    printf("\n");

    tsetlin_export_model(net, "weights/optimized_cascade.bin");
    tsetlin_destroy(t); boolnet_destroy(net);
    return (ok == N_PATS) ? 0 : 1;
}
