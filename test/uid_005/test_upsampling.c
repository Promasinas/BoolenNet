/* upsampling per-function tests — UID 005 (uint8_t API) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/upsampling/upsampling.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) { T("upsample_create");
    UpsampleLayer *u = upsample_create(5, 3, 4);
    C(u && u->uid==5 && u->num_routers==3,"create M=3,N=4");
    upsample_destroy(u);
    C(!upsample_create(5,0,4),"reject M=0");
    C(!upsample_create(5,2,0),"reject N=0"); }

void test_set_router(void) { T("upsample_set_router");
    UpsampleLayer *u = upsample_create(5, 1, 4);
    uint8_t b[4]={1,0,1,0};
    C(!upsample_set_router(u,0,b),"set router ok");
    C(u->routers[0].bits[0]==1 && u->routers[0].bits[2]==1,"bits stored");
    C(upsample_set_router(u,99,b)==-1,"reject OOB idx");
    upsample_destroy(u); }

void test_forward(void) { T("upsample_forward (uint8_t: flip=255-x)");
    UpsampleLayer *u = upsample_create(5, 2, 2);
    uint8_t r0[2]={0,1}, r1[2]={1,0};
    upsample_set_router(u,0,r0); upsample_set_router(u,1,r1);
    uint8_t in[2]={3, 5}, out[4]={0};
    C(!upsample_forward(u,in,out),"forward ok");
    C(out[0]==3 && out[1]==250,"r0: pass[0]=3, flip[1]=255-5=250");
    C(out[2]==252 && out[3]==5,"r1: flip[0]=255-3=252, pass[1]=5");
    upsample_destroy(u); }

void test_cascade(void) { T("upsample_cascade");
    UpsampleLayer *u1 = upsample_create(5,1,2);
    uint8_t b1[2]={0,1}; upsample_set_router(u1,0,b1);
    UpsampleLayer *u2 = upsample_create(99,1,2);
    uint8_t b2[2]={1,1}; upsample_set_router(u2,0,b2);
    uint8_t in[2]={10,20}, out[2]={0};
    UpsampleLayer *layers[2]={u1,u2};
    C(!upsample_cascade(layers,2,in,out),"cascade ok");
    /* u1: [10, 255-20=235] → u2 flips both: [245, 20] — wait: 255-10=245, 255-235=20 */
    C(out[0]==245 && out[1]==20,"cascade: 10→235→245→? double flip back to 20");
    upsample_destroy(u1); upsample_destroy(u2); }

int main(void) {
    test_create(); test_set_router(); test_forward(); test_cascade();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
