#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "tsetlin_train.h"

TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max) {
    if (!net) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1, sizeof(TsetlinTrainer));
    if (!t) return NULL;
    t->network = net; t->neg_tolerance = neg_tol; t->byte_max = byte_max;
    t->best_reward = -1e9f;
    return t;
}
void tsetlin_destroy(TsetlinTrainer *t) { free(t); }

int tsetlin_train_step(TsetlinTrainer *t, const float *target) {
    if (!t || !t->network || !target) return -1;

    /* 1. Pick a layer with router bits to flip (skip non-router layers) */
    uint32_t nl = t->network->num_layers;
    if (!nl) return -1;
    uint32_t li = (uint32_t)(rand() % nl);

    /* 2. Forward before flip */
    float *before = (float*)malloc(t->network->output_dim * sizeof(float));
    float *after  = (float*)malloc(t->network->output_dim * sizeof(float));
    if (!before || !after) { free(before); free(after); return -2; }

    /* For simplicity, forward on current state */
    /* In real impl, we'd get input from network context */
    /* Here we compute reward based on output vs target */

    /* 3. Flip a random bit in the selected layer (router context) */
    /* Actual bit-flip would be layer-type specific */
    /* For Tsetlin: flip one weight bit, re-forward, compare */

    /* 4. Compute reward: byte_max - sum(|output_byte - target_byte|) */
    float reward = (float)t->byte_max;
    for (uint32_t i = 0; i < t->network->output_dim; i++) {
        float diff = fabsf(after[i] - target[i]);
        if (diff > 255.0f) diff = 255.0f;
        reward -= diff;
    }

    /* 5. Accept/reject */
    t->step_count++;
    if (reward > 0) {
        t->accept_count++;
        t->neg_counter = 0;  /* reset negative streak */
        if (reward > t->best_reward) t->best_reward = reward;
        free(before); free(after);
        return 1;  /* accepted */
    } else {
        t->neg_counter++;
        /* revert the flip (in real impl: restore previous bit) */
        free(before); free(after);
        if (t->neg_counter >= t->neg_tolerance) return -2;  /* stop training */
        return 0;  /* rejected */
    }
}

int tsetlin_train_epochs(TsetlinTrainer *t, const float *target,
                         uint32_t max_epochs, float converge_acc) {
    if (!t || !target) return -1;
    for (uint32_t ep = 0; ep < max_epochs; ep++) {
        int rc = tsetlin_train_step(t, target);
        if (rc == -2) return 0;  /* stopped by negative tolerance */
        if (rc < 0) return rc;   /* error */
    }
    return 0;
}

void tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *steps, uint32_t *accepted, float *best_r) {
    if (steps)    *steps    = t ? t->step_count : 0;
    if (accepted) *accepted = t ? t->accept_count : 0;
    if (best_r)   *best_r   = t ? t->best_reward : 0.0f;
}

int tsetlin_save(const TsetlinTrainer *t, const char *path) {
    if (!t || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    fwrite(&t->neg_tolerance,4,1,f); fwrite(&t->neg_counter,4,1,f);
    fwrite(&t->step_count,4,1,f); fwrite(&t->accept_count,4,1,f);
    fwrite(&t->byte_max,4,1,f); fwrite(&t->best_reward,4,1,f);
    fclose(f); return 0;
}

TsetlinTrainer* tsetlin_load(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1, sizeof(TsetlinTrainer));
    if (!t) { fclose(f); return NULL; }
    if (fread(&t->neg_tolerance,4,1,f)!=1||fread(&t->neg_counter,4,1,f)!=1||
        fread(&t->step_count,4,1,f)!=1||fread(&t->accept_count,4,1,f)!=1||
        fread(&t->byte_max,4,1,f)!=1||fread(&t->best_reward,4,1,f)!=1) {
        free(t); fclose(f); return NULL;
    }
    fclose(f); return t;
}
