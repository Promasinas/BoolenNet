# Activation Manager API

## Overview

The Activation Manager (`activation_mgr`) is Agent 2's unified high-level interface for the BoolNet partitioned memory layer. It manages **P** fixed-size partitions, each storing a **D**-dimensional float activation vector with O(1) direct-index access.

## Quick Start

```c
#include "activation_mgr.h"

int main() {
    activation_mgr_init(1024, 256);          // 1024 partitions × 256 dims

    float vec[256];
    for (int i = 0; i < 256; i++) vec[i] = 0.5f;
    activation_mgr_write(42, vec);           // write to partition 42

    float out[256];
    activation_mgr_read(42, out);            // read back — O(1)

    trigger_entry_t t[] = {{42, 0.8f}, {100, -0.3f}};
    trigger_pattern_t p = {2, t, 256};
    float sum[256];
    activation_mgr_trigger(&p, sum);         // weighted sum

    activation_mgr_reset();
    return 0;
}
```

## API Reference

### Lifecycle

| Function | Description |
|----------|-------------|
| `activation_mgr_init(P, D)` | Allocate P partitions × D dimensions. Returns 0 on success. |
| `activation_mgr_reset()` | Clear all partitions and timestamps. Does not free memory. |

### Read/Write

| Function | Description |
|----------|-------------|
| `activation_mgr_write(addr, vec)` | Write D-dim float vector to partition. Hard overwrite. |
| `activation_mgr_read(addr, out)` | Read D-dim float vector. O(1). Returns zero for empty slots. |

### Trigger

| Function | Description |
|----------|-------------|
| `activation_mgr_trigger(pattern, out)` | Weighted sum of triggered partitions. O(K×D) where K = occupied triggered. |

### Status

| Function | Description |
|----------|-------------|
| `activation_mgr_get_status(addr, out)` | Per-partition occupancy, timestamps, preview. |
| `activation_mgr_get_bitmask(out)` | Full P-bit occupancy bitmask (ceil(P/8) bytes). |
| `activation_mgr_occupied_count()` | Number of occupied partitions. |
| `activation_mgr_fill_ratio()` | Fill ratio ∈ [0.0, 1.0]. |

## Error Codes

| Code | Meaning |
|------|---------|
| 0 | OK |
| -1 | ERR_BAD_ADDRESS — address ≥ P |
| -2 | ERR_NULL_PARAM — NULL argument |
| -3 | ERR_NOT_INIT — call before init() |
| -4 | ERR_DIM_MISMATCH — pattern dim ≠ D |
| -5 | ERR_ALLOC — memory allocation failure |

## Build

```bash
cd src/
make all     # builds activation_mgr.o + dependencies
```

## See Also

- `specs/002-agent2-implementation/spec.md` — Feature specification
- `specs/002-agent2-implementation/plan.md` — Implementation plan
- `src/activation_mgr_diag.c` — Diagnostic / validation program
