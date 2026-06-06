/**
 * coord_descent_demo.c — Tsetlin vs Coordinate Descent comparison
 *
 * Same task: 1 input → 1 output byte mapping, Router(32b)→Memory(4c)
 * Compare training speed and final reward.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"
#include "../src/tsetlin_train/coord_descent.h"

#define N 4
#define BYTE_MAX (N * 255)

static void reset_mem(BoolNet *net) {
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)net->layers[i].instance);
}

static BoolNet* build_net(void) {
    BoolNet *net = boolnet_create(1, N, 2);
    uint8_t zb[4] = {0};
    BoolRouter *r = bool_router_create(1, 32, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    MemIntLayer *m = mem_int_create(2, ML_PRECISION_UINT8, N, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);
    return net;
}

static int32_t reward(BoolNet *net, const uint8_t *in, const uint8_t *tg) {
    uint8_t out[N]; reset_mem(net); boolnet_forward(net, in, out);
    int32_t r = BYTE_MAX;
    for (int i = 0; i < N; i++) { int d = (int)out[i] - (int)tg[i]; r -= (d < 0 ? -d : d); }
    return r;
}

int main(void) {
    srand((unsigned)time(NULL));
    uint8_t in[N]  = {0x01, 0x01, 0x01, 0x01};
    uint8_t tg[N]  = {0xFF, 0x00, 0x00, 0x00};

    printf("=== Tsetlin vs Coordinate Descent ===\n");
    printf("Task: Router(32b)->Memory(4c), map [01 01 01 01] -> [FF 00 00 00]\n\n");

    /* ---- Tsetlin ---- */
    {
        BoolNet *net = build_net();
        printf("Tsetlin initial reward: %d\n", reward(net, in, tg));
        TsetlinTrainer *t = tsetlin_create(net, 5000, BYTE_MAX);
        clock_t t0 = clock();
        for (int s = 0; s < 5000; s++) {
            int rc = tsetlin_train_step(t, in, tg);
            if (rc == -2) break;
        }
        double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
        printf("Tsetlin 5000 steps: %.0f ms, reward=%d\n", sec * 1000, reward(net, in, tg));
        tsetlin_destroy(t); boolnet_destroy(net);
    }

    /* ---- Coordinate Descent ---- */
    {
        BoolNet *net = build_net();
        printf("\nCoordDescent initial reward: %d\n", reward(net, in, tg));
        clock_t t0 = clock();
        int sweeps = coord_descent_train(net, in, tg, BYTE_MAX, 5);
        double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
        printf("CoordDescent %d sweeps: %.0f ms, reward=%d\n",
               sweeps, sec * 1000, reward(net, in, tg));
        boolnet_destroy(net);
    }

    return 0;
}
