# Tasks: Tsetlin 自动机训练

## Phase 1: Setup
- [ ] T001 Create `src/tsetlin_train.h`: TsetlinTrainer struct + train_step/config/save API
- [ ] T002 [P] Update Makefile

## Phase 2: US1 — 单步训练 (P1)
- [ ] T003 [US1] Implement `tsetlin_train_step()` in `src/tsetlin_train.c`: pick random layer, flip random bit, forward, compute reward, accept/reject
- [ ] T004 [US1] Implement reward function: byte_max - abs(output_byte - target_byte)
- [ ] T005 [US1] Implement negative reward tolerance: counter, stop on exceed

## Phase 3: US2 — 多轮收敛 (P2)
- [ ] T006 [US2] Implement `tsetlin_train_epochs()`: loop train_step until converge or max
- [ ] T007 [US2] Convergence check: accuracy > threshold for N consecutive steps

## Phase 4: Polish
- [ ] T008 Compile verify + XOR convergence test
- [ ] T009 Agent 3 notification

## MVP: Phase 1–2 (T001–T005)
