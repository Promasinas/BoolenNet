/**
 * part_mgr.h — Partition Activation Manager (Agent 2 unified API)
 *
 * Combines float partition storage, vector ops, and trigger weighted-sum.
 * See: specs/002-agent2-implementation/contracts/activation-mgr-api.md
 */
#ifndef PART_MGR_H
#define PART_MGR_H
#include "part_types.h"

/* Lifecycle */
int  part_mgr_init(uint32_t P, uint32_t D);
void part_mgr_reset(void);

/* US1/US2: Read/Write */
int part_mgr_write(uint32_t address, const float *activation);
int part_mgr_read(uint32_t address, float *out);

/* US3: Trigger */
int part_mgr_trigger(const trigger_pattern_t *pattern, float *out);

/* US4: Status */
int      part_mgr_get_status(uint32_t address, part_status_t *out);
int      part_mgr_get_bitmask(uint8_t *out_bitmask);
uint32_t part_mgr_occupied_count(void);
float    part_mgr_fill_ratio(void);
int      part_mgr_save(const char *path);
int      part_mgr_load(const char *path);

#endif
