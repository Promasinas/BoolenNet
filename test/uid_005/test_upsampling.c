/* Per-function tests for upsampling (Constitution VI) — UID 005 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/upsampling/upsampling.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("upsample_create");
    UpsampleLayer *u = upsample_create(5, 3, 4);
    C(u && u->uid==5 && u->num_routers==3 && u->input_length==4,"create M=3,N=4");
    upsample_destroy(u);
    C(!upsample_create(5,0,4),"reject M=0");
    C(!upsample_create(5,2,0),"reject N=0"); }

void test_set_router(void) { T("upsample_set_router");
    UpsampleLayer *u = upsample_create(5, 1, 4);
    uint8_t b[4]={1,0,1,0};
    C(!upsample_set_router(u,0,b),"set router 0 ok");
    C(u->routers[0].bits[0]==1 && u->routers[0].bits[2]==1,"bits stored");
    C(upsample_set_router(u,99,b)==-1,"reject OOB idx");
    upsample_destroy(u); }

void test_forward(void) { T("upsample_forward");
    UpsampleLayer *u = upsample_create(5, 2, 2);
    uint8_t r0[2]={0,1}, r1[2]={1,0};
    upsample_set_router(u,0,r0); upsample_set_router(u,1,r1);
    float in[2]={3.0f, 5.0f}, out[4];
    C(!upsample_forward(u,in,out),"forward ok");
    C(fabsf(out[0]-3.0f)<0.001f && fabsf(out[1]+5.0f)<0.001f,"r0: pass[0], flip[1]");
    C(fabsf(out[2]+3.0f)<0.001f && fabsf(out[3]-5.0f)<0.001f,"r1: flip[0], pass[1]");
    upsample_destroy(u); }

void test_cascade(void) { T("upsample_cascade");
    UpsampleLayer *u1 = upsample_create(5, 1, 3);
    uint8_t b1[3]={0,1,0}; upsample_set_router(u1,0,b1);
    UpsampleLayer *u2 = upsample_create(99, 2, 3);
    uint8_t b2[3]={1,1,1}; upsample_set_router(u2,0,b2); upsample_set_router(u2,1,b2);
    float in[3]={1,2,3}, out[6];
    UpsampleLayer *layers[2]={u1,u2};
    C(!upsample_cascade(layers,2,in,out),"cascade 2 layers ok");
    /* u1: [1,-2,3] → u2 flips all → [-1,2,-3, -1,2,-3] */
    C(fabsf(out[0]+1)<0.001f && fabsf(out[1]-2)<0.001f,"cascade double-negation correct");
    upsample_destroy(u1); upsample_destroy(u2); }

int main(void) {
    test_create(); test_set_router(); test_forward(); test_cascade();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
