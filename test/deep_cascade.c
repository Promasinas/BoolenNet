/**
 * deep_cascade.c — BoolNet Deep Cascade Network (128 Boolean Routers)
 *
 * Architecture from conversation design:
 *   Byte Stream → 1D Conv → [128 Boolean Router layers in cascade]
 *                          → Memory Slots (output)
 *
 * Each layer is a single BoolRouter that routes the signal progressively
 * toward the correct memory slot. The cascade narrows the address space
 * with each Boolean decision.
 *
 * Depth: 128 Router layers + 1 Conv1D + 2 Memory = 131 layers total
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

#define N_ROUTERS 128   /* cascade depth: 128 boolean router layers */
#define N_BYTES   16    /* 128 bits = 16 bytes per layer */
#define N_CELLS   16    /* memory cells */

/* Build deep cascade: Conv1D + 128x Router + 2x Memory */
static BoolNet* build_deep_cascade(void)
{
    BoolNet *net = boolnet_create(1, N_BYTES, N_ROUTERS + 3);
    uint8_t zb[N_BYTES]; memset(zb, 0, N_BYTES);

    /* Conv1D: input feature extraction */
    Conv1D *c1 = conv1d_create(1, N_BYTES, 3, 1, 1);
    boolnet_add_layer(net, LAYER_CONV1D, 1, c1,
        conv1d_forward_layer, (int(*)(void*,const char*))conv1d_save);

    /* 128 Router layers in cascade (UID 2..129) */
    for (int i = 0; i < N_ROUTERS; i++) {
        BoolRouter *r = bool_router_create((LayerUID)(i + 2), N_BYTES * 8, zb);
        boolnet_add_layer(net, LAYER_ROUTER, (LayerUID)(i + 2), r,
            bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    }

    /* Hidden Memory (nonlinearity at cascade midpoint) — UID 130 */
    MemIntLayer *m1 = mem_int_create(N_ROUTERS + 2, ML_PRECISION_UINT8, N_CELLS, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, N_ROUTERS + 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    /* Output Memory — UID 131 */
    MemIntLayer *m2 = mem_int_create(N_ROUTERS + 3, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, N_ROUTERS + 3, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    return net;
}

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet Deep Cascade Network           ║\n");
    printf("║  128 Boolean Routers in Series          ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Build */
    clock_t t0 = clock();
    BoolNet *net = build_deep_cascade();
    double build_ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;

    printf("Layers: %u total\n", net->num_layers);
    printf("  Layer 1: Conv1D (3-neighbor feature extraction)\n");
    printf("  Layers 2-%d: 128 Boolean Routers (cascade)\n", N_ROUTERS + 1);
    printf("  Layer %u: Hidden Memory (nonlinearity)\n", N_ROUTERS + 2);
    printf("  Layer %u: Output Memory (cell values)\n", N_ROUTERS + 3);
    printf("Trainable: 128×128 + 3 = %d bits\n", N_ROUTERS * 128 + 3);
    printf("Build time: %.0f ms\n\n", build_ms);

    /* Data */
    #define N_PATS 4
    uint8_t tr_in[N_PATS * N_BYTES];
    uint8_t tr_tg[N_PATS * N_CELLS];
    memset(tr_in, 0, sizeof(tr_in));
    memset(tr_tg, 0, sizeof(tr_tg));

    /* 4 distinct patterns, each activates a different output cell */
    for (int p = 0; p < N_PATS; p++) {
        tr_in[p * N_BYTES + p * 4] = 0xFF;      /* byte[p*4]=FF */
        tr_tg[p * N_CELLS + p]     = 0xFF;      /* cell[p]=FF */
    }

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 50000, N_CELLS * 255);

    TrainConfig cfg = {
        .max_epochs = 50,
        .steps_per_epoch = 2000,
        .neg_tolerance = 200000,
        .byte_max = N_CELLS * 255,
        .early_stop_patience = 50,
        .checkpoint_every = 50,
        .eval_every = 100,
        .checkpoint_dir = "."
    };

    printf("Training: %d patterns, %d epochs, %d steps/epoch\n",
           N_PATS, cfg.max_epochs, cfg.steps_per_epoch);
    printf("Trainable params: %d bits\n\n", N_ROUTERS * 128 + 3);

    clock_t t1 = clock();
    tsetlin_train_full(t, &cfg, tr_in, tr_tg, N_PATS,
                       tr_in, tr_tg, N_PATS,
                       N_BYTES, N_CELLS, reset_mem);
    double train_sec = (double)(clock() - t1) / CLOCKS_PER_SEC;

    /* Evaluate */
    printf("\n═══ Inference ═══\n");
    int correct = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[N_CELLS];
        reset_mem(net);
        boolnet_forward(net, &tr_in[p * N_BYTES], out);

        int max_pos = 0;
        for (int i = 1; i < N_CELLS; i++)
            if (out[i] > out[max_pos]) max_pos = i;

        if (max_pos == p) correct++;
        printf("  Pattern %d: max_out=cell[%d]=%3d  %s\n",
               p, max_pos, out[max_pos], max_pos == p ? "✅" : "❌");
    }

    uint32_t st, ac; int32_t br; uint32_t ep; int32_t bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);

    /* Count total flipped bits across all routers */
    int total_flips = 0;
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            for (uint32_t b = 0; b < r->num_bits; b++)
                if ((r->bits[b/8] >> (b&7)) & 1) total_flips++;
        }
    }

    printf("\n═══ Stats ═══\n");
    printf("  Layers: %u  Trainable bits: %d\n", net->num_layers, N_ROUTERS*128+3);
    printf("  Steps: %u  Accepted: %u (%.1f%%)\n", st, ac, st?100.0f*ac/st:0);
    printf("  Accuracy: %d/%d  Best val: %d\n", correct, N_PATS, bv);
    printf("  Total flipped bits: %d / %d (%.2f%%)\n",
           total_flips, N_ROUTERS*128, 100.0f*total_flips/(N_ROUTERS*128));
    printf("  Training time: %.1f sec (%.0f steps/sec)\n",
           train_sec, st/(train_sec+0.001));

    tsetlin_export_model(net, "deep128_model.bin");

    tsetlin_destroy(t); boolnet_destroy(net);
    return 0;
}
