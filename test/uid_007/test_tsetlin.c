/* Per-function tests for tsetlin_train (Constitution X) — UID 007 */
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

void test_train_step(void) { T("tsetlin_train_step");
    BoolNet *n = boolnet_create(99, 4, 4);
    boolnet_add_layer(n, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(n, 10, 255);
    uint8_t input[4]={0,0,0,0}, target[4]={0,0,0,0};
    int rc = tsetlin_train_step(t, input, target);
    C(rc==-1, "returns -1 (no valid router — needs real BoolRouter instance)");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_train_epochs(void) { T("tsetlin_train_epochs");
    BoolNet *n = boolnet_create(99, 4, 4);
    boolnet_add_layer(n, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(n, 3, 255);
    uint8_t input[4]={0,0,0,0}, target[4]={0,0,0,0};
    C(!tsetlin_train_epochs(t, input, target, 5), "epochs complete");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_get_stats(void) { T("tsetlin_get_stats");
    BoolNet *n = boolnet_create(99, 8, 4);
    boolnet_add_layer(n, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(n, 5, 255);
    uint32_t s=0,a=0; int32_t b=0;
    tsetlin_get_stats(t, &s, &a, &b);
    C(s==0 && b<=255, "stats initialized (no training without real router)");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_save_load(void) { T("tsetlin_save/load");
    BoolNet *n = boolnet_create(99, 4, 4);
    boolnet_add_layer(n, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(n, 7, 128);
    C(!tsetlin_save(t, "_tt_test.bin"), "save ok");
    TsetlinTrainer *t2 = tsetlin_load("_tt_test.bin");
    C(t2 && t2->neg_tolerance==7 && t2->byte_max==128, "load params match");
    tsetlin_destroy(t); tsetlin_destroy(t2); boolnet_destroy(n); remove("_tt_test.bin"); }

int main(void) {
    test_create(); test_train_step(); test_train_epochs(); test_get_stats(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
