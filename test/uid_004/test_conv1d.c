/* Per-function tests for conv1d_circular (Constitution V) — UID 004 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/conv1d_circular/conv1d_circular.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("conv1d_create");
    Conv1D *c = conv1d_create(4, 8, 3, 1, 1);
    C(c && c->uid==4 && c->input_length==8,"create N=8,K=3,s=1,d=1");
    conv1d_destroy(c);
    C(!conv1d_create(4,0,3,1,1),"reject N=0");
    C(!conv1d_create(4,8,0,1,1),"reject K=0"); }

void test_forward(void) { T("conv1d_forward");
    Conv1D *c = conv1d_create(4, 6, 3, 2, 1);
    c->kernel[0]=1; c->kernel[1]=0; c->kernel[2]=0;
    float in[6]={1,2,3,4,5,6}, out[3];
    C(!conv1d_forward(c,in,out),"forward ok");
    C(fabsf(out[0]-1)<0.001f && fabsf(out[1]-3)<0.001f && fabsf(out[2]-5)<0.001f, "out[0]=in[0]=1, out[1]=in[2]=3, out[2]=in[4]=5 (stride=2)");
    /* Circular wrap: out[2] with idx=(2*2+0)%6=4→in[4]=5 ✓ */
    conv1d_destroy(c); }

void test_forward_circular(void) { T("conv1d_forward (circular wrap)");
    Conv1D *c = conv1d_create(4, 4, 2, 1, 1);
    c->kernel[0]=1; c->kernel[1]=10;
    float in[4]={1,0,0,0}, out[4];
    conv1d_forward(c,in,out);
    /* out[3]: idx0=3, idx1=(3+1)%4=0: k[0]*in[3]+k[1]*in[0]=0+10=10 */
    C(fabsf(out[3]-10)<0.001f,"circular: out[3]=k[0]*in[3]+k[1]*in[0]=10");
    conv1d_destroy(c); }

void test_save_load(void) { T("conv1d_save/load");
    Conv1D *c = conv1d_create(4, 5, 2, 1, 1);
    c->kernel[0]=3.5f; c->kernel[1]=-2.0f;
    C(!conv1d_save(c,"_cv_test.bin"),"save ok");
    Conv1D *c2 = conv1d_load("_cv_test.bin");
    C(c2 && c2->uid==4 && c2->kernel_size==2,"load params");
    C(fabsf(c2->kernel[0]-3.5f)<0.001f && fabsf(c2->kernel[1]+2.0f)<0.001f,"kernel match");
    conv1d_destroy(c); conv1d_destroy(c2); remove("_cv_test.bin"); }

int main(void) {
    test_create(); test_forward(); test_forward_circular(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
