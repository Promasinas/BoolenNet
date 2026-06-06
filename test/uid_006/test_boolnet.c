/* Per-function tests for boolnet (Constitution IX) — UID 006 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/boolnet/boolnet.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("boolnet_create");
    BoolNet *n = boolnet_create(6, 8, 10);
    C(n && n->uid==6 && n->input_dim==8 && n->capacity==10,"create dim=8,max=10");
    boolnet_destroy(n);
    C(!boolnet_create(6,0,10),"reject input_dim=0 (fixed)"); }

void test_add_layer(void) { T("boolnet_add_layer");
    BoolNet *n = boolnet_create(6, 4, 5);
    C(!boolnet_add_layer(n,LAYER_ROUTER,20,NULL,NULL,NULL),"add uid=20");
    C(!boolnet_add_layer(n,LAYER_CONV1D,5,NULL,NULL,NULL),"add uid=5 (sorted before 20)");
    C(n->num_layers==2 && n->layers[0].uid==5 && n->layers[1].uid==20,"UID sorted order");
    boolnet_destroy(n); }

void test_forward_empty(void) { T("boolnet_forward (0 layers = identity)");
    BoolNet *n = boolnet_create(6, 4, 4);
    float in[4]={1,2,3,4}, out[4]={0};
    C(!boolnet_forward(n,in,out),"forward ok");
    int ident=1; for(int i=0;i<4;i++) if(fabsf(in[i]-out[i])>0.001f) ident=0;
    C(ident,"identity pass-through");
    boolnet_destroy(n); }

void test_save_load(void) { T("boolnet_save/load");
    BoolNet *n = boolnet_create(6, 4, 8);
    boolnet_add_layer(n,LAYER_ROUTER,1,NULL,NULL,NULL);
    boolnet_add_layer(n,LAYER_CONV1D,3,NULL,NULL,NULL);
    C(!boolnet_save(n,"_bn_test.bin"),"save ok");
    BoolNet *n2 = boolnet_load("_bn_test.bin");
    C(n2 && n2->uid==6 && n2->num_layers==2,"load: uid+layers match");
    C(n2->layers[0].uid==1 && n2->layers[1].uid==3,"load: layer UIDs preserved");
    boolnet_destroy(n); boolnet_destroy(n2); remove("_bn_test.bin"); }

int main(void) {
    test_create(); test_add_layer(); test_forward_empty(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
