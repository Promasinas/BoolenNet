# Tasks: 一维回环卷积

## Phase 1: Setup
- [ ] T001 Create `src/conv1d_circular.h`: API + Conv1DConfig struct
- [ ] T002 [P] Update Makefile

## Phase 2: US1 — 基本回环卷积 (P1)
- [ ] T003 [US1] Implement `conv1d_circular()`: O(N×K) double loop, modulo wrap index
- [ ] T004 [US1] Validate: each input position processed exactly K times

## Phase 3: US2 — Stride + Dilation (P2)
- [ ] T005 [US2] Implement stride > 1: output_length = ceil(N/stride), skip positions
- [ ] T006 [US2] Implement dilation > 1: skip (dilation-1) between kernel elements

## Phase 4: Multi-type + Persist
- [ ] T007 Add macro dispatch for int8/16/32/64
- [ ] T008 Implement save/load with UID

## Phase 5: Polish
- [ ] T009 Compile verify: 0 errors
- [ ] T010 Agent 3 notification

## MVP: Phase 1–2 (T001–T004)
