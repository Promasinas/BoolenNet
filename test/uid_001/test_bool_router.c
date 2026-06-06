/* Per-function tests for bool_router (Constitution IV) — UID 001 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/bool_router/bool_router.h"

static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) {
    T("bool_router_create");
    BoolRouter *r = bool_router_create(1, 16, NULL);
    C(r && r->uid==1 && r->num_bits==16, "create 16-bit zero router");
    bool_router_destroy(r);
    C(!bool_router_create(2,0,NULL), "reject num_bits=0");
}

void test_forward(void) {
    T("bool_router_forward");
    uint8_t bits[2]={0x0F,0x00}; /* flip low 4 bits */
    BoolRouter *r = bool_router_create(1, 16, bits);
    uint8_t in[2]={0xFF,0xFF}, out[2]={0};
    bool_router_forward(r,in,out);
    C(out[0]==0xF0 && out[1]==0xFF, "XOR: 0xFF^0x0F=0xF0, high byte unchanged");
    /* edge: trailing bit mask for non-byte-aligned */
    BoolRouter *r2 = bool_router_create(2, 3, NULL);
    uint8_t in2[1]={0x07}, out2[1]={0};
    bool_router_forward(r2,in2,out2);
    C(out2[0]==0x07, "3-bit identity (no flip bits set)");
    bool_router_destroy(r); bool_router_destroy(r2);
}

void test_flip_bit(void) {
    T("bool_router_flip_bit (Tsetlin)");
    BoolRouter *r = bool_router_create(1, 8, NULL);
    C(bool_router_flip_bit(r,3)==0, "flip bit 3 ok");
    C((r->bits[0]&0x08)==0x08, "bit 3 is now 1");
    C(bool_router_flip_bit(r,3)==0, "flip bit 3 again");
    C((r->bits[0]&0x08)==0x00, "bit 3 back to 0");
    C(bool_router_flip_bit(r,99)==-1, "reject out-of-range bit");
    bool_router_destroy(r);
}

void test_save_load(void) {
    T("bool_router_save/load");
    uint8_t bits[1]={0xAA};
    BoolRouter *r = bool_router_create(1, 8, bits);
    C(!bool_router_save(r,"_br_test.bin"), "save ok");
    BoolRouter *r2 = bool_router_load("_br_test.bin");
    C(r2 && r2->uid==1 && r2->num_bits==8 && r2->bits[0]==0xAA, "load roundtrip");
    bool_router_destroy(r); bool_router_destroy(r2); remove("_br_test.bin");
}

void test_forward_layer(void) {
    T("bool_router_forward_layer (BoolNet adaptor)");
    uint8_t bits[1]={0x0F}; /* flip low 4 bits */
    BoolRouter *r = bool_router_create(1, 8, bits);
    uint8_t in[1]={0xFF}, out[1]={0};
    C(!bool_router_forward_layer(r,in,out), "forward_layer ok");
    C(out[0]==0xF0, "0xFF^0x0F=0xF0");
    bool_router_destroy(r);
}

int main(void) {
    test_create(); test_forward(); test_flip_bit(); test_save_load(); test_forward_layer();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F);
    return F>0;
}
