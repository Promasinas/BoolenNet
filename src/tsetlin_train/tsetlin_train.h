/**
 * tsetlin_train.h — Tsetlin Automaton Training (Constitution X)
 *
 * Per-layer single-bit flip training with reward:
 *   reward = byte_max - |output_byte - target_byte|
 * Accept flip if reward > 0; reject and count negative reward.
 * Stop training when consecutive negative rewards exceed tolerance.
 */
#ifndef TSETLIN_TRAIN_H
#define TSETLIN_TRAIN_H
#include <stdint.h>
#include "../common/boolnet_common.h"
#include "../boolnet/boolnet.h"

typedef struct {
    BoolNet  *network;           /* the network being trained               */
    uint32_t  neg_tolerance;     /* max consecutive negative rewards        */
    uint32_t  neg_counter;       /* current consecutive negative count      */
    uint32_t  step_count;        /* total training steps executed           */
    uint32_t  accept_count;      /* number of accepted flips               */
    uint32_t  byte_max;          /* reward range: 0..byte_max              */
    float     best_reward;       /* best reward seen so far                 */
} TsetlinTrainer;

TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max);
void            tsetlin_destroy(TsetlinTrainer *t);
int             tsetlin_train_step(TsetlinTrainer *t, const float *target);
int             tsetlin_train_epochs(TsetlinTrainer *t, const float *target,
                                     uint32_t max_epochs, float converge_acc);
void            tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *steps,
                                  uint32_t *accepted, float *best_r);
int             tsetlin_save(const TsetlinTrainer *t, const char *path);
TsetlinTrainer* tsetlin_load(const char *path);

#endif
