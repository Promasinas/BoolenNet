# Test Report: upsampling (Multi-Router Upsampling)

## Module Info
- **UID**: 4 | **Module**: upsampling | **Source**: ./src/upsampling/

## Summary
- **Total Tests**: 12 | **Passed**: 12 | **Failed**: 0 | **Pass Rate**: 100.0%

## Results by Story

| Story | Tests | Result |
|-------|-------|--------|
| US1: Create | 3 | ✅ create(M=2,N=4), reject M=0, reject N=0 |
| US2: Router set + Forward | 6 | ✅ flip=negate, pass=identity, output concatenation |
| US3: Cascade | 2 | ✅ 2-layer cascade, double-negation verified |

## Notes
- Router: bit=0 → pass (identity), bit=1 → flip (negate)
- Output: M × N concatenated floats per router segment
- Cascade: intermediate buffers allocated dynamically, handles chained layers
