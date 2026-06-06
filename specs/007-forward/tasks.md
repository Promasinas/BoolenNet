# Tasks: Forward 函数

## Phase 1: Setup
- [ ] T001 Create `src/boolnet.h`: LayerType enum + Layer struct (union of all layer types) + BoolNet struct
- [ ] T002 [P] Update Makefile

## Phase 2: US1+US2 — Forward + Topology (P1)
- [ ] T003 [US1] Implement `boolnet_add_layer()` in `src/boolnet.c`: append layer sorted by UID
- [ ] T004 [US1] Implement `boolnet_forward()`: iterate layers, call each layer's forward, pass output→next input
- [ ] T005 [US1] Error propagation: layer returns error → stop + return error code

## Phase 3: US3 — Persist (P2)
- [ ] T006 [US3] Implement `boolnet_save()`: save all layers sequentially
- [ ] T007 [US3] Implement `boolnet_load()`: reconstruct network from saved data

## Phase 4: Polish
- [ ] T008 Compile verify with all dependencies linked
- [ ] T009 Agent 3 notification

## MVP: Phase 1–2 (T001–T005)
