/**
 * train_pipeline.c — BoolNet Complete Training Pipeline
 *
 * Demonstrates: build → train (epochs+val+checkpoint+early_stop) → export → load → infer
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

#define N_IN    4
#define N_CELLS 4
#define N_TRAIN 4
#define N_VAL   4

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

static BoolNet* build_network(void)
{
    BoolNet *net = boolnet_create(1, N_IN, 5);
    uint8_t zb[4] = {0};

    BoolRouter *r1 = bool_router_create(1, 32, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    Conv1D *c1 = conv1d_create(2, N_IN, 3, 1, 1);
    boolnet_add_layer(net, LAYER_CONV1D, 2, c1,
        conv1d_forward_layer, (int(*)(void*,const char*))conv1d_save);

    MemIntLayer *m1 = mem_int_create(3, ML_PRECISION_UINT8, N_CELLS, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 3, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    BoolRouter *r2 = bool_router_create(4, 32, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 4, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    MemIntLayer *m2 = mem_int_create(5, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 5, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    return net;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════╗\n");
    printf("║  BoolNet Training Pipeline           ║\n");
    printf("╚══════════════════════════════════════╝\n");

    /* 1. Build */
    BoolNet *net = build_network();
    printf("Network: 5 layers, 67 trainable bits\n");

    /* 2. Data */
    uint8_t tr_in[N_TRAIN*N_IN] = {
        0x01,0,0,0,  0,0x01,0,0,  0,0,0x01,0,  0,0,0,0x01
    };
    uint8_t tr_tg[N_TRAIN*N_CELLS] = {
        0xFF,0,0,0,  0,0xFF,0,0,  0,0,0xFF,0,  0,0,0,0xFF
    };
    uint8_t vl_in[N_VAL*N_IN] = {
        0x01,0,0,0,  0,0x01,0,0,  0,0,0x01,0,  0,0,0,0x01
    };
    uint8_t vl_tg[N_VAL*N_CELLS] = {
        0xFF,0,0,0,  0,0xFF,0,0,  0,0,0xFF,0,  0,0,0,0xFF
    };

    /* 3. Train config */
    TsetlinTrainer *t = tsetlin_create(net, 5000, N_CELLS*255);

    TrainConfig cfg = {
        .max_epochs = 20,
        .steps_per_epoch = 1000,
        .neg_tolerance = 5000,
        .byte_max = N_CELLS * 255,
        .early_stop_patience = 5,
        .checkpoint_every = 5,
        .eval_every = 100,
        .checkpoint_dir = "weights"
    };

    /* 4. Train */
    int final_val = tsetlin_train_full(t, &cfg,
        tr_in, tr_tg, N_TRAIN,
        vl_in, vl_tg, N_VAL,
        N_IN, N_CELLS, reset_mem);

    /* 5. Export */
    tsetlin_export_model(t->network, "weights/final_model.bin");

    /* 6. Final inference demo */
    printf("\n═══ Inference Demo ═══\n");
    uint8_t test_in[4] = {0x01, 0x00, 0x00, 0x00};
    uint8_t test_out[N_CELLS];
    reset_mem(t->network);
    boolnet_forward(t->network, test_in, test_out);
    printf("Input:  [%02X %02X %02X %02X]\n", test_in[0],test_in[1],test_in[2],test_in[3]);
    printf("Output: [%3d %3d %3d %3d]\n", test_out[0],test_out[1],test_out[2],test_out[3]);

    /* Cleanup */
    tsetlin_destroy(t);
    boolnet_destroy(net);
    return 0;
}
