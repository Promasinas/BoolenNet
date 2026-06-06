# Tasks: Memory 层模块

**Input**: Design documents from `/specs/001-agent1-design/`

## Phase 1: Setup

- [x] T001 Create `src/boolnet_common.h`: LayerUID, MemLayerPrecision enum, error codes, helper macros per data-model.md
- [x] T002 [P] Create `src/memory_layer.h`: MemLayer struct + 8 function declarations per contracts/memory-layer-api.md
- [x] T003 [P] Create `src/Makefile`: compile target memory_layer.o with -Wall -Wextra -std=c99

## Phase 2: Foundational

- [x] T004 Implement `memlayer_create()` in `src/memory_layer.c`: validate params, allocate struct+cells, init cells to 0, clamp max_value to precision range
- [x] T005 [P] Implement `memlayer_destroy()` in `src/memory_layer.c`: free cells, free struct, NULL-safe
- [x] T006 [P] Implement multi-precision helpers: cell_read(), cell_write(), cell_size() using switch-based dispatch (alternative to macros, functionally equivalent per research.md)

## Phase 3: User Story 1 - 创建与初始化 (P1) 🎯

- [x] T007 [US1] Add validation edge cases: reject length=0, invalid precision > 3, allocation failure with NULL return
- [x] T008 [US1] Implement `memlayer_all_zero()` helper in `src/memory_layer.c`

## Phase 4: User Story 2 - Forward 传播 (P1) 🎯

- [x] T009 [US2] Implement `memlayer_forward()` with overflow-safe decay: `val >= decay ? val - decay : 0`
- [x] T010 [US2] Multi-precision dispatch via cell_read/cell_write internal helpers (switch-based, covers all 4 types)
- [x] T011 [US2] Edge cases verified: all-zero (no underflow), all-triggered (reset to max), mixed signal, decay>value (clamp to 0)

## Phase 5: User Story 3 - 状态查询 (P2)

- [x] T012 [US3] Implement `memlayer_query()`: trigger_mask[i] = (cell_read(layer, i) > 0) ? 1 : 0
- [x] T013 [US3] Idempotency: query() reads only, no state mutation → consecutive calls return identical results

## Phase 6: User Story 4 - 持久化 (P2)

- [x] T014 [US4] Implement `memlayer_save()`: 25-byte binary header + raw cells via fwrite(), LE byte order
- [x] T015 [US4] Implement `memlayer_load()`: validate header, allocate via memlayer_create(), fread cells, error cleanup with goto fail
- [x] T016 [US4] Implement `memlayer_verify_roundtrip()`: save→load→memcmp, returns mismatch code (0=perfect)

## Phase 7: Polish

- [x] T017 [P] Doxygen comments complete in `src/memory_layer.h` — all public functions documented
- [x] T018 [P] Doxygen comments complete in `src/memory_layer.c` — implementation sections documented
- [x] T019 NULL guards: all public functions (forward, query, save, load, all_zero, verify_roundtrip) check input pointers
- [x] T020 Compilation verified: `gcc -Wall -Wextra -std=c99 -pedantic -c memory_layer.c` — zero errors, zero warnings
- [x] T021 Agent 3 notification ready in `docs/interact/`

## Summary

**21/21 tasks complete** ✅

Code: `src/memory_layer.c` (254 lines) + `src/memory_layer.h` (122 lines) + `src/boolnet_common.h` (55 lines)
Compiles cleanly with `gcc -Wall -Wextra -std=c99 -pedantic`.
