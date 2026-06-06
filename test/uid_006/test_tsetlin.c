/* Test suite for tsetlin_train (Tsetlin Automaton Training) — uid_006 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/tsetlin_train/tsetlin_train.h"
#include "../../src/boolnet/boolnet.h"

static int p = 0, f = 0;
#define T(n) printf("[TEST] Running %s...\n", n)
#define C(c, m, i, e, a) do { if(c){printf("[PASS] %s\n",m);p++;} \
  else{printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n",m,i,e,a);f++;} } while(0)

int main(void) {
    /* US1: Create trainer */
    T("Create Tsetlin trainer");
    BoolNet *net = boolnet_create(99, 8, 4);
    boolnet_add_layer(net, LAYER_ROUTER, 1, NULL, NULL, NULL);
    TsetlinTrainer *t = tsetlin_create(net, 5, 255);
    C(t != NULL, "create valid trainer", "net,neg_tol=5,bmax=255", "non-NULL", "NULL");
    C(t->neg_tolerance==5 && t->byte_max==255, "params stored", "5,255", "ok", "mismatch");
    C(tsetlin_create(NULL, 1, 255) == NULL, "reject NULL network", "NULL net", "NULL", "non-NULL");

    /* US2: Train step on empty network (should handle gracefully) */
    T("Train step");
    float target[8] = {1,2,3,4,5,6,7,8};
    int rc = tsetlin_train_step(t, target);
    /* With after/before both uninitialized, the reward computation still runs */
    C(rc >= -2, "train_step completes (rc=%d)", "", ">= -2", "error");

    /* US3: Stats */
    T("Get training stats");
    uint32_t steps, accepted;
    float best;
    tsetlin_get_stats(t, &steps, &accepted, &best);
    C(steps > 0, "step count > 0", "", ">0", "0");
    C(best <= 255.0f, "best reward ≤ 255", "", "≤255", "other");

    /* US4: Save/Load */
    T("Save trainer state");
    int sv = tsetlin_save(t, "tsetlin_test.bin");
    C(sv == 0, "save returns 0", "", "0", "other");

    T("Load trainer state");
    TsetlinTrainer *t2 = tsetlin_load("tsetlin_test.bin");
    C(t2 != NULL, "load returns non-NULL", "", "non-NULL", "NULL");
    if (t2) {
        C(t2->neg_tolerance==t->neg_tolerance && t2->byte_max==t->byte_max, "params match", "", "ok", "mismatch");
        C(t2->step_count==t->step_count, "step count preserved", "", "ok", "mismatch");
        tsetlin_destroy(t2);
    }
    remove("tsetlin_test.bin");

    tsetlin_destroy(t);
    boolnet_destroy(net);
    printf("\n=== TEST SUMMARY ===\nTotal: %d | Passed: %d | Failed: %d\n", p+f, p, f);
    return (f > 0);
}
