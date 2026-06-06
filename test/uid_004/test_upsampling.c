/* Test suite for upsampling (Multi-Router Upsampling) — uid_004 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/upsampling/upsampling.h"

static int p = 0, f = 0;
#define T(n) printf("[TEST] Running %s...\n", n)
#define C(c, m, i, e, a) do { if(c){printf("[PASS] %s\n",m);p++;} \
  else{printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n",m,i,e,a);f++;} } while(0)

int main(void) {
    /* US1: Create */
    T("Create upsampling (M=2, N=4)");
    UpsampleLayer *u = upsample_create(4, 2, 4);
    C(u != NULL, "create valid layer", "M=2,N=4", "non-NULL", "NULL");
    C(u->uid==4 && u->num_routers==2, "params stored", "uid=4,M=2", "ok", "mismatch");
    C(upsample_create(5, 0, 4) == NULL, "reject M=0", "M=0", "NULL", "non-NULL");
    C(upsample_create(5, 2, 0) == NULL, "reject N=0", "N=0", "NULL", "non-NULL");

    /* US2: Set router bits + Forward */
    T("Set router and forward");
    uint8_t r0[4] = {0, 1, 0, 0};  /* flip element 1 only */
    uint8_t r1[4] = {1, 0, 0, 1};  /* flip elements 0 and 3 */
    upsample_set_router(u, 0, r0);
    upsample_set_router(u, 1, r1);
    float input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float output[8];
    upsample_forward(u, input, output);
    /* Router 0: [1, -2, 3, 4]  (flip idx 1) */
    /* Router 1: [-1, 2, 3, -4] (flip idx 0,3) */
    C(fabsf(output[0]-1.0f)<0.001f, "r0[0]=1 (pass)", "", "1.0", "other");
    C(fabsf(output[1]+2.0f)<0.001f, "r0[1]=-2 (flip)", "", "-2.0", "other");
    C(fabsf(output[4]+1.0f)<0.001f, "r1[0]=-1 (flip)", "", "-1.0", "other");
    C(fabsf(output[7]+4.0f)<0.001f, "r1[3]=-4 (flip)", "", "-4.0", "other");
    C(fabsf(output[6]-3.0f)<0.001f, "r1[2]=3 (pass)", "", "3.0", "other");

    /* US3: Cascade */
    T("Cascade two layers");
    UpsampleLayer *u2 = upsample_create(99, 2, 8); /* takes 8 inputs (M*N from u1) */
    uint8_t r2[8] = {1,1,1,1,1,1,1,1}; /* flip all */
    upsample_set_router(u2, 0, r2);
    upsample_set_router(u2, 1, r2);
    float cascade_out[16];
    UpsampleLayer *layers[2] = {u, u2};
    int rc = upsample_cascade(layers, 2, input, cascade_out);
    C(rc == 0, "cascade returns 0", "2 layers", "0", "other");
    /* All should be negated: output of u1 negated by u2 routers */
    /* r0: [1, -2, 3, 4] → u2 flips all → [-1, 2, -3, -4, -1, -2, -3, -4, 1, 2, 3, 4, 1, 2, 3, 4] */
    C(fabsf(cascade_out[0]+1.0f)<0.001f, "cascade: r0[0]=-1 (double flip)", "", "-1.0", "other");
    C(fabsf(cascade_out[1]-2.0f)<0.001f, "cascade: r0[1]=2 (un-flipped)", "", "2.0", "other");

    upsample_destroy(u); upsample_destroy(u2);
    printf("\n=== TEST SUMMARY ===\nTotal: %d | Passed: %d | Failed: %d\n", p+f, p, f);
    return (f > 0);
}
