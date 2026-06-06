# Test Report: mem_int (Integer Memory Layer)

## Module Info

- **UID**: 1
- **Module**: mem_int
- **Source**: ./src/mem_int/mem_int_layer.c

## Test Time

- **Start**: 2026-06-06 22:05:00
- **Duration**: ~50 ms

## Compile Result

- **Status**: PASS (1 warning: misleading indentation in mem_int_load)
- **Duration**: ~500 ms
- **Compiler**: GCC 13.2.0 (MinGW-Builds)

### Compile Output

```text
src/mem_int/mem_int_layer.c:47:5: warning: this 'if' clause does not guard...
Compilation succeeded.
```

## Run Result

- **Status**: PASS
- **Duration**: ~50 ms
- **Exit Code**: 0

### Run Output

```text
=== TEST SUMMARY ===
Total: 22 | Passed: 22 | Failed: 0
```

## Summary

- **Total Tests**: 22
- **Passed**: 22
- **Failed**: 0
- **Pass Rate**: 100.0%

### Test Breakdown

| Story | Tests | Result |
|-------|-------|--------|
| US1: Create & Initialize | 5 | ✅ All pass |
| US2: Forward propagation | 6 | ✅ All pass |
| US3: Trigger query | 2 | ✅ All pass |
| US4: Save/Load/Roundtrip | 6 | ✅ All pass |
| Misc: all_zero check | 1 | ✅ Pass |
| Misc: Error handling (zero length, invalid precision) | 2 | ✅ Pass |

### Key Validations

- ✅ Create with valid params (uid, precision=uint8, length=10, max=255, decay=1)
- ✅ Reject zero length and invalid precision
- ✅ Trigger recovery: router_signal[0]=1 → cell[0] restored to max_value (255)
- ✅ Decay: non-triggered cells decrement by decay value
- ✅ Underflow protection: cell value clamped at 0 (no uint wrap)
- ✅ Query: cell > 0 → triggered (1), cell == 0 → not triggered (0)
- ✅ Binary save → file written with correct header (25B) + cell data
- ✅ Binary load → all fields restored (uid, precision, length, max_value, decay, cells)
- ✅ Roundtrip verification passes (mem_int_verify_roundtrip returns 0=success)
- ✅ Fresh layer is all zero
