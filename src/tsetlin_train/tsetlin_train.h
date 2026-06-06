/**
 * tsetlin_train.h — Tsetlin Automaton Training Framework
 *
 * Complete training pipeline: epochs, validation, checkpointing, early stopping.
 */
#ifndef TSETLIN_TRAIN_H
#define TSETLIN_TRAIN_H
#include <stdint.h>
#include "../common/boolnet_common.h"
#include "../boolnet/boolnet.h"

/* ---- Trainer State ---- */
typedef struct {
    BoolNet  *network;
    uint32_t  neg_tolerance, neg_counter;
    uint32_t  step_count, accept_count, epoch_count;
    uint32_t  byte_max;
    int32_t   best_reward;
    int32_t   best_val_acc;       /* best validation accuracy */
    uint32_t  steps_since_best;   /* for early stopping */
} TsetlinTrainer;

/* ---- Training Config ---- */
typedef struct {
    uint32_t  max_epochs;         /* max training epochs */
    uint32_t  steps_per_epoch;    /* training steps per epoch */
    uint32_t  neg_tolerance;      /* consecutive neg rewards before stopping */
    uint32_t  byte_max;           /* reward = byte_max - sum|out-target| */
    uint32_t  early_stop_patience;/* epochs without improvement before stop */
    uint32_t  checkpoint_every;   /* save checkpoint every N epochs */
    uint32_t  eval_every;         /* evaluate every N steps */
    const char *checkpoint_dir;   /* directory for checkpoint files */
} TrainConfig;

/* ---- Lifecycle ---- */
TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max);
void            tsetlin_destroy(TsetlinTrainer *t);

/* ---- Core Training ---- */
int  tsetlin_train_step(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target);
int  tsetlin_train_seq(TsetlinTrainer *t, const uint8_t **tokens, uint32_t seq_len,
                       const uint8_t *target);

/* ---- High-Level Training Loop ---- */
/**
 * Full training loop with validation, checkpointing, early stopping.
 *
 * @param t          trainer instance
 * @param cfg        training configuration
 * @param train_in   training inputs [num_train][input_dim]
 * @param train_tg   training targets [num_train][output_dim]
 * @param num_train  number of training samples
 * @param val_in     validation inputs (can be NULL)
 * @param val_tg     validation targets
 * @param num_val    number of validation samples (0 = skip)
 * @param reset_fn   callback to reset memory between samples (can be NULL)
 * @return final validation accuracy (0-100), or -1 on error
 */
int tsetlin_train_full(TsetlinTrainer *t, const TrainConfig *cfg,
                       const uint8_t *train_in, const uint8_t *train_tg, uint32_t num_train,
                       const uint8_t *val_in,   const uint8_t *val_tg,   uint32_t num_val,
                       uint32_t input_dim, uint32_t output_dim,
                       void (*reset_fn)(BoolNet*));

/* ---- Stats & Persistence ---- */
void tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *steps, uint32_t *accepted,
                       int32_t *best_r, uint32_t *epochs, int32_t *best_val);
int  tsetlin_save(const TsetlinTrainer *t, const char *path);
TsetlinTrainer* tsetlin_load(const char *path);

/* ---- Model Export / Import ---- */
int      tsetlin_export_model(BoolNet *net, const char *path);
BoolNet* tsetlin_import_model(const char *path);

#endif
