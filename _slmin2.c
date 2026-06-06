#include <stdio.h>
#include <string.h>
#include <time.h>
#include "src/boolnet/boolnet.h"
#include "src/bool_router/bool_router.h"
#include "src/mem_int/mem_int_layer.h"
#include "src/tsetlin_train/tsetlin_train.h"
static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}
int main(void) {
    printf("BUILD...\n"); fflush(stdout);
    BoolNet *net = boolnet_create(1, 16, 4);
    uint8_t zb[16]={0};
    BoolRouter *r1 = bool_router_create(1, 128, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1, bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, 128, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1, mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);
    BoolRouter *r2 = bool_router_create(3, 128, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2, bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, 16, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2, mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);
    printf("BUILD OK layers=%u\n", net->num_layers); fflush(stdout);

    printf("TRAIN 200 steps...\n"); fflush(stdout);
    srand((unsigned)time(NULL));
    TsetlinTrainer *t = tsetlin_create(net, 5000, 16*255);
    int train_pairs[5][2] = {{0,1},{4,4},{11,11},{12,12},{13,13}};
    for (int s = 0; s < 200; s++) {
        int p = s % 5;
        uint8_t in[16]={0}, tg[16]={0};
        in[train_pairs[p][0]] = 255;
        tg[train_pairs[p][1]] = 255;
        tsetlin_train_step(t, in, tg);
    }
    printf("TRAIN DONE steps=%u accepted=%u\n", t->step_count, t->accept_count); fflush(stdout);

    printf("EVAL...\n"); fflush(stdout);
    int correct = 0;
    for (int p = 0; p < 5; p++) {
        uint8_t in[16]={0}, out[16]={0};
        in[train_pairs[p][0]] = 255;
        reset_mem(net);
        boolnet_forward(net, in, out);
        int max_pos = 0;
        for (int i = 1; i < 16; i++) if (out[i] > out[max_pos]) max_pos = i;
        int ok = (max_pos == train_pairs[p][1]);
        printf("  %d->%d: pred=%d %s\n", train_pairs[p][0], train_pairs[p][1], max_pos, ok?"OK":"??");
        correct += ok;
    }
    printf("ACC: %d/5\n", correct); fflush(stdout);
    tsetlin_destroy(t); boolnet_destroy(net);
    return (correct >= 3) ? 0 : 1;
}
