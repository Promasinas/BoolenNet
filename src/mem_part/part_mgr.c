/**
 * part_mgr.c — Partition Activation Manager implementation
 */
#include <string.h>
#include "part_mgr.h"
#include "partition.h"
#include "vector_ops.h"
#include "trigger.h"

static partition_store_t g_store;
static bool g_ok = false;
static uint32_t g_P = 0, g_D = 0;

static int _chk(void)   { return g_ok ? PART_OK : PART_ERR_NOT_INIT; }
static int _addr(uint32_t a) { return (a < g_P) ? PART_OK : PART_ERR_BAD_ADDRESS; }

int part_mgr_init(uint32_t P, uint32_t D) {
    if (P < 256 || P > 65536) return PART_ERR_BAD_ADDRESS;
    if (D < 64 || D > 4096)   return PART_ERR_DIM_MISMATCH;
    if (g_ok) { partition_store_free(&g_store); g_ok = false; }
    int rc = partition_store_alloc(&g_store, P, D);
    if (rc) return rc;
    partition_store_clear(&g_store);
    g_P = P; g_D = D; g_ok = true;
    return PART_OK;
}
void part_mgr_reset(void) { if (g_ok) partition_store_clear(&g_store); }

int part_mgr_write(uint32_t a, const float *v) {
    int r; if ((r=_chk())||(r=_addr(a))) return r;
    if (!v) return PART_ERR_NULL_PARAM;
    if (!partition_is_occupied(&g_store, a) && part_mgr_occupied_count() >= g_P)
        partition_evict_lru(&g_store);
    vector_copy(partition_get_ptr(&g_store, a), v, g_D);
    partition_touch_write(&g_store, a);
    return PART_OK;
}
int part_mgr_read(uint32_t a, float *out) {
    int r; if ((r=_chk())||(r=_addr(a))) return r;
    if (!out) return PART_ERR_NULL_PARAM;
    if (partition_is_occupied(&g_store, a))
        vector_copy(out, partition_get_ptr(&g_store, a), g_D);
    else vector_zero(out, g_D);
    partition_touch_read(&g_store, a);
    return PART_OK;
}
uint32_t part_mgr_occupied_count(void) {
    if (!g_ok) return 0;
    uint32_t c = 0;
    for (uint32_t i = 0; i < g_P; i++) if (partition_is_occupied(&g_store, i)) c++;
    return c;
}
float part_mgr_fill_ratio(void) {
    return (g_ok && g_P) ? (float)part_mgr_occupied_count() / (float)g_P : 0.0f;
}
int part_mgr_trigger(const trigger_pattern_t *p, float *out) {
    int r; if ((r=_chk())) return r;
    if (!p || !out) return PART_ERR_NULL_PARAM;
    if (p->output_dim != g_D) return PART_ERR_DIM_MISMATCH;
    if (p->num_entries == 0) { vector_zero(out, g_D); return PART_OK; }
    if ((r = trigger_compute(&g_store, p, out))) return r;
    for (uint32_t i = 0; i < p->num_entries; i++)
        if (partition_addr_valid(&g_store, p->entries[i].address))
            partition_touch_read(&g_store, p->entries[i].address);
    return PART_OK;
}
int part_mgr_get_status(uint32_t a, part_status_t *out) {
    int r; if ((r=_chk())||(r=_addr(a))) return r;
    if (!out) return PART_ERR_NULL_PARAM;
    memset(out, 0, sizeof(*out));
    out->address = a;
    out->occupied = partition_is_occupied(&g_store, a);
    if (out->occupied) {
        out->last_write_time  = g_store.last_write[a];
        out->last_access_time = g_store.last_access[a];
        float *v = partition_get_ptr(&g_store, a);
        uint32_t n = (g_D < 8) ? g_D : 8;
        for (uint32_t j = 0; j < n; j++) out->activation_preview[j] = v[j];
    }
    return PART_OK;
}
int part_mgr_get_bitmask(uint8_t *bm) {
    if (_chk()) return PART_ERR_NOT_INIT;
    if (!bm) return PART_ERR_NULL_PARAM;
    memcpy(bm, g_store.occupancy, (g_P + 7) / 8);
    return PART_OK;
}
