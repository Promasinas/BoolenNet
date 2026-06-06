/**
 * part_diag.c — Diagnostic program for Partition Activation Manager
 * Build: gcc -std=c99 -I.. -o part_diag.exe part_diag.c part_mgr.c partition.c vector_ops.c trigger.c part_config.c
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "part_mgr.h"

#define TP 256
#define TD 64
static int ok = 0, fail = 0;
#define T(c, m) do { if(c){printf("  PASS: %s\n",m);ok++;}else{printf("  FAIL: %s\n",m);fail++;} } while(0)

int main(void) {
    printf("=== Partition Activation Manager Diag (P=%u D=%u) ===\n\n", (unsigned)TP, (unsigned)TD);

    T(part_mgr_init(TP, TD) == 0, "init OK");
    T(part_mgr_occupied_count() == 0, "occupied=0 after init");
    T(part_mgr_fill_ratio() == 0.0f, "fill=0 after init");

    float va[TD]; for (int i=0;i<TD;i++) va[i]=(float)(i+1);
    T(part_mgr_write(0, va) == 0, "write(0) OK");
    T(part_mgr_write(1, va) == 0, "write(1) OK");
    float vb[TD]; for (int i=0;i<TD;i++) vb[i]=(float)(i+1)*2.0f;
    T(part_mgr_write(2, vb) == 0, "write(2) OK");
    T(part_mgr_write(TP+100, va) == PART_ERR_BAD_ADDRESS, "write(OOB) -> ERR");
    T(part_mgr_write(10, NULL) == PART_ERR_NULL_PARAM, "write(NULL) -> ERR");
    T(part_mgr_occupied_count() == 3, "occupied=3");

    float out[TD];
    T(part_mgr_read(0, out) == 0 && memcmp(out, va, sizeof(va)) == 0, "read(0) matches");
    T(part_mgr_read(50, out) == 0, "read(unoccupied) OK");
    int z = 1; for (int i=0;i<TD;i++) if(out[i]!=0.0f) {z=0;break;}
    T(z, "unoccupied returns zero");

    trigger_entry_t e[2] = {{0,0.5f},{2,-0.5f}};
    trigger_pattern_t p = {2, e, TD};
    float sum[TD];
    T(part_mgr_trigger(&p, sum) == 0, "trigger OK");
    int t_ok = 1;
    for (int i=0;i<TD;i++) { float ex=0.5f*va[i]-0.5f*vb[i]; if(fabsf(sum[i]-ex)>1e-5f){t_ok=0;break;} }
    T(t_ok, "weighted sum correct");

    trigger_pattern_t empty = {0,NULL,TD}; float es[TD];
    T(part_mgr_trigger(&empty, es) == 0, "empty trigger -> zero");

    part_status_t st;
    T(part_mgr_get_status(0, &st) == 0 && st.occupied, "status(0) occupied");
    uint8_t bm[(TP+7)/8];
    T(part_mgr_get_bitmask(bm) == 0 && (bm[0]&0x07)==0x07, "bitmask shows 0,1,2");

    part_mgr_reset();
    T(part_mgr_occupied_count() == 0, "reset -> occupied=0");

    printf("\n=== %d passed, %d failed ===\n", ok, fail);
    return fail ? 1 : 0;
}
