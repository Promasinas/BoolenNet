/* Test suite for mem_int (Integer Memory Layer) — uid_001 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/common/boolnet_common.h"
#include "../../src/mem_int/mem_int_layer.h"

static int g_pass = 0, g_fail = 0;

#define TEST(name) printf("[TEST] Running %s...\n", name)
#define CHECK(cond, msg, input, expect, actual) do { \
    if (cond) { printf("[PASS] %s\n", msg); g_pass++; } \
    else { printf("[FAIL] %s | Input: %s | Expected: %s | Actual: %s\n", msg, input, expect, actual); g_fail++; } \
} while(0)

int main(void) {
    MemIntLayer *layer;
    uint8_t trigger_mask[16];
    uint8_t router_signal[16];
    int i;

    /* === US1: Create & Initialize === */
    TEST("US1: mem_int_create with valid params");
    layer = mem_int_create(1, ML_PRECISION_UINT8, 10, 255, 1);
    CHECK(layer != NULL, "create valid layer", "length=10,precision=uint8", "non-NULL", "NULL");
    CHECK(layer->uid == 1, "UID stored correctly", "1", "1", layer ? "?" : "N/A");
    CHECK(layer->length == 10, "length stored", "10", "10", layer ? "?" : "N/A");

    TEST("US1: mem_int_create with invalid params");
    MemIntLayer *bad1 = mem_int_create(2, ML_PRECISION_UINT8, 0, 255, 1);
    CHECK(bad1 == NULL, "reject zero length", "length=0", "NULL", "non-NULL");
    /* Note: precision 99 is checked inside create; C enum allows passing 99 */
    MemIntLayer *bad2 = mem_int_create(3, 99, 10, 255, 1);
    CHECK(bad2 == NULL, "reject invalid precision", "precision=99", "NULL", bad2 ? "non-NULL" : "NULL");
    if (bad2) mem_int_destroy(bad2);

    /* === US2: Forward propagation === */
    TEST("US2: forward — trigger recovery");
    /* Set some cells manually to verify trigger recovery */
    memset(router_signal, 0, 10);
    router_signal[0] = 1;  /* trigger cell 0 */
    router_signal[3] = 1;  /* trigger cell 3 */
    mem_int_forward(layer, router_signal);
    uint8_t *cells = (uint8_t*)layer->cells;
    CHECK(cells[0] == 255, "cell 0 restored to max (255)", "trigger cell0", "255", "other");
    CHECK(cells[3] == 255, "cell 3 restored to max (255)", "trigger cell3", "255", "other");
    CHECK(cells[1] == 0, "cell 1 unchanged (0-1 clamped to 0)", "no trigger", "0", "other");

    TEST("US2: forward — decay without underflow");
    /* After first forward, trigger cell 2 only */
    memset(router_signal, 0, 10);
    router_signal[2] = 1;
    mem_int_forward(layer, router_signal);
    /* cell 0 and 3 were 255, now decay by 1 → 254 */
    CHECK(cells[0] == 254, "cell 0 decay 255→254", "decay=1", "254", "other");
    CHECK(cells[3] == 254, "cell 3 decay 255→254", "decay=1", "254", "other");
    CHECK(cells[2] == 255, "cell 2 restored to max", "trigger cell2", "255", "other");

    TEST("US2: forward — underflow protection");
    /* Forward with no triggers — all cells decay */
    memset(router_signal, 0, 10);
    for (i = 0; i < 5; i++) mem_int_forward(layer, router_signal);
    /* Cell 1 was 0, should stay 0 (no underflow) */
    CHECK(cells[1] == 0, "cell 1 stays 0 (underflow protected)", "decay=1 from 0", "0", "other");

    /* === US3: Trigger status query === */
    TEST("US3: query trigger status");
    mem_int_query(layer, trigger_mask);
    CHECK(trigger_mask[0] == 1, "cell 0 triggered (value>0)", "value>0", "1", "0");
    CHECK(trigger_mask[1] == 0, "cell 1 not triggered (value=0)", "value=0", "0", "1");

    /* === US4: Save/Load persistence === */
    TEST("US4: save to file");
    int save_ok = mem_int_save(layer, "mem_int_save.bin");
    CHECK(save_ok == ML_OK, "save returns OK", "ML_OK(0)", "0", "other");

    TEST("US4: load from file");
    MemIntLayer *loaded = mem_int_load("mem_int_save.bin");
    CHECK(loaded != NULL, "load returns non-NULL", "valid file", "non-NULL", "NULL");
    if (loaded) {
        CHECK(loaded->uid == layer->uid, "loaded UID matches", "same uid", "same", "different");
        CHECK(loaded->length == layer->length, "loaded length matches", "same", "same", "different");
        CHECK(loaded->precision == layer->precision, "loaded precision matches", "same", "same", "different");
        uint8_t *loaded_cells = (uint8_t*)loaded->cells;
        int cells_match = (memcmp(cells, loaded_cells, layer->length) == 0);
        CHECK(cells_match, "all cells match after load", "same values", "match", "mismatch");
        mem_int_destroy(loaded);
    }

    TEST("US4: roundtrip verification");
    int rt_ok = mem_int_verify_roundtrip(layer);
    CHECK(rt_ok == 0, "verify_roundtrip passes (returns 0=success)", "0", "0", "other");

    TEST("all_zero check");
    /* Create a fresh layer — should be all zero */
    MemIntLayer *fresh = mem_int_create(99, ML_PRECISION_UINT32, 5, 100, 1);
    int all_z = mem_int_all_zero(fresh);
    CHECK(all_z == 1, "fresh layer is all zero", "all zero", "1", "0");
    mem_int_destroy(fresh);

    /* Cleanup */
    mem_int_destroy(layer);

    /* === SUMMARY === */
    printf("\n=== TEST SUMMARY ===\n");
    printf("Total: %d | Passed: %d | Failed: %d\n", g_pass + g_fail, g_pass, g_fail);
    return (g_fail > 0) ? 1 : 0;
}
