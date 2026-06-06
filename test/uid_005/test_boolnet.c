/* Test suite for boolnet (Forward Function & Network Topology) — uid_005 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/boolnet/boolnet.h"

static int p = 0, f = 0;
#define T(n) printf("[TEST] Running %s...\n", n)
#define C(c, m, i, e, a) do { if(c){printf("[PASS] %s\n",m);p++;} \
  else{printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n",m,i,e,a);f++;} } while(0)

int main(void) {
    /* US1: Create network */
    T("Create BoolNet (input_dim=8, max_layers=4)");
    BoolNet *net = boolnet_create(5, 8, 4);
    C(net != NULL, "create valid network", "dim=8,max=4", "non-NULL", "NULL");
    C(net->uid==5 && net->input_dim==8, "params stored", "uid=5,dim=8", "ok", "mismatch");

    /* US2: Forward with 0 layers (identity) */
    T("Forward with 0 layers (identity)");
    float input[8] = {1,2,3,4,5,6,7,8};
    float out[8] = {0};
    int rc = boolnet_forward(net, input, out);
    C(rc == 0, "forward returns 0", "0 layers", "0", "other");
    int ident = 1;
    for(int i=0;i<8;i++) if(fabsf(out[i]-input[i])>0.001f) ident=0;
    C(ident, "0 layers = identity pass-through", "", "same as input", "different");

    /* US3: Add layers and verify UID ordering */
    T("Add layers (sorted by UID)");
    rc = boolnet_add_layer(net, LAYER_ROUTER, 10, NULL, NULL, NULL);
    C(rc == 0, "add layer uid=10", "uid=10", "0", "other");
    rc = boolnet_add_layer(net, LAYER_CONV1D, 3, NULL, NULL, NULL);
    C(rc == 0, "add layer uid=3 (sorted before 10)", "uid=3", "0", "other");
    C(net->num_layers==2, "2 layers registered", "", "2", "other");
    C(net->layers[0].uid==3, "first layer uid=3 (sorted)", "", "3", "other");
    C(net->layers[1].uid==10, "second layer uid=10", "", "10", "other");

    /* US4: Save/Load */
    T("Save network to file");
    int sv = boolnet_save(net, "boolnet_test.bin");
    C(sv == 0, "save returns 0", "", "0", "other");

    T("Load network from file");
    BoolNet *loaded = boolnet_load("boolnet_test.bin");
    C(loaded != NULL, "load returns non-NULL", "", "non-NULL", "NULL");
    if (loaded) {
        C(loaded->uid==net->uid && loaded->num_layers==net->num_layers, "params match", "", "ok", "mismatch");
        boolnet_destroy(loaded);
    }
    remove("boolnet_test.bin");

    /* Edge: NULL param handling */
    T("Edge: NULL param handling");
    C(boolnet_forward(NULL, input, out) == -1, "forward(NULL) returns -1", "", "-1", "other");

    boolnet_destroy(net);
    printf("\n=== TEST SUMMARY ===\nTotal: %d | Passed: %d | Failed: %d\n", p+f, p, f);
    return (f > 0);
}
