/* conv1d_circular per-function tests — UID 004 (uint8_t API) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/conv1d_circular/conv1d_circular.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("conv1d_create");
    Conv1D *c = conv1d_create(4, 8, 3, 1, 1);
    C(c && c->uid==4 && c->input_length==8, "create N=8,K=3");
    conv1d_destroy(c);
    C(!conv1d_create(4,0,3,1,1),"reject N=0");
    C(!conv1d_create(4,8,0,1,1),"reject K=0"); }

void test_forward(void) { T("conv1d_forward (uint8_t)");
    Conv1D *c = conv1d_create(4, 6, 3, 2, 1);
    c->kernel[0]=1; c->kernel[1]=0; c->kernel[2]=0;
    uint8_t in[6]={10,20,30,40,50,60}, out[3]={0};
    C(!conv1d_forward(c,in,out),"forward ok");
    C(out[0]==10 && out[1]==30 && out[2]==50,"stride=2: in[0]=10,in[2]=30,in[4]=50");
    conv1d_destroy(c); }

void test_forward_circular(void) { T("conv1d_forward (circular wrap)");
    Conv1D *c = conv1d_create(4, 4, 2, 1, 1);
    c->kernel[0]=1; c->kernel[1]=10;
    uint8_t in[4]={1,0,0,0}, out[4]={0};
    conv1d_forward(c,in,out);
    /* out[3]: idx0=3, idx1=(3+1)%4=0: 1*0+10*1=10 */
    C(out[3]==10,"circular: k0*in[3]+k1*in[0]=10");
    conv1d_destroy(c); }

void test_save_load(void) { T("conv1d_save/load");
    Conv1D *c = conv1d_create(4, 5, 2, 1, 1);
    c->kernel[0]=3; c->kernel[1]=255;
    C(!conv1d_save(c,"_cv.bin"),"save ok");
    Conv1D *c2 = conv1d_load("_cv.bin");
    C(c2 && c2->uid==4 && c2->kernel_size==2,"load params match");
    C(c2->kernel[0]==3 && c2->kernel[1]==255,"kernel preserved");
    conv1d_destroy(c); conv1d_destroy(c2); remove("_cv.bin"); }

int main(void) {
    test_create(); test_forward(); test_forward_circular(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
