/* Per-function tests for mem_int (Constitution VIII) — UID 002 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/common/boolnet_common.h"
#include "../../src/mem_int/mem_int_layer.h"
static int P=0,F=0;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_create(void) {
    T("mem_int_create");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT8, 100, 255, 1);
    C(l && l->uid==2 && l->length==100, "create uint8 len=100");
    mem_int_destroy(l);
    C(!mem_int_create(3, ML_PRECISION_UINT16, 0, 255, 1), "reject len=0");
    C(!mem_int_create(4, 99, 10, 255, 1), "reject invalid precision");
}

void test_forward(void) {
    T("mem_int_forward");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT8, 5, 100, 3);
    uint8_t sig[5]={1,0,1,0,0}; mem_int_forward(l, sig);
    uint8_t *c=(uint8_t*)l->cells;
    C(c[0]==100 && c[2]==100, "trigger recovery to max=100");
    mem_int_forward(l, (uint8_t[]){0,0,0,0,0});
    C(c[0]==97 && c[1]==0, "decay by 3, cell1 stays at 0");
    mem_int_destroy(l);
}

void test_query(void) {
    T("mem_int_query");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT8, 4, 10, 1);
    uint8_t sig[4]={1,0,0,1}; mem_int_forward(l, sig);
    uint8_t mask[4]; mem_int_query(l, mask);
    C(mask[0]==1 && mask[1]==0 && mask[3]==1, "trigger mask correct");
    mem_int_destroy(l);
}

void test_save_load(void) {
    T("mem_int_save/load");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT32, 10, 999, 2);
    uint8_t sig[10]={1,1,0,0,0,0,0,0,0,0}; mem_int_forward(l, sig);
    C(!mem_int_save(l,"_mi_test.bin"), "save ok");
    MemIntLayer *l2 = mem_int_load("_mi_test.bin");
    C(l2 && l2->uid==2 && l2->precision==ML_PRECISION_UINT32, "load params match");
    C(!memcmp(l->cells,l2->cells,l->length*4), "cells match");
    mem_int_destroy(l); mem_int_destroy(l2); remove("_mi_test.bin");
}

void test_all_zero(void) {
    T("mem_int_all_zero");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT8, 3, 10, 1);
    C(mem_int_all_zero(l)==1, "fresh layer all zero");
    uint8_t sig[3]={0,1,0}; mem_int_forward(l, sig);
    C(mem_int_all_zero(l)==0, "after trigger not all zero");
    mem_int_destroy(l);
}

void test_verify_roundtrip(void) {
    T("mem_int_verify_roundtrip");
    MemIntLayer *l = mem_int_create(2, ML_PRECISION_UINT16, 20, 500, 5);
    uint8_t sig[20]; memset(sig,1,20); mem_int_forward(l, sig);
    C(!mem_int_verify_roundtrip(l), "roundtrip ok (0=success)");
    mem_int_destroy(l);
}

int main(void) {
    test_create(); test_forward(); test_query(); test_save_load(); test_all_zero(); test_verify_roundtrip();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F);
    return F>0;
}
