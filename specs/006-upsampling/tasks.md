# Tasks: 多路由上采样

## Phase 1: Setup
- [x] T001 Create `src/upsampling.h`: UpsampleLayer struct + API
- [x] T002 [P] Update Makefile

## Phase 2: US1 — 基本上采样 (P1)
- [x] T003 [US1] Implement `upsample_create()` / `upsample_destroy()` in `src/upsampling.c`
- [x] T004 [US1] Implement `upsample_forward()`: M routers × input → concatenated output
- [x] T005 [US1] Validate: router length mismatch → error, M=0 → error

## Phase 3: US2 — 级联 (P2)
- [x] T006 [US2] Implement `upsample_cascade()`: chain multiple UpsampleLayers
- [x] T007 [US2] Verify 2-level cascade: input N → M1×N → M1×M2×N

## Phase 4: Polish
- [x] T008 Compile verify
- [x] T009 Agent 3 notification

## MVP: Phase 1–2 (T001–T005)
