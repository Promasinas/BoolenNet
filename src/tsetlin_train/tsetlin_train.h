/**
 * tsetlin_train.h — Tsetlin Automaton Training (Constitution X)
 *
 * Pure Boolean byte-stream: all data is uint8_t.
 * Reward = byte_max - sum(|output[i] - target[i]|)  per byte.
 * Single bit-flip per step, accept if reward improved.
 */
#ifndef TSETLIN_TRAIN_H
#define TSETLIN_TRAIN_H
#include <stdint.h>
#include "../common/boolnet_common.h"
#include "../boolnet/boolnet.h"

typedef struct {
    BoolNet  *network;
    uint32_t  neg_tolerance, neg_counter;
    uint32_t  step_count, accept_count;
    uint32_t  byte_max;
    int32_t   best_reward;
} TsetlinTrainer;

TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max);
void            tsetlin_destroy(TsetlinTrainer *t);
int             tsetlin_train_step(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target);
int             tsetlin_train_seq(TsetlinTrainer *t, const uint8_t **tokens, uint32_t seq_len,
                                  const uint8_t *target);
int             tsetlin_train_epochs(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target, uint32_t max_epochs);
void            tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *steps, uint32_t *accepted, int32_t *best_r);
int             tsetlin_save(const TsetlinTrainer *t, const char *path);
TsetlinTrainer* tsetlin_load(const char *path);
#endif
