#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tsetlin_train.h"
#include "../bool_router/bool_router.h"
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

int tsetlin_train_step(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target) {
    if (!t || !t->network || !input || !target) return -1;
    uint32_t nl = t->network->num_layers;
    if (!nl) return -1;

    /* Find a router layer */
    uint32_t ri[64], nr = 0;
    for (uint32_t i = 0; i < nl && nr < 64; i++)
        if (t->network->layers[i].type == LAYER_ROUTER)
            ri[nr++] = i;
    if (!nr) return -1;

    uint32_t picked = ri[rand() % nr];
    BoolRouter *router = (BoolRouter*)t->network->layers[picked].instance;
    if (!router || !router->num_bits) return -1;

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

    /* Flip a random bit */
    uint32_t bit = (uint32_t)(rand() % router->num_bits);
    bool_router_flip_bit(router, bit);

    /* Forward AFTER flip */
    reset_memory(t->network);
    rc = boolnet_forward(t->network, input, buf);
    if (rc) { bool_router_flip_bit(router, bit); free(buf); return rc; }

    int32_t r_after = (int32_t)t->byte_max;
    for (uint32_t i = 0; i < od; i++) {
        int d = (int)buf[i] - (int)target[i];
        r_after -= (d < 0 ? -d : d);
    }

    /* Accept or revert */
    t->step_count++;
    if (r_after > r_before) {
        t->accept_count++;
        t->neg_counter = 0;
        if (r_after > t->best_reward) t->best_reward = r_after;
        free(buf);
        return 1;
    } else {
        bool_router_flip_bit(router, bit);
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
