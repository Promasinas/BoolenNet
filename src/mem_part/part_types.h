/**
 * part_types.h — Type definitions for the Partition Activation Manager
 *
 * All structs, enums, and error codes used by mem_part/ modules.
 * See: specs/002-agent2-implementation/data-model.md
 */
#ifndef PART_TYPES_H
#define PART_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ---- Error codes ---- */
#define PART_OK                0
#define PART_ERR_BAD_ADDRESS  -1
#define PART_ERR_NULL_PARAM   -2
#define PART_ERR_NOT_INIT     -3
#define PART_ERR_DIM_MISMATCH -4
#define PART_ERR_ALLOC        -5

/* ---- Configuration (set at init, immutable) ---- */
typedef struct {
    uint32_t num_partitions;   /* P: total partition count [256..65536] */
    uint32_t activation_dim;   /* D: dimension of each activation vector */
    uint32_t uid;              /* Layer UID (constitution VII)           */
} part_config_t;

/* ---- Per-partition status (returned by part_mgr_get_status) ---- */
typedef struct {
    uint32_t address;                  /* partition index (0..P-1)       */
    bool     occupied;                 /* true if non-zero activation    */
    uint64_t last_write_time;          /* global-clock at last write     */
    uint64_t last_access_time;         /* global-clock at last access    */
    float    activation_preview[8];    /* first 8 dims for inspection    */
} part_status_t;

/* ---- Trigger pattern (sparse weighted address list) ---- */
typedef struct {
    uint32_t address;   /* partition address in [0, P-1]   */
    float    weight;    /* trigger weight in [-1.0, 1.0]   */
} trigger_entry_t;

typedef struct {
    uint32_t         num_entries;   /* number of trigger entries    */
    trigger_entry_t* entries;       /* pointer to array of entries  */
    uint32_t         output_dim;    /* must match activation_dim    */
} trigger_pattern_t;

#endif /* PART_TYPES_H */
