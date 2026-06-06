#include <stdio.h>
#include <string.h>
#include "src/boolnet/boolnet.h"
#include "src/bool_router/bool_router.h"
#include "src/mem_int/mem_int_layer.h"
#include "src/tsetlin_train/tsetlin_train.h"
int main(void) {
    BoolNet *net = boolnet_create(1, 16, 4);
    uint8_t zb[16]={0};
    boolnet_add_layer(net, LAYER_ROUTER, 1, bool_router_create(1,128,zb), bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    boolnet_add_layer(net, LAYER_MEMORY, 2, mem_int_create(2,ML_PRECISION_UINT8,128,255,1), mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);
    boolnet_add_layer(net, LAYER_ROUTER, 3, bool_router_create(3,128,zb), bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);
    boolnet_add_layer(net, LAYER_MEMORY, 4, mem_int_create(4,ML_PRECISION_UINT8,16,255,0), mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);
    printf("NET OK\n"); fflush(stdout);
    TsetlinTrainer *t = tsetlin_create(net, 5000, 16*255);
    printf("TSETLIN OK t=%p\n", (void*)t); fflush(stdout);
    uint8_t in[16]={255,0}, tg[16]={0,255};
    printf("STEP...\n"); fflush(stdout);
    int rc = tsetlin_train_step(t, in, tg);
    printf("RC=%d steps=%u\n", rc, t->step_count); fflush(stdout);
    tsetlin_destroy(t); boolnet_destroy(net);
    return 0;
}
