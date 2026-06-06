/* Test suite for mem_part (Partition Activation Manager) — uid_002 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/mem_part/part_mgr.h"

static int g_pass = 0, g_fail = 0;

#define D 64
#define TEST(name) printf("[TEST] Running %s...\n", name)
#define CHECK(cond, msg, input, expect, actual) do { \
    if (cond) { printf("[PASS] %s\n", msg); g_pass++; } \
    else { printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n", msg, input, expect, actual); g_fail++; } \
} while(0)

static float vec_eq(const float *a, const float *b, int n) {
    for (int i = 0; i < n; i++) if (fabsf(a[i] - b[i]) > 0.001f) return 0;
    return 1;
}

int main(void) {
    int ret;
    float act0[D], act1[D], result[D];
    memset(act0, 0, sizeof(act0)); memset(act1, 0, sizeof(act1));

    /* Fill first few elements for visibility */
    for (int i = 0; i < 8; i++) act0[i] = (float)(i + 1);
    for (int i = 0; i < 8; i++) act1[i] = (float)((i + 1) * 10);

    /* === US1: Init === */
    TEST("US1: Initialize partition manager (P=256, D=64)");
    ret = part_mgr_init(256, D);
    CHECK(ret == PART_OK, "init with P=256, D=64", "P=256,D=64", "PART_OK(0)", "error");

    /* === US1/US2: Write + Read === */
    TEST("US1: Write activation to partition 0");
    ret = part_mgr_write(0, act0);
    CHECK(ret == PART_OK, "write to address 0", "valid", "PART_OK(0)", "error");

    TEST("US2: Read activation from partition 0");
    memset(result, 0, sizeof(result));
    ret = part_mgr_read(0, result);
    int read_ok = (ret == PART_OK) && vec_eq(result, act0, D);
    CHECK(read_ok, "read matches written values (D=64)", "vector[64]", "exact match", "mismatch");

    TEST("US1: Write rejects out-of-bounds address");
    ret = part_mgr_write(300, act0);
    CHECK(ret == PART_ERR_BAD_ADDRESS, "reject addr 300 with P=256", "addr=300", "BAD_ADDRESS(-1)", "other");

    TEST("US2: Read empty partition returns zeros");
    memset(result, 0xFF, sizeof(result));
    ret = part_mgr_read(10, result);
    int all_zero = 1;
    for (int i = 0; i < D; i++) if (fabsf(result[i]) > 0.0001f) { all_zero = 0; break; }
    CHECK(ret == PART_OK && all_zero, "empty partition returns zero vector", "addr=10", "zeros", "non-zero");

    /* Write more */
    part_mgr_write(1, act0);
    part_mgr_write(2, act1);

    /* === US3: Trigger === */
    TEST("US3: Trigger weighted-sum — two partitions w=1.0");
    trigger_entry_t entries[2] = {{0, 1.0f}, {2, 1.0f}};
    trigger_pattern_t pattern = {2, entries, D};
    memset(result, 0, sizeof(result));
    ret = part_mgr_trigger(&pattern, result);
    float expected[D];
    memset(expected, 0, sizeof(expected));
    for (int i = 0; i < 8; i++) expected[i] = act0[i] + act1[i];
    int trig_ok = (ret == PART_OK) && vec_eq(result, expected, D);
    CHECK(trig_ok, "weighted sum act0+act1 correct", "w=1.0 on 0,2", "expected sum", "different");

    TEST("US3: Trigger with negative weight");
    trigger_entry_t entries2[2] = {{0, 1.0f}, {2, -1.0f}};
    trigger_pattern_t pattern2 = {2, entries2, D};
    memset(result, 0, sizeof(result));
    ret = part_mgr_trigger(&pattern2, result);
    float exp2[D];
    memset(exp2, 0, sizeof(exp2));
    for (int i = 0; i < 8; i++) exp2[i] = act0[i] - act1[i];
    int trig2_ok = (ret == PART_OK) && vec_eq(result, exp2, D);
    CHECK(trig2_ok, "weighted sum act0-act1 correct", "w=1.0,-1.0", "expected", "different");

    /* === US4: Status === */
    TEST("US4: Occupancy and fill ratio");
    uint32_t occ = part_mgr_occupied_count();
    CHECK(occ == 3, "occupied count = 3", "3 writes", "3", "other");
    float ratio = part_mgr_fill_ratio();
    CHECK(fabsf(ratio - 3.0f/256.0f) < 0.001f, "fill ratio = 3/256", "3/256", "~0.0117", "other");

    TEST("US4: Get bitmask");
    uint8_t bitmask[32] = {0}; /* 256 bits */
    ret = part_mgr_get_bitmask(bitmask);
    CHECK(ret == PART_OK && (bitmask[0] & 0x07) == 0x07, "bits 0,1,2 set", "writes to 0,1,2", "0x07", "other");

    TEST("US4: Per-partition status");
    part_status_t status;
    ret = part_mgr_get_status(0, &status);
    CHECK(ret == PART_OK && status.occupied, "partition 0 occupied", "written", "occupied", "not occupied");
    ret = part_mgr_get_status(10, &status);
    CHECK(ret == PART_OK && !status.occupied, "partition 10 empty", "not written", "empty", "occupied");

    /* Reset */
    TEST("Reset");
    part_mgr_reset();
    CHECK(part_mgr_occupied_count() == 0, "after reset: occupied=0", "reset", "0", "other");

    printf("\n=== TEST SUMMARY ===\n");
    printf("Total: %d | Passed: %d | Failed: %d\n", g_pass + g_fail, g_pass, g_fail);
    return (g_fail > 0) ? 1 : 0;
}
