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
    TsetlinTrainer *t = tsetlin_create(n, 5, 255);
    C(t && t->neg_tolerance==5 && t->byte_max==255, "create tol=5,bmax=255");
    C(t->epoch_count==0 && t->best_val_acc==-1, "epoch_count=0, best_val_acc=-1");
    C(!tsetlin_create(NULL,1,255), "reject NULL net");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_get_stats(void) { T("tsetlin_get_stats (new API: +epochs, +best_val)");
    BoolNet *n = boolnet_create(99, 8, 4);
    TsetlinTrainer *t = tsetlin_create(n, 5, 255);
    uint32_t s=0,a=0,ep=0; int32_t br=0,bv=0;
    tsetlin_get_stats(t, &s, &a, &br, &ep, &bv);
    C(s==0 && a==0 && ep==0 && br==-2147483648 && bv==-1, "all stats initialized");
    tsetlin_destroy(t); boolnet_destroy(n); }

void test_save_load(void) { T("tsetlin_save/load");
    BoolNet *n = boolnet_create(99, 4, 4);
    TsetlinTrainer *t = tsetlin_create(n, 7, 128);
    C(!tsetlin_save(t, "_tt.bin"), "save ok");
    TsetlinTrainer *t2 = tsetlin_load("_tt.bin");
    C(t2 && t2->neg_tolerance==7 && t2->byte_max==128, "load params match");
    tsetlin_destroy(t); tsetlin_destroy(t2); boolnet_destroy(n); remove("_tt.bin"); }

void test_train_config(void) { T("TrainConfig struct");
    TrainConfig cfg = {100, 50, 15000, 1020, 10, 5, 10, "_ckpt"};
    C(cfg.max_epochs==100 && cfg.steps_per_epoch==50, "config fields set");
    C(cfg.early_stop_patience==10 && cfg.checkpoint_every==5, "early stop + checkpoint"); }

void test_export_model(void) { T("tsetlin_export_model");
    BoolNet *n = boolnet_create(99, 8, 4);
    C(!tsetlin_export_model(n, "_export.bin"), "export ok");
    C(tsetlin_export_model(NULL, "_x.bin")!=0, "reject NULL net");
    boolnet_destroy(n); remove("_export.bin"); }

int main(void) {
    test_create(); test_get_stats(); test_save_load(); test_train_config(); test_export_model();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
