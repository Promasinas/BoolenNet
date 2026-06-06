#include <stdio.h>
#include <string.h>
#include "src/boolnet/boolnet.h"
#include "src/bool_router/bool_router.h"
#include "src/mem_int/mem_int_layer.h"
#include "src/tsetlin_train/tsetlin_train.h"
int main(void) {
    printf("1\n"); fflush(stdout);
    BoolNet *net = boolnet_create(1, 16, 4);
    printf("2 net=%p layers=%u\n", (void*)net, net->num_layers); fflush(stdout);
    uint8_t zb[16] = {0};
    BoolRouter *r1 = bool_router_create(1, 128, zb);
    printf("3 r1=%p\n", (void*)r1); fflush(stdout);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1, bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    printf("4\n"); fflush(stdout);
    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, 128, 255, 1);
    printf("5 m1=%p\n", (void*)m1); fflush(stdout);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1, mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);
    printf("6\n"); fflush(stdout);
    BoolRouter *r2 = bool_router_create(3, 128, zb);
    printf("7\n"); fflush(stdout);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2, bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    printf("8\n"); fflush(stdout);
    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, 16, 255, 0);
    printf("9\n"); fflush(stdout);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2, mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);
    printf("10 BUILD OK\n"); fflush(stdout);
    uint8_t in[16]={255,0}, out[16]={0};
    int rc = boolnet_forward(net, in, out);
    printf("11 forward rc=%d out[0]=%d\n", rc, out[0]); fflush(stdout);
    boolnet_destroy(net); printf("DONE\n");
    return 0;
}
