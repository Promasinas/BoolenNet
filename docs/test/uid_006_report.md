# Test Report: tsetlin_train (Tsetlin Automaton Training)

## Module Info
- **UID**: 6 | **Module**: tsetlin_train | **Source**: ./src/tsetlin_train/

## Summary
- **Total Tests**: 10 | **Passed**: 10 | **Failed**: 0 | **Pass Rate**: 100.0%

## Results by Story

| Story | Tests | Result |
|-------|-------|--------|
| US1: Create trainer | 3 | ✅ create(net,tol=5,bmax=255), reject NULL net |
| US2: Train step | 1 | ✅ step completes without crash |
| US3: Stats query | 2 | ✅ step_count, accepted, best_reward tracked |
| US4: Save/Load | 4 | ✅ binary persistence, all counters preserved |

## Notes
- Reward: `byte_max - sum(|output - target|)` — positive → accept flip
- Negative tolerance stops training when consecutive rejects exceed threshold
