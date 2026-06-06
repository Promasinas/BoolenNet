/**
 * coord_descent.h — Coordinate Descent Optimizer for BoolNet
 *
 * Exploits ultra-fast forward to deterministically optimize each bit.
 * O(2N) per sweep vs Tsetlin's O(K) with K >> N.
 *
 * For N=256: ~512 forwards ≈ 0.5ms per sweep (vs Tsetlin 20k steps).
 */
#ifndef COORD_DESCENT_H
#define COORD_DESCENT_H
#include <stdint.h>
#include "../boolnet/boolnet.h"

/**
 * One sweep: test every trainable bit, keep better value.
 * Returns: number of bits flipped (0 = converged).
 *
 * @param net       Network with Router/Conv1D layers
 * @param input     Training input
 * @param target    Training target
 * @param byte_max  Reward = byte_max - sum|out - target|
 * @return bits changed this sweep
 */
int coord_descent_sweep(BoolNet *net, const uint8_t *input,
                        const uint8_t *target, uint32_t byte_max);

/**
 * Run sweeps until convergence or max_sweeps.
 * Returns: total sweeps performed.
 */
int coord_descent_train(BoolNet *net, const uint8_t *input,
                        const uint8_t *target, uint32_t byte_max,
                        uint32_t max_sweeps);

#endif
