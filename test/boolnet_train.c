/**
 * boolnet_train.c — Full BoolNet Network Training & Evaluation
 *
 * Network Architecture:
 *   Router(8bit) → Memory(4cell) → Router(8bit) → Memory(4cell)
 *
 * Task: Learn to classify 8-bit input patterns into 4 output classes.
 *   - Input patterns represent "features" (which bits are set)
 *   - Each memory cell corresponds to a "class"
 *   - Only one output cell should trigger per input pattern
 *
 * Training data: 4 distinct input patterns → 4 one-hot outputs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BYTES     4    /* 32 bits = 4 bytes throughout the network */
#define N_CELLS     4    /* 4 memory cells = 4 output classes */
#define N_PATTERNS  4
#define TRAIN_STEPS 20000

/* ---- Network Builder ---- */
static BoolNet* build_network(void)
{
    BoolNet *net = boolnet_create(1, N_BYTES, 4);

    /* Layer 1: Router (8 bit) — learns input routing */
    uint8_t r1_bits[1] = {0x00};
    BoolRouter *r1 = bool_router_create(1, 8, r1_bits);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 2: Memory (4 cell, uint8, max=255, decay=0) — hidden state */
    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    /* Layer 3: Router (8 bit) — learns output routing */
    uint8_t r2_bits[1] = {0x00};
    BoolRouter *r2 = bool_router_create(3, 8, r2_bits);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 4: Memory (4 cell, uint8, max=255, decay=0) — output */
    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    return net;
}

/* ---- Evaluate accuracy on all patterns ---- */
static int evaluate(BoolNet *net, uint8_t inputs[N_PATTERNS][N_BYTES],
                    uint8_t targets[N_PATTERNS][N_CELLS])
{
    int correct = 0;
    uint8_t out[N_CELLS];

    for (int p = 0; p < N_PATTERNS; p++) {
        /* Reset memory between evaluations */
        for (uint32_t i = 0; i < net->num_layers; i++)
            if (net->layers[i].type == LAYER_MEMORY)
                mem_int_reset((MemIntLayer*)net->layers[i].instance);

        boolnet_forward(net, inputs[p], out);
        if (!memcmp(out, targets[p], N_CELLS))
            correct++;
    }
    return correct;
}

/* ---- Main ---- */
int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════╗\n");
    printf("║  BoolNet Network Training Test       ║\n");
    printf("║  Router→Memory→Router→Memory         ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    /* ---- Training Data ---- */
    /* 4 input patterns (8-bit each), 4 one-hot output targets */
    uint8_t inputs[N_PATTERNS][N_BYTES] = {
        {0x01},  /* bit 0: pattern A */
        {0x02},  /* bit 1: pattern B */
        {0x04},  /* bit 2: pattern C */
        {0x08},  /* bit 3: pattern D */
    };
    uint8_t targets[N_PATTERNS][N_CELLS] = {
        {0x01, 0x00, 0x00, 0x00},  /* A → cell 0 */
        {0x00, 0x01, 0x00, 0x00},  /* B → cell 1 */
        {0x00, 0x00, 0x01, 0x00},  /* C → cell 2 */
        {0x00, 0x00, 0x00, 0x01},  /* D → cell 3 */
    };

    printf("Training Data:\n");
    for (int p = 0; p < N_PATTERNS; p++) {
        printf("  Pattern %d: in=0x%02X → target=[%d %d %d %d]\n",
               p, inputs[p][0],
               targets[p][0], targets[p][1], targets[p][2], targets[p][3]);
    }

    /* ---- Build & Test Initial ---- */
    BoolNet *net = build_network();
    printf("\nNetwork: %u layers (Router→Mem→Router→Mem)\n", net->num_layers);

    int init_ok = evaluate(net, inputs, targets);
    printf("Initial accuracy: %d/%d (%.0f%%)\n\n",
           init_ok, N_PATTERNS, 100.0f*init_ok/N_PATTERNS);

    /* ---- Training ---- */
    TsetlinTrainer *trainer = tsetlin_create(net, 5000, N_CELLS * 255);
    printf("Training %d steps (byte_max=%u)...\n", TRAIN_STEPS, trainer->byte_max);

    int best_acc = init_ok;
    int best_step = 0;

    for (int s = 0; s < TRAIN_STEPS; s++) {
        /* Round-robin through all 4 patterns */
        int p = s % N_PATTERNS;
        int rc = tsetlin_train_step(trainer, inputs[p], targets[p]);

        if (rc == -2) {
            printf("  Step %d: stopped (neg_tol exceeded)\n", s+1);
            break;
        }

        /* Evaluate every 1000 steps */
        if ((s+1) % 1000 == 0) {
            int acc = evaluate(net, inputs, targets);
            uint32_t st, ac; int32_t br;
            tsetlin_get_stats(trainer, &st, &ac, &br);
            printf("  Step %5d: acc=%d/%d  accepted=%u  best_r=%d\n",
                   s+1, acc, N_PATTERNS, ac, br);
            if (acc > best_acc) { best_acc = acc; best_step = s+1; }
        }
    }

    /* ---- Final Evaluation ---- */
    printf("\n═══ Final Results ═══\n");
    int final_acc = evaluate(net, inputs, targets);
    uint8_t out[N_CELLS];

    for (int p = 0; p < N_PATTERNS; p++) {
        for (uint32_t i = 0; i < net->num_layers; i++)
            if (net->layers[i].type == LAYER_MEMORY)
                mem_int_reset((MemIntLayer*)net->layers[i].instance);
        boolnet_forward(net, inputs[p], out);

        int ok = !memcmp(out, targets[p], N_CELLS);
        printf("  %s  in=0x%02X → out=[%d %d %d %d]  target=[%d %d %d %d]\n",
               ok ? "✅" : "❌",
               inputs[p][0],
               out[0], out[1], out[2], out[3],
               targets[p][0], targets[p][1], targets[p][2], targets[p][3]);
    }

    uint32_t steps, accepted; int32_t best_r;
    tsetlin_get_stats(trainer, &steps, &accepted, &best_r);

    /* Get router states */
    BoolRouter *r1 = NULL, *r2 = NULL;
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER) {
            if (!r1) r1 = (BoolRouter*)net->layers[i].instance;
            else     r2 = (BoolRouter*)net->layers[i].instance;
        }
    }

    printf("\n═══ Stats ═══\n");
    printf("  Steps: %u  Accepted: %u (%.1f%%)\n",
           steps, accepted, steps ? 100.0f*accepted/steps : 0);
    printf("  Best reward: %d  Best accuracy: %d/%d (step %d)\n",
           best_r, best_acc, N_PATTERNS, best_step);
    if (r1) printf("  Router1 bits: 0x%02X\n", r1->bits[0]);
    if (r2) printf("  Router2 bits: 0x%02X\n", r2->bits[0]);
    printf("  Final accuracy: %d/%d (%.0f%%)\n",
           final_acc, N_PATTERNS, 100.0f*final_acc/N_PATTERNS);

    /* Cleanup: network destroys owned layers */
    tsetlin_destroy(trainer);
    boolnet_destroy(net);
    /* Router/Memory destroyed by boolnet_destroy via layer instances */

    return (final_acc == N_PATTERNS) ? 0 : 1;
}
