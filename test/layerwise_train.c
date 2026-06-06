/**
 * layerwise_train.c — Layer-wise Cascade Training
 *
 * Strategy from history: each Boolean Filter is a binary classifier.
 * Train layer-by-layer: filter 0 learns to split patterns, then freeze,
 * filter 1 learns on the split subsets, etc.
 *
 * Each filter = 1 Router(32bit) + 1 Memory(4cell, cell values)
 *   Input → Router XOR → Memory(cell values) → decision
 *
 * Task: 4 one-hot inputs → 4 one-hot outputs
 * Layer 0: separate {0,1} from {2,3}
 * Layer 1: separate 0 from 1, separate 2 from 3
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BYTES 4
#define N_CELLS 4

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

/* Train ONE Router+Memory pair on a binary classification task */
static int train_pair(BoolRouter *r, MemIntLayer *m,
                      const uint8_t *in, const uint8_t *labels,
                      int n_samples, int class_a, int class_b)
{
    /* Build a mini-network: Router → Memory */
    BoolNet *pair_net = boolnet_create(99, N_BYTES, 2);
    boolnet_add_layer(pair_net, LAYER_ROUTER, 1, r,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    boolnet_add_layer(pair_net, LAYER_MEMORY, 2, m,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    /* Create binary targets: 0xFF on cell[class_a] if in class_a, cell[class_b] if class_b */
    uint8_t *tg = (uint8_t*)malloc(n_samples * N_CELLS);
    memset(tg, 0, n_samples * N_CELLS);
    for (int i = 0; i < n_samples; i++) {
        if (labels[i] == class_a) tg[i * N_CELLS + class_a] = 0xFF;
        else if (labels[i] == class_b) tg[i * N_CELLS + class_b] = 0xFF;
    }

    TsetlinTrainer *t = tsetlin_create(pair_net, 5000, N_CELLS * 255);
    int best_acc = 0;

    printf("    Training pair (class %d vs %d): ", class_a, class_b);
    for (int ep = 0; ep < 2000; ep++) {
        int idx = rand() % n_samples;
        if (labels[idx] != class_a && labels[idx] != class_b) continue;
        tsetlin_train_step(t, &in[idx * N_BYTES], &tg[idx * N_CELLS]);
    }

    /* Evaluate */
    int ok = 0, total = 0;
    for (int i = 0; i < n_samples; i++) {
        if (labels[i] != class_a && labels[i] != class_b) continue;
        total++;
        uint8_t out[N_CELLS];
        reset_mem(pair_net);
        boolnet_forward(pair_net, &in[i * N_BYTES], out);
        int predicted = (out[class_a] >= out[class_b]) ? class_a : class_b;
        if (predicted == labels[i]) ok++;
    }
    float acc = total ? 100.0f * ok / total : 0;

    uint32_t st, ac; int32_t br;
    tsetlin_get_stats(t, &st, &ac, &br, NULL, NULL);
    printf("%d/%d (%.0f%%)  steps=%u  ok=%u\n", ok, total, acc, st, ac);

    /* Don't destroy the router/memory — they're part of the full network */
    tsetlin_destroy(t);
    boolnet_destroy(pair_net);
    free(tg);

    return (acc >= 100.0f) ? 0 : 1;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Layer-wise Cascade Training            ║\n");
    printf("║  Each filter: binary classifier         ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Data */
    #define N_PATS 8
    uint8_t in[N_PATS * N_BYTES];
    uint8_t labels[N_PATS];
    memset(in, 0, sizeof(in));
    /* Task: each pattern has FF in a specific byte, must route to DIFFERENT cell.
     * Router MUST learn to transform the signal. */
    for (int p = 0; p < N_PATS; p++) {
        in[p * N_BYTES + (p % N_BYTES)] = 0xFF;
        labels[p] = (p + 2) % N_PATS;  /* shifted: pattern p → class (p+2)%8 */
    }
    printf("Data: %d patterns, labels shifted by 2\n\n", N_PATS);

    /* Build full cascade */
    #define N_PAIRS 3  /* 3 pairs = classify 8 patterns (2^3 = 8) */
    BoolNet *full_net = boolnet_create(1, N_BYTES, N_PAIRS * 2);
    uint8_t zb[N_BYTES]; memset(zb, 0, N_BYTES);

    BoolRouter *routers[N_PAIRS];
    MemIntLayer *mems[N_PAIRS];

    for (int i = 0; i < N_PAIRS; i++) {
        routers[i] = bool_router_create((LayerUID)(i*2+1), N_BYTES*8, zb);
        boolnet_add_layer(full_net, LAYER_ROUTER, i*2+1, routers[i],
            bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

        mems[i] = mem_int_create(i*2+2, ML_PRECISION_UINT8, N_CELLS, 255, 0);
        boolnet_add_layer(full_net, LAYER_MEMORY, i*2+2, mems[i],
            mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);
    }

    printf("Full network: %u layers, %d trainable bits\n\n",
           full_net->num_layers, N_PAIRS * N_BYTES * 8);

    /* ---- Layer-wise training ---- */
    printf("=== Layer-wise Training ===\n");

    /* Layer 0: split {0,1,2,3} from {4,5,6,7} */
    train_pair(routers[0], mems[0], in, labels, N_PATS, 0, 4);

    /* Layer 1: split {0,1} from {2,3} and {4,5} from {6,7} */
    train_pair(routers[1], mems[1], in, labels, N_PATS, 0, 2);
    train_pair(routers[2], mems[2], in, labels, N_PATS, 0, 1);

    /* ---- Final evaluation ---- */
    printf("\n═══ Full Network Evaluation ═══\n");
    int ok = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[N_CELLS];
        reset_mem(full_net);
        boolnet_forward(full_net, &in[p*N_BYTES], out);

        int mx = 0;
        for (int i = 1; i < N_CELLS; i++)
            if (out[i] > out[mx]) mx = i;

        printf("  P%d: max=cell[%d]=%3d  %s\n", p, mx, out[mx],
               mx == (p % N_CELLS) ? "✅" : "❌");
        if (mx == (p % N_CELLS)) ok++;
    }
    printf("\nAccuracy: %d/%d\n", ok, N_PATS);

    /* Show router states */
    printf("\nRouter bits:\n");
    for (int i = 0; i < N_PAIRS; i++) {
        printf("  R%d: [%02X %02X %02X %02X]\n", i,
               routers[i]->bits[0], routers[i]->bits[1],
               routers[i]->bits[2], routers[i]->bits[3]);
    }

    tsetlin_export_model(full_net, "weights/layerwise_cascade.bin");
    boolnet_destroy(full_net);
    return 0;
}
