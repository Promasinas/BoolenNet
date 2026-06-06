/* boolnet per-function tests — UID 006 (uint8_t API) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/boolnet/boolnet.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("boolnet_create");
    BoolNet *n = boolnet_create(6, 8, 10);
    C(n && n->uid==6 && n->input_dim==8,"create dim=8,max=10");
    boolnet_destroy(n);
    C(!boolnet_create(6,0,10),"reject dim=0"); }

void test_add_layer(void) { T("boolnet_add_layer");
    BoolNet *n = boolnet_create(6, 4, 5);
    C(!boolnet_add_layer(n,LAYER_ROUTER,20,NULL,NULL,NULL),"add uid=20");
    C(!boolnet_add_layer(n,LAYER_CONV1D,5,NULL,NULL,NULL),"add uid=5");
    C(n->num_layers==2 && n->layers[0].uid==5,"UID sorted");
    boolnet_destroy(n); }

void test_forward(void) { T("boolnet_forward (0 layers, uint8_t)");
    BoolNet *n = boolnet_create(6, 4, 4);
    uint8_t in[4]={1,2,3,4}, out[4]={0};
    C(!boolnet_forward(n,in,out),"forward ok");
    C(!memcmp(in,out,4),"identity pass-through");
    /* Note: if this fails, check memcpy uses correct size */
    boolnet_destroy(n); }

void test_save_load(void) { T("boolnet_save/load");
    BoolNet *n = boolnet_create(6, 4, 8);
    boolnet_add_layer(n,LAYER_ROUTER,1,NULL,NULL,NULL);
    boolnet_add_layer(n,LAYER_CONV1D,3,NULL,NULL,NULL);
    C(!boolnet_save(n,"_bn.bin"),"save ok");
    BoolNet *n2 = boolnet_load("_bn.bin");
    C(n2 && n2->uid==6 && n2->num_layers==2,"load layers match");
    boolnet_destroy(n); boolnet_destroy(n2); remove("_bn.bin"); }

int main(void) {
    test_create(); test_add_layer(); test_forward(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
