#include <stdlib.h>
#include <string.h>
#include "partition.h"

int partition_store_alloc(partition_store_t *ps, uint32_t P, uint32_t D) {
    size_t vb = (size_t)P * D * sizeof(float);
    size_t ob = (size_t)((P + 7) / 8);
    size_t tb = (size_t)P * sizeof(uint64_t);
    ps->activations = (float   *)calloc(1, vb);
    ps->occupancy   = (uint8_t *)calloc(1, ob);
    ps->last_write  = (uint64_t*)calloc(1, tb);
    ps->last_access = (uint64_t*)calloc(1, tb);
    if (!ps->activations || !ps->occupancy || !ps->last_write || !ps->last_access) {
        partition_store_free(ps); return PART_ERR_ALLOC;
    }
    ps->num_partitions = P; ps->activation_dim = D;
    ps->global_clock = 0; ps->initialized = true;
    return PART_OK;
}
void partition_store_free(partition_store_t *ps) {
    free(ps->activations); free(ps->occupancy);
    free(ps->last_write); free(ps->last_access);
    ps->activations = ps->occupancy = NULL;
    ps->last_write = ps->last_access = NULL;
    ps->initialized = false;
}
void partition_store_clear(partition_store_t *ps) {
    if (!ps->initialized) return;
    memset(ps->activations, 0, (size_t)ps->num_partitions * ps->activation_dim * sizeof(float));
    memset(ps->occupancy,   0, (size_t)((ps->num_partitions + 7) / 8));
    memset(ps->last_write,  0, (size_t)ps->num_partitions * sizeof(uint64_t));
    memset(ps->last_access, 0, (size_t)ps->num_partitions * sizeof(uint64_t));
    ps->global_clock = 0;
}
bool partition_is_occupied(const partition_store_t *ps, uint32_t addr) {
    if (!partition_addr_valid(ps, addr)) return false;
    return (ps->occupancy[addr / 8] >> (addr % 8)) & 1U;
}
void partition_mark_occupied(partition_store_t *ps, uint32_t addr, bool occ) {
    if (!partition_addr_valid(ps, addr)) return;
    if (occ) ps->occupancy[addr / 8] |=  (uint8_t)(1U << (addr % 8));
    else     ps->occupancy[addr / 8] &= ~(uint8_t)(1U << (addr % 8));
}
void partition_touch_write(partition_store_t *ps, uint32_t addr) {
    if (!partition_addr_valid(ps, addr)) return;
    ps->global_clock++;
    ps->last_write[addr] = ps->last_access[addr] = ps->global_clock;
    partition_mark_occupied(ps, addr, true);
}
void partition_touch_read(partition_store_t *ps, uint32_t addr) {
    if (!partition_addr_valid(ps, addr)) return;
    ps->global_clock++;
    ps->last_access[addr] = ps->global_clock;
}
uint32_t partition_evict_lru(partition_store_t *ps) {
    uint64_t min_ts = UINT64_MAX; uint32_t victim = 0; bool found = false;
    for (uint32_t i = 0; i < ps->num_partitions; i++) {
        if (partition_is_occupied(ps, i) && ps->last_access[i] < min_ts) {
            min_ts = ps->last_access[i]; victim = i; found = true;
        }
    }
    if (!found) return 0;
    memset(partition_get_ptr(ps, victim), 0, (size_t)ps->activation_dim * sizeof(float));
    partition_mark_occupied(ps, victim, false);
    ps->last_write[victim] = ps->last_access[victim] = 0;
    return victim;
}
