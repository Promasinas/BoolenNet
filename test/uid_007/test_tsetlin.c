/* tsetlin_train per-function tests — UID 007 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/tsetlin_train/tsetlin_train.h"
#include "../../src/boolnet/boolnet.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("tsetlin_create");
    BoolNet *n = boolnet_create(99, 8, 4);
    boolnet_add_layer(n, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(n, 5, 255);
    C(t && t->neg_tolerance==5 && t->byte_max==255, "create tol=5,bmax=255");
    C(!tsetlin_create(NULL,1,255), "reject NULL net");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_get_stats(void) { T("tsetlin_get_stats");
    BoolNet *n = boolnet_create(99, 8, 4);
    TsetlinTrainer *t = tsetlin_create(n, 5, 255);
    uint32_t s=0,a=0; int32_t b=0;
    tsetlin_get_stats(t, &s, &a, &b);
    C(s==0 && a==0 && b==-2147483648, "stats initialized");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_save_load(void) { T("tsetlin_save/load");
    BoolNet *n = boolnet_create(99, 4, 4);
    TsetlinTrainer *t = tsetlin_create(n, 7, 128);
    C(!tsetlin_save(t, "_tt.bin"), "save ok");
    TsetlinTrainer *t2 = tsetlin_load("_tt.bin");
    C(t2 && t2->neg_tolerance==7 && t2->byte_max==128, "load params match");
    tsetlin_destroy(t); tsetlin_destroy(t2); boolnet_destroy(n); remove("_tt.bin"); }

int main(void) {
    test_create(); test_get_stats(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
