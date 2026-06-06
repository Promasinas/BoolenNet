/**
 * net128_train.c — BoolNet 128-Wide Network Training
 *
 * Architecture (all layers 128-wide):
 *   Input(16B=128b) → Router(128b) → Conv1D(N=16,K=5) → Memory(128c,decay=1)
 *                    → Router(128b) → Memory(128c,output,cell values)
 *
 * 128 boolean routers per layer = 128 bits = 16 bytes per router.
 * Trainable: Router1(128b) + Conv1D(5b) + Router2(128b) = 261 bits
 *
 * Task: Learn to route 4 distinct 16-byte input patterns → 4 output classes.
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

#define WIDTH     16    /* bytes = 128 bits per layer */
#define N_CELLS   8     /* memory cells (fewer than 128 for classification) */
#define N_PATS    4
#define EPOCHS    30
#define STEPS_EP  2000

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

static BoolNet* build_128_network(void)
{
    BoolNet *net = boolnet_create(1, WIDTH, 5);
    uint8_t zb[WIDTH];
    memset(zb, 0, WIDTH);

    /* Layer 1: Router (128 bits = 16 bytes) */
    BoolRouter *r1 = bool_router_create(1, WIDTH*8, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 2: Conv1D (boolean kernel, 5 neighbors, circular) */
    Conv1D *c1 = conv1d_create(2, WIDTH, 5, 1, 1);
    boolnet_add_layer(net, LAYER_CONV1D, 2, c1,
        conv1d_forward_layer, (int(*)(void*,const char*))conv1d_save);

    /* Layer 3: Memory (128 cells, uint8, decay=1 — hidden nonlinearity) */
    MemIntLayer *m1 = mem_int_create(3, ML_PRECISION_UINT8, WIDTH, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 3, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    /* Layer 4: Router (128 bits) */
    BoolRouter *r2 = bool_router_create(4, WIDTH*8, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 4, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 5: Output Memory (decay=0, cell values for classification output) */
    MemIntLayer *m2 = mem_int_create(5, ML_PRECISION_UINT8, WIDTH, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 5, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    return net;
}

/* Generate one-hot input patterns */
static void make_onehot(uint8_t *buf, int pos, int len)
{
    memset(buf, 0, len);
    buf[pos % len] = 0xFF;  /* full byte signal at position */
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  BoolNet 128-Wide Network Training          ║\n");
    printf("║  Each layer: 128 boolean routers (16 bytes) ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    /* ---- Build ---- */
    clock_t t0 = clock();
    BoolNet *net = build_128_network();
    printf("Build time: %.0f ms\n", 1000.0*(clock()-t0)/CLOCKS_PER_SEC);
    printf("Layers: Router(128b)→Conv1D(16,K=5)→Mem(128c)→Router(128b)→Mem(128c)\n");
    printf("Trainable: 128+5+128 = 261 bits\n");
    printf("Memory cells: %u + %u = %u (%.1f KB)\n\n",
           WIDTH, WIDTH, WIDTH*2, WIDTH*2/1024.0);

    /* ---- Data ---- */
    uint8_t tr_in[N_PATS*WIDTH], tr_tg[N_PATS*WIDTH];
    for (int p = 0; p < N_PATS; p++) {
        make_onehot(&tr_in[p*WIDTH], p*4, WIDTH);      /* spread patterns */
        make_onehot(&tr_tg[p*WIDTH], p*4+2, WIDTH);    /* shifted targets */
    }

    printf("Data: %d patterns, %d bytes each\n", N_PATS, WIDTH);
    printf("  Pattern 0: byte[0]=FF      → target byte[2]=FF\n");
    printf("  Pattern 1: byte[4]=FF      → target byte[6]=FF\n");
    printf("  Pattern 2: byte[8]=FF      → target byte[10]=FF\n");
    printf("  Pattern 3: byte[12]=FF     → target byte[14]=FF\n\n");

    /* ---- Train ---- */
    TsetlinTrainer *t = tsetlin_create(net, 30000, WIDTH * 255);

    TrainConfig cfg = {
        .max_epochs = EPOCHS,
        .steps_per_epoch = STEPS_EP,
        .neg_tolerance = 30000,
        .byte_max = WIDTH * 255,
        .early_stop_patience = 8,
        .checkpoint_every = 5,
        .eval_every = 100,
        .checkpoint_dir = "weights"
    };

    clock_t t1 = clock();
    tsetlin_train_full(t, &cfg, tr_in, tr_tg, N_PATS,
                       tr_in, tr_tg, N_PATS,  /* val = train for demo */
                       WIDTH, WIDTH, reset_mem);

    double train_sec = (double)(clock() - t1) / CLOCKS_PER_SEC;
    printf("\nTraining time: %.1f sec (%.0f steps/sec)\n",
           train_sec, t->step_count / (train_sec + 0.001));

    /* ---- Export ---- */
    tsetlin_export_model(t->network, "weights/net128_model.bin");

    /* ---- Inference ---- */
    printf("\n═══ Inference ═══\n");
    for (int p = 0; p < N_PATS; p++) {
        uint8_t in[WIDTH], out[WIDTH];
        make_onehot(in, p*4, WIDTH);
        reset_mem(t->network);
        boolnet_forward(t->network, in, out);

        /* Find which output byte has max value */
        int max_pos = 0;
        for (int i = 1; i < WIDTH; i++)
            if (out[i] > out[max_pos]) max_pos = i;

        int expected = (p*4+2) % WIDTH;
        printf("  Pattern %d: byte[%d]=FF → max_out=byte[%d]=%3d  %s\n",
               p, p*4, max_pos, out[max_pos],
               max_pos == expected ? "✅" : "❌");
    }

    uint32_t st, ac; int32_t br; uint32_t ep; int32_t bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);

    /* Show router stats */
    BoolRouter *r1 = NULL, *r2 = NULL;
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER) {
            if (!r1) r1 = (BoolRouter*)net->layers[i].instance;
            else     r2 = (BoolRouter*)net->layers[i].instance;
        }
    }

    /* Count non-zero bits in routers (how many learned flips) */
    int r1_flips = 0, r2_flips = 0;
    if (r1) for (uint32_t i = 0; i < r1->num_bits; i++)
        if ((r1->bits[i/8] >> (i&7)) & 1) r1_flips++;
    if (r2) for (uint32_t i = 0; i < r2->num_bits; i++)
        if ((r2->bits[i/8] >> (i&7)) & 1) r2_flips++;

    printf("\n═══ Stats ═══\n");
    printf("  Steps: %u  Accepted: %u (%.1f%%)  Epochs: %u\n",
           st, ac, st?100.0f*ac/st:0, ep);
    printf("  Router1 flipped bits: %d/128  Router2: %d/128  Conv1D: 5b\n",
           r1_flips, r2_flips);
    printf("  Model: net128_model.bin\n");

    tsetlin_destroy(t); boolnet_destroy(net);
    return 0;
}
