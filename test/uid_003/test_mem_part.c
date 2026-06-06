/* Per-function tests for mem_part — UID 003 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/mem_part/part_mgr.h"
static int P=0,F=0; static const uint32_t PD=256, DD=64;
#define T(n) printf("[TEST] %s...\n",n)
#define C(c,n) do{if(c){printf("  [PASS]\n");P++;}else{printf("  [FAIL] %s\n",n);F++;}}while(0)

void test_init(void) { T("part_mgr_init");
    C(!part_mgr_init(256,64),"init P=256,D=64");
    C(part_mgr_init(100,64)!=0,"reject P<256");
    C(part_mgr_init(256,30)!=0,"reject D<64"); }

void test_write(void) { T("part_mgr_write");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v)); v[0]=42.0f;
    C(!part_mgr_write(0,v),"write addr 0 ok");
    C(part_mgr_write(999,v)==PART_ERR_BAD_ADDRESS,"reject OOB addr"); }

void test_read(void) { T("part_mgr_read");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v)); v[0]=7.5f; v[1]=3.2f;
    part_mgr_write(10,v); float out[DD]; memset(out,0,sizeof(out));
    C(!part_mgr_read(10,out) && fabsf(out[0]-7.5f)<0.001f && fabsf(out[1]-3.2f)<0.001f, "read matches write");
    memset(out,0xFF,sizeof(out)); part_mgr_read(50,out);
    int z=1; for(uint32_t i=0;i<DD;i++) if(fabsf(out[i])>0.001f)z=0;
    C(z,"empty partition→zero"); }

void test_trigger(void) { T("part_mgr_trigger");
    part_mgr_init(PD,DD); float a[DD],b[DD]; memset(a,0,sizeof(a)); memset(b,0,sizeof(b)); a[0]=1;a[1]=2; b[0]=10;b[1]=20;
    part_mgr_write(0,a); part_mgr_write(1,b);
    /* weight 2.0→clamped to 1.0, weight -1.0 stays -1.0 */
    trigger_entry_t e[2]={{0,2.0f},{1,-1.0f}}; trigger_pattern_t p={2,e,DD}; float out[DD];
    C(!part_mgr_trigger(&p,out), "trigger ok");
    C(fabsf(out[0]-(1.0f*1-1.0f*10))<0.01f && fabsf(out[1]-(1.0f*2-1.0f*20))<0.01f, "weighted sum (2.0→1.0 clamped)"); }

void test_status(void) { T("part_mgr_get_status");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v)); v[0]=1; part_mgr_write(5,v);
    part_status_t s; part_mgr_get_status(5,&s);
    C(s.occupied && s.address==5, "occupied addr 5");
    part_mgr_get_status(100,&s); C(!s.occupied,"empty addr 100"); }

void test_bitmask(void) { T("part_mgr_get_bitmask");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v));
    part_mgr_write(0,v); part_mgr_write(2,v); part_mgr_write(7,v);
    uint8_t bm[32]={0}; part_mgr_get_bitmask(bm);
    C((bm[0]&0x85)==0x85,"bits 0,2,7 set"); }

void test_occupied_count(void) { T("part_mgr_occupied_count");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v));
    C(part_mgr_occupied_count()==0,"initially 0");
    part_mgr_write(0,v); part_mgr_write(1,v); part_mgr_write(2,v);
    C(part_mgr_occupied_count()==3,"3 after 3 writes"); }

void test_fill_ratio(void) { T("part_mgr_fill_ratio");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v));
    part_mgr_write(0,v); C(fabsf(part_mgr_fill_ratio()-1.0f/PD)<0.001f,"ratio 1/256"); }

void test_reset(void) { T("part_mgr_reset");
    part_mgr_init(PD,DD); float v[DD]; memset(v,0,sizeof(v)); part_mgr_write(0,v); part_mgr_reset();
    C(part_mgr_occupied_count()==0,"reset clears all"); }

int main(void) {
    test_init(); test_write(); test_read(); test_trigger(); test_status();
    test_bitmask(); test_occupied_count(); test_fill_ratio(); test_reset();
    printf("\n=== SUMMARY: %d/%d ===\n", P, P+F); return F>0;
}
