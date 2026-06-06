/**
 * partition.h — Internal partition storage (SoA layout)
 */
#ifndef PARTITION_H
#define PARTITION_H

#include <stdint.h>
#include <stdbool.h>
#include "part_types.h"

typedef struct {
    uint32_t  num_partitions, activation_dim;
    float    *activations;     /* P*D floats, SoA */
    uint8_t  *occupancy;       /* P bits          */
    uint64_t *last_write;      /* P entries       */
    uint64_t *last_access;     /* P entries       */
    uint64_t  global_clock;
    bool      initialized;
} partition_store_t;

int  partition_store_alloc(partition_store_t *ps, uint32_t P, uint32_t D);
void partition_store_free(partition_store_t *ps);
void partition_store_clear(partition_store_t *ps);

static inline bool partition_addr_valid(const partition_store_t *ps, uint32_t addr) {
    return addr < ps->num_partitions;
}
static inline float *partition_get_ptr(const partition_store_t *ps, uint32_t addr) {
    return ps->activations + (uint64_t)addr * ps->activation_dim;
}

bool     partition_is_occupied(const partition_store_t *ps, uint32_t addr);
void     partition_mark_occupied(partition_store_t *ps, uint32_t addr, bool occupied);
void     partition_touch_write(partition_store_t *ps, uint32_t addr);
void     partition_touch_read(partition_store_t *ps, uint32_t addr);
uint32_t partition_evict_lru(partition_store_t *ps);

#endif
