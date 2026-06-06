#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tsetlin_train.h"
#include "../bool_router/bool_router.h"
#include "../conv1d_circular/conv1d_circular.h"
#include "../mem_int/mem_int_layer.h"

static void reset_memory(BoolNet *net) {
    for (uint32_t i = 0; i < net->num_layers; i++)
        if (net->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)net->layers[i].instance);
}

TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max) {
    if (!net) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1, sizeof(TsetlinTrainer));
    if (!t) return NULL;
    t->network = net; t->neg_tolerance = neg_tol; t->byte_max = byte_max;
    t->best_reward = -2147483648;
    return t;
}

void tsetlin_destroy(TsetlinTrainer *t) { free(t); }

/* Collect all trainable layers (Router + Conv1D) */
static uint32_t collect_trainable(BoolNet *net, uint32_t *indices, uint32_t *types) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < net->num_layers && n < 64; i++) {
        if (net->layers[i].type == LAYER_ROUTER ||
            net->layers[i].type == LAYER_CONV1D) {
            indices[n] = i;
            types[n] = net->layers[i].type;
            n++;
        }
    }
    return n;
}

/* Flip one bit in a trainable layer. Returns bit index or -1 on error. */
static int flip_layer_bit(BoolNetLayer *layer, uint32_t *bit_out) {
    if (layer->type == LAYER_ROUTER) {
        BoolRouter *r = (BoolRouter*)layer->instance;
        if (!r || !r->num_bits) return -1;
        uint32_t bit = (uint32_t)(rand() % r->num_bits);
        bool_router_flip_bit(r, bit);
        *bit_out = bit;
        return 0;
    } else if (layer->type == LAYER_CONV1D) {
        Conv1D *c = (Conv1D*)layer->instance;
        if (!c || !c->kernel_size) return -1;
        uint32_t bit = (uint32_t)(rand() % c->kernel_size);
        conv1d_flip_kernel_bit(c, bit);
        *bit_out = bit;
        return 0;
    }
    return -1;
}

/* Revert (re-flip) a bit. bit_out is the same bit index. */
static void revert_layer_bit(BoolNetLayer *layer, uint32_t bit) {
    if (layer->type == LAYER_ROUTER) {
        bool_router_flip_bit((BoolRouter*)layer->instance, bit);
    } else if (layer->type == LAYER_CONV1D) {
        conv1d_flip_kernel_bit((Conv1D*)layer->instance, bit);
    }
}

int tsetlin_train_step(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target) {
    if (!t || !t->network || !input || !target) return -1;
    uint32_t nl = t->network->num_layers;
    if (!nl) return -1;

    /* Collect trainable layers (Router + Conv1D) */
    uint32_t ti[64], ttypes[64];
    uint32_t nt = collect_trainable(t->network, ti, ttypes);
    if (!nt) return -1;

    /* Pick a random trainable layer */
    uint32_t pick = rand() % nt;
    uint32_t layer_idx = ti[pick];
    BoolNetLayer *layer = &t->network->layers[layer_idx];

    uint32_t od = t->network->output_dim;
    uint8_t *buf = (uint8_t*)malloc(od);
    if (!buf) return -2;

    /* Forward BEFORE flip */
    reset_memory(t->network);
    int rc = boolnet_forward(t->network, input, buf);
    if (rc) { free(buf); return rc; }

    int32_t r_before = (int32_t)t->byte_max;
    for (uint32_t i = 0; i < od; i++) {
        int d = (int)buf[i] - (int)target[i];
        r_before -= (d < 0 ? -d : d);
    }

    /* Flip a random bit in the chosen layer */
    uint32_t flipped_bit = 0;
    if (flip_layer_bit(layer, &flipped_bit) != 0) { free(buf); return -1; }

    /* Forward AFTER flip */
    reset_memory(t->network);
    rc = boolnet_forward(t->network, input, buf);
    if (rc) { revert_layer_bit(layer, flipped_bit); free(buf); return rc; }

    int32_t r_after = (int32_t)t->byte_max;
    for (uint32_t i = 0; i < od; i++) {
        int d = (int)buf[i] - (int)target[i];
        r_after -= (d < 0 ? -d : d);
    }

    /* Accept or revert */
    if (t->step_count < 5)
        printf('  [DEBUG seq] before=%d after=%d
', r_before, r_after);
    t->step_count++;
    if (r_after > r_before) {
        t->accept_count++;
        t->neg_counter = 0;
        if (r_after > t->best_reward) t->best_reward = r_after;
        free(buf);
        return 1;
    } else {
        revert_layer_bit(layer, flipped_bit);
        t->neg_counter++;
        free(buf);
        if (t->neg_tolerance > 0 && t->neg_counter >= t->neg_tolerance)
            return -2;
        return 0;
    }
}

/* Sequential training: feed token sequence without reset between tokens.
 * Compares final output after all tokens to target. */
int tsetlin_train_seq(TsetlinTrainer *t, const uint8_t **tokens, uint32_t seq_len,
                      const uint8_t *target) {
    if (!t || !t->network || !tokens || !target || !seq_len) return -1;

    uint32_t nt = 0;
    uint32_t ti[64], ttypes[64];
    nt = collect_trainable(t->network, ti, ttypes);
    if (!nt) return -1;

    uint32_t pick = rand() % nt;
    BoolNetLayer *layer = &t->network->layers[ti[pick]];

    uint32_t od = t->network->output_dim;
    uint8_t *buf = (uint8_t*)malloc(od);
    if (!buf) return -2;

    /* Sequential forward BEFORE flip (reset once, then all tokens) */
    reset_memory(t->network);
    for (uint32_t s = 0; s < seq_len; s++)
        boolnet_forward(t->network, tokens[s], buf);

    int32_t r_before = (int32_t)t->byte_max;
    for (uint32_t i = 0; i < od; i++) {
        int d = (int)buf[i] - (int)target[i];
        r_before -= (d < 0 ? -d : d);
    }

    /* Flip */
    uint32_t flipped_bit = 0;
    if (flip_layer_bit(layer, &flipped_bit) != 0) { free(buf); return -1; }

    /* Sequential forward AFTER flip */
    reset_memory(t->network);
    for (uint32_t s = 0; s < seq_len; s++)
        boolnet_forward(t->network, tokens[s], buf);

    int32_t r_after = (int32_t)t->byte_max;
    for (uint32_t i = 0; i < od; i++) {
        int d = (int)buf[i] - (int)target[i];
        r_after -= (d < 0 ? -d : d);
    }

    if (t->step_count < 5)
        printf('  [DEBUG seq] before=%d after=%d
', r_before, r_after);
    t->step_count++;
    if (r_after > r_before) {
        t->accept_count++;
        t->neg_counter = 0;
        if (r_after > t->best_reward) t->best_reward = r_after;
        free(buf);
        return 1;
    } else {
        revert_layer_bit(layer, flipped_bit);
        t->neg_counter++;
        free(buf);
        if (t->neg_tolerance > 0 && t->neg_counter >= t->neg_tolerance)
            return -2;
        return 0;
    }
}

int tsetlin_train_epochs(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target, uint32_t max) {
    if (!t || !input || !target) return -1;
    for (uint32_t e = 0; e < max; e++) {
        int rc = tsetlin_train_step(t, input, target);
        if (rc == -2) return 0;
        if (rc < -2) return rc;
    }
    return 0;
}

void tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *steps, uint32_t *accepted, int32_t *best_r) {
    if (steps)    *steps    = t ? t->step_count   : 0;
    if (accepted) *accepted = t ? t->accept_count : 0;
    if (best_r)   *best_r   = t ? t->best_reward  : -1;
}

int tsetlin_save(const TsetlinTrainer *t, const char *path) {
    if (!t || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    fwrite(&t->neg_tolerance,4,1,f); fwrite(&t->neg_counter,4,1,f);
    fwrite(&t->step_count,4,1,f);    fwrite(&t->accept_count,4,1,f);
    fwrite(&t->byte_max,4,1,f);      fwrite(&t->best_reward,4,1,f);
    fclose(f); return 0;
}

TsetlinTrainer* tsetlin_load(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1, sizeof(TsetlinTrainer));
    if (!t) { fclose(f); return NULL; }
    if (fread(&t->neg_tolerance,4,1,f)!=1||fread(&t->neg_counter,4,1,f)!=1||
        fread(&t->step_count,4,1,f)!=1||fread(&t->accept_count,4,1,f)!=1||
        fread(&t->byte_max,4,1,f)!=1||fread(&t->best_reward,4,1,f)!=1)
    { free(t); fclose(f); return NULL; }
    fclose(f); return t;
}
