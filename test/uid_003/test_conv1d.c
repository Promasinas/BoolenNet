/* Test suite for conv1d_circular (1D Circular Convolution) — uid_003 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/conv1d_circular/conv1d_circular.h"

static int p = 0, f = 0;
#define T(n) printf("[TEST] Running %s...\n", n)
#define C(c, m, i, e, a) do { if(c){printf("[PASS] %s\n",m);p++;} \
  else{printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n",m,i,e,a);f++;} } while(0)

int main(void) {
    /* US1: Create */
    T("Create conv1d (N=8, K=3, stride=2, dilation=1)");
    Conv1D *c = conv1d_create(3, 8, 3, 2, 1);
    C(c != NULL, "create valid conv1d", "N=8,K=3,s=2,d=1", "non-NULL", "NULL");
    C(c->uid==3 && c->input_length==8, "params stored", "uid=3,N=8", "ok", "mismatch");

    T("Reject invalid params");
    C(conv1d_create(4, 0, 3, 1, 1) == NULL, "reject N=0", "N=0", "NULL", "non-NULL");
    C(conv1d_create(4, 8, 0, 1, 1) == NULL, "reject K=0", "K=0", "NULL", "non-NULL");

    /* US2: Forward — circular wrap */
    T("Forward: basic convolution");
    float input[8] = {1,2,3,4,5,6,7,8};
    c->kernel[0] = 1.0f; c->kernel[1] = 0.0f; c->kernel[2] = 0.0f;
    float out[4];
    conv1d_forward(c, input, out);
    C(fabsf(out[0]-1.0f)<0.001f, "out[0]=input[0*2+0]=1", "stride=2", "1.0", "other");
    C(fabsf(out[1]-3.0f)<0.001f, "out[1]=input[2]=3", "", "3.0", "other");
    C(fabsf(out[2]-5.0f)<0.001f, "out[2]=input[4]=5", "", "5.0", "other");
    C(fabsf(out[3]-7.0f)<0.001f, "out[3]=input[6]=7", "", "7.0", "other");

    T("Forward: circular wrap (overflow wraps to start)");
    c->kernel[0] = 0.0f; c->kernel[1] = 1.0f; c->kernel[2] = 0.0f;
    conv1d_forward(c, input, out);
    /* out[0]=k[1]*input[(0*2+1*1)%8]=input[1]=2, out[3]=k[1]*input[(3*2+1)%8]=input[7]=8 */
    C(fabsf(out[0]-2.0f)<0.001f, "out[0]=input[1]=2", "", "2.0", "other");
    /* out[3]: idx=(3*2+1)%8=7 */
    C(fabsf(out[3]-8.0f)<0.001f, "out[3]=input[7]=8", "", "8.0", "other");

    /* US3: Save/Load */
    T("Save to file");
    int sv = conv1d_save(c, "conv1d_test.bin");
    C(sv == 0, "save returns 0", "", "0", "other");

    T("Load from file");
    Conv1D *c2 = conv1d_load("conv1d_test.bin");
    C(c2 != NULL, "load returns non-NULL", "", "non-NULL", "NULL");
    if (c2) {
        C(c2->uid==c->uid && c2->input_length==c->input_length, "params match", "", "ok", "mismatch");
        int kern_ok = 1;
        for (uint32_t i=0;i<c->kernel_size;i++) if(fabsf(c2->kernel[i]-c->kernel[i])>0.001f) kern_ok=0;
        C(kern_ok, "kernel preserved after load", "", "ok", "mismatch");
        conv1d_destroy(c2);
    }
    remove("conv1d_test.bin");

    conv1d_destroy(c);
    printf("\n=== TEST SUMMARY ===\nTotal: %d | Passed: %d | Failed: %d\n", p+f, p, f);
    return (f > 0);
}
