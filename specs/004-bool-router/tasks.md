# Tasks: 布尔路由器

## Phase 1: Setup
- [ ] T001 Create `src/bool_router.h`: BoolRouter struct + 6 API declarations
- [ ] T002 [P] Update `src/Makefile`: add bool_router.o target

## Phase 2: US1 — 单路由翻转 (P1)
- [ ] T003 [US1] Implement `bool_router_create()` + `bool_router_destroy()` in `src/bool_router.c`
- [ ] T004 [US1] Implement `bool_router_forward()`: per-bit XOR, length validation
- [ ] T005 [US1] Verify edge cases: all-0 route (identity), all-1 route (NOT), length mismatch error

## Phase 3: US2 — 批量并行 (P2)
- [ ] T006 [US2] Implement `bool_router_forward_batch()` in `src/bool_router.c`: M routers × 1 input → M outputs
- [ ] T007 [US2] Verify: 0 routers → empty batch, 3 routers → 3 outputs

## Phase 4: US3 — 持久化 (P2)
- [ ] T008 [US3] Implement `bool_router_save()`: binary save (uid + length + bits)
- [ ] T009 [US3] Implement `bool_router_load()`: validate + allocate + restore
- [ ] T010 [US3] Verify roundtrip: save → load → forward produces identical output

## Phase 5: Polish
- [ ] T011 Compile with -Wall -Wextra -std=c99, verify 0 errors
- [ ] T012 Create docs/interact notification for Agent 3

## MVP: Phase 1–2 (T001–T005)
