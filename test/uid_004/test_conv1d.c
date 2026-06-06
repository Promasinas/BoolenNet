/* conv1d_circular per-function tests — UID 004 (Pure Boolean kernel) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/conv1d_circular/conv1d_circular.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("conv1d_create (Boolean kernel)");
    Conv1D *c = conv1d_create(4, 8, 3, 1, 1);
    C(c && c->uid==4 && c->input_length==8 && c->kernel_size==3, "create N=8,K=3");
    C(c->kernel_bits[0]&1, "default: kernel bit0=1 (identity-like)");
    conv1d_destroy(c);
    C(!conv1d_create(4,0,3,1,1),"reject N=0"); }

void test_forward(void) { T("conv1d_forward (Boolean sum)");
    /* Default kernel: bit0=1 only → copies input[i*stride] */
    Conv1D *c = conv1d_create(4, 6, 3, 2, 1);
    uint8_t in[6]={10,20,30,40,50,60}, out[3]={0};
    conv1d_forward(c,in,out);
    C(out[0]==10 && out[1]==30 && out[2]==50,"bit0 only: out[i]=in[i*stride]");
    conv1d_destroy(c); }

void test_forward_multi_bit(void) { T("conv1d_forward (multi-bit kernel)");
    /* Set kernel bits 0 and 1 → sum two neighbors */
    Conv1D *c = conv1d_create(4, 4, 3, 1, 1);
    c->kernel_bits[0] = 0x03; /* bits 0 and 1 set */
    uint8_t in[4]={1,2,3,4}, out[4]={0};
    conv1d_forward(c,in,out);
    /* out[0]=in[0]+in[1]=3, out[1]=in[1]+in[2]=5 */
    C(out[0]==3 && out[1]==5,"kernel bits 0+1: sum of 2 neighbors");
    /* out[3]: idx0=3, idx1=(3+1)%4=0 → 4+1=5 */
    C(out[3]==5,"circular: in[3]+in[0]=5");
    conv1d_destroy(c); }

void test_flip_kernel_bit(void) { T("conv1d_flip_kernel_bit (Tsetlin)");
    Conv1D *c = conv1d_create(4, 4, 2, 1, 1);
    c->kernel_bits[0] = 0x00; /* clear all */
    C(!conv1d_flip_kernel_bit(c,0),"flip bit 0 ok");
    C(c->kernel_bits[0]&1,"bit0=1 after flip");
    C(!conv1d_flip_kernel_bit(c,0),"flip back");
    C(!(c->kernel_bits[0]&1),"bit0=0");
    C(conv1d_flip_kernel_bit(c,99)==-1,"reject OOB");
    conv1d_destroy(c); }

void test_save_load(void) { T("conv1d_save/load");
    Conv1D *c = conv1d_create(4, 5, 8, 1, 1);
    c->kernel_bits[0] = 0xAA; /* alternating pattern */
    C(!conv1d_save(c,"_cv.bin"),"save ok");
    Conv1D *c2 = conv1d_load("_cv.bin");
    C(c2 && c2->uid==4 && c2->kernel_bits[0]==0xAA,"load: kernel preserved");
    conv1d_destroy(c); conv1d_destroy(c2); remove("_cv.bin"); }

int main(void) {
    test_create(); test_forward(); test_forward_multi_bit(); test_flip_kernel_bit(); test_save_load();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
