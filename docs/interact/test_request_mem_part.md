# Test Request: Partition Activation Manager

## Target UID

2

## Target Module

mem_part

## Test Priority

high

## Test Params

- timeout: 300
- cflags: -Wall -Wextra -std=c99

## Test Scope

Float partition manager with O(1) read/write, LRU eviction, sparse trigger weighted-sum, and status monitoring.

### Source Files

- `src/mem_part/part_mgr.h` — public API (9 functions)
- `src/mem_part/part_mgr.c` — unified implementation
- `src/mem_part/part_types.h` — type definitions
- `src/mem_part/partition.h/c` — SoA storage + LRU
- `src/mem_part/vector_ops.h/c` — float vector utilities
- `src/mem_part/trigger.h/c` — sparse weighted-sum
- `src/mem_part/part_config.h/c` — lifecycle

### Build

```bash
cd src/ && make mem_part && make diag
```

### Test Cases

1. **Init/Reset**: `part_mgr_init(512, 64)` succeeds; `part_mgr_reset()` clears all data
2. **Write (US1)**: write to partitions 0,1,2; OOB address → PART_ERR_BAD_ADDRESS; NULL → PART_ERR_NULL_PARAM
3. **Read (US2)**: read back matches written data; unoccupied partition → zero vector; idempotent reads
4. **Trigger (US3)**: weighted sum with positive + negative weights; empty trigger → zero vector; dimension mismatch → error
5. **Status (US4)**: occupancy bitmask, per-partition timestamps, occupied_count, fill_ratio
6. **LRU eviction**: fill all partitions → write new address → evict least-recently-accessed
7. **Diagnostic**: run `part_diag.exe` → all 15 checks PASS
