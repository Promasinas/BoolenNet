# Tasks: Forward 函数

## Phase 1: Setup
- [x] T001 Create `src/boolnet.h`: LayerType enum + Layer struct (union of all layer types) + BoolNet struct
- [x] T002 [P] Update Makefile

## Phase 2: US1+US2 — Forward + Topology (P1)
- [x] T003 [US1] Implement `boolnet_add_layer()` in `src/boolnet.c`: append layer sorted by UID
- [x] T004 [US1] Implement `boolnet_forward()`: iterate layers, call each layer's forward, pass output→next input
- [x] T005 [US1] Error propagation: layer returns error → stop + return error code

## Phase 3: US3 — Persist (P2)
- [x] T006 [US3] Implement `boolnet_save()`: save all layers sequentially
- [x] T007 [US3] Implement `boolnet_load()`: reconstruct network from saved data

## Phase 4: Polish
- [x] T008 Compile verify with all dependencies linked
- [x] T009 Agent 3 notification

## MVP: Phase 1–2 (T001–T005)
