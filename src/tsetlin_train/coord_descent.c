/**
 * coord_descent.c — Coordinate Descent Optimizer
 *
 * Exploits ultra-fast Boolean forward (~1µs) to deterministically
 * test each bit position. O(2N) per sweep, converges in 1-3 sweeps.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "coord_descent.h"
#include "../bool_router/bool_router.h"
#include "../conv1d_circular/conv1d_circular.h"
#include "../mem_int/mem_int_layer.h"

/* Flip a specific bit in a layer. Returns 0 on success. */
static int flip_bit(BoolNetLayer *layer, uint32_t bit_idx) {
    if (layer->type == LAYER_ROUTER)
        return bool_router_flip_bit((BoolRouter*)layer->instance, bit_idx);
    if (layer->type == LAYER_CONV1D)
        return conv1d_flip_kernel_bit((Conv1D*)layer->instance, bit_idx);
    return -1;
}

/* Count trainable bits across all Router + Conv1D layers */
static uint32_t count_trainable_bits(BoolNet *net) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER)
            total += ((BoolRouter*)net->layers[i].instance)->num_bits;
        else if (net->layers[i].type == LAYER_CONV1D)
            total += ((Conv1D*)net->layers[i].instance)->kernel_size;
    }
    return total;
}

/* Reset Memory layers between evaluations */
static void reset_all_memory(BoolNet *net) {
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)net->layers[i].instance);
}

/* Compute reward for current network state */
static int32_t compute_reward(BoolNet *net, const uint8_t *input,
                              const uint8_t *target, uint32_t byte_max) {
    uint8_t *out = (uint8_t*)malloc(net->output_dim);
    if (!out) return -1;
    reset_all_memory(net);
    boolnet_forward(net, input, out);
    int32_t r = (int32_t)byte_max;
    for (uint32_t i = 0; i < net->output_dim; i++) {
        int d = (int)out[i] - (int)target[i];
        r -= (d < 0 ? -d : d);
    }
    free(out);
    return r;
}

int coord_descent_sweep(BoolNet *net, const uint8_t *input,
                        const uint8_t *target, uint32_t byte_max) {
    int bits_changed = 0;
    uint32_t global_bit = 0;

    for (uint32_t li = 0; li < net->num_layers; li++) {
        if (net->layers[li].type != LAYER_ROUTER &&
            net->layers[li].type != LAYER_CONV1D) continue;

        uint32_t nbits = (net->layers[li].type == LAYER_ROUTER)
            ? ((BoolRouter*)net->layers[li].instance)->num_bits
            : ((Conv1D*)net->layers[li].instance)->kernel_size;

        for (uint32_t bi = 0; bi < nbits; bi++, global_bit++) {
            /* Current reward */
            int32_t before = compute_reward(net, input, target, byte_max);
            if (before < 0) continue;

            /* Flip bit */
            flip_bit(&net->layers[li], bi);

            /* Reward after flip */
            int32_t after = compute_reward(net, input, target, byte_max);

            if (after > before) {
                /* Keep the flip */
                bits_changed++;
            } else {
                /* Revert */
                flip_bit(&net->layers[li], bi);
            }
        }
    }
    return bits_changed;
}

int coord_descent_train(BoolNet *net, const uint8_t *input,
                        const uint8_t *target, uint32_t byte_max,
                        uint32_t max_sweeps) {
    uint32_t total_bits = count_trainable_bits(net);
    printf("CoordDescent: %u trainable bits, max %u sweeps\n",
           total_bits, max_sweeps);
    printf("  Each sweep: %u forwards (est %u ms @ 1us/forward)\n",
           total_bits * 2, total_bits * 2 / 1000);

    for (uint32_t s = 0; s < max_sweeps; s++) {
        int changed = coord_descent_sweep(net, input, target, byte_max);
        int32_t reward = compute_reward(net, input, target, byte_max);
        printf("  Sweep %u: %d bits flipped, reward=%d (perfect=%u)\n",
               s + 1, changed, reward, byte_max);
        if (changed == 0) {
            printf("  Converged at sweep %u\n", s + 1);
            return s + 1;
        }
    }
    return max_sweeps;
}
