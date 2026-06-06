# Test Request: Integer Memory Layer

## Target UID

1

## Target Module

mem_int

## Test Priority

high

## Test Params

- timeout: 300
- cflags: -Wall -Wextra -std=c99

## Test Scope

Constitution VIII integer memory with decay + trigger + persistence.

### Source Files

- `src/common/boolnet_common.h` — shared types (LayerUID, MemLayerPrecision, error codes)
- `src/mem_int/mem_int_layer.h` — public API (8 functions)
- `src/mem_int/mem_int_layer.c` — full implementation

### Build

```bash
cd src/ && make mem_int
```

### Test Cases

1. **Create/Destroy**: `mem_int_create(2, 0, 100, 1000, 5)` succeeds for all 4 precision types
2. **Forward propagation**: router_signal → cells set to max_value; no signal → cells decay by `decay` (no underflow)
3. **Query trigger mask**: cells > 0 → mask bit = 1
4. **Persistence roundtrip**: save → load → byte-identical comparison via `mem_int_verify_roundtrip`
5. **Edge cases**: NULL parameters, zero length, invalid precision, corrupt file load → all handled gracefully
