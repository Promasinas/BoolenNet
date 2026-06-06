#include <stdlib.h>
#include "part_config.h"
#include "partition.h"
static partition_store_t g_part_store;
static bool g_part_ready = false;
int part_config_init(uint32_t P, uint32_t D) {
    if (P < 256 || P > 65536) return PART_ERR_BAD_ADDRESS;
    if (D < 64 || D > 4096)   return PART_ERR_DIM_MISMATCH;
    if (g_part_ready) partition_store_free(&g_part_store);
    int rc = partition_store_alloc(&g_part_store, P, D);
    if (rc) return rc;
    partition_store_clear(&g_part_store);
    g_part_ready = true;
    return PART_OK;
}
void part_config_reset(void) {
    if (!g_part_ready) return;
    partition_store_clear(&g_part_store);
}
void part_config_get(part_config_t *out) {
    if (!out) return;
    out->num_partitions = g_part_store.num_partitions;
    out->activation_dim = g_part_store.activation_dim;
    out->uid = 0;
}
