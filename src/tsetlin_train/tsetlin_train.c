/**
 * tsetlin_train.c — Tsetlin Automaton Training Framework
 * Complete pipeline: step/seq training, full loop with validation,
 * checkpointing, early stopping, model export.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tsetlin_train.h"
#include "../bool_router/bool_router.h"
#include "../conv1d_circular/conv1d_circular.h"
#include "../mem_int/mem_int_layer.h"

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

static uint32_t collect_trainable(BoolNet *net, uint32_t *idx, uint32_t *types) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < net->num_layers && n < 64; i++) {
        if (net->layers[i].type == LAYER_ROUTER || net->layers[i].type == LAYER_CONV1D)
            { idx[n]=i; types[n]=net->layers[i].type; n++; }
    }
    return n;
}

static int flip_bit(BoolNetLayer *l, uint32_t *b) {
    if (l->type == LAYER_ROUTER) {
        BoolRouter *r = (BoolRouter*)l->instance;
        if (!r || !r->num_bits) return -1;
        *b = (uint32_t)(rand() % r->num_bits); bool_router_flip_bit(r, *b); return 0;
    } else if (l->type == LAYER_CONV1D) {
        Conv1D *c = (Conv1D*)l->instance;
        if (!c || !c->kernel_size) return -1;
        *b = (uint32_t)(rand() % c->kernel_size); conv1d_flip_kernel_bit(c, *b); return 0;
    }
    return -1;
}

static void revert_bit(BoolNetLayer *l, uint32_t b) {
    if (l->type == LAYER_ROUTER) bool_router_flip_bit((BoolRouter*)l->instance, b);
    else if (l->type == LAYER_CONV1D) conv1d_flip_kernel_bit((Conv1D*)l->instance, b);
}

static int32_t reward(const uint8_t *o, const uint8_t *t, uint32_t d, uint32_t bm) {
    int32_t r = (int32_t)bm;
    for (uint32_t i = 0; i < d; i++) { int df = (int)o[i]-(int)t[i]; r -= (df<0?-df:df); }
    return r;
}

TsetlinTrainer* tsetlin_create(BoolNet *net, uint32_t neg_tol, uint32_t byte_max) {
    if (!net) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1, sizeof(TsetlinTrainer));
    if (!t) return NULL;
    t->network = net; t->neg_tolerance = neg_tol; t->byte_max = byte_max;
    t->best_reward = -2147483648; t->best_val_acc = -1;
    return t;
}
void tsetlin_destroy(TsetlinTrainer *t) { free(t); }

int tsetlin_train_step(TsetlinTrainer *t, const uint8_t *input, const uint8_t *target) {
    if (!t || !t->network || !input || !target) return -1;
    uint32_t ti[64], tt[64]; uint32_t nt = collect_trainable(t->network, ti, tt);
    if (!nt) return -1;
    BoolNetLayer *l = &t->network->layers[ti[rand() % nt]];
    uint32_t od = t->network->output_dim;
    uint8_t *buf = (uint8_t*)malloc(od); if (!buf) return -2;

    reset_mem(t->network);
    int rc = boolnet_forward(t->network, input, buf);
    if (rc) { free(buf); return rc; }
    int32_t rb = reward(buf, target, od, t->byte_max);

    uint32_t fb = 0;
    if (flip_bit(l, &fb)) { free(buf); return -1; }

    reset_mem(t->network);
    rc = boolnet_forward(t->network, input, buf);
    if (rc) { revert_bit(l, fb); free(buf); return rc; }
    int32_t ra = reward(buf, target, od, t->byte_max);

    t->step_count++;
    if (ra > rb) { t->accept_count++; t->neg_counter=0; if(ra>t->best_reward)t->best_reward=ra; free(buf); return 1; }
    else { revert_bit(l, fb); if (ra < rb) t->neg_counter++; free(buf);
        if (t->neg_tolerance>0 && t->neg_counter>=t->neg_tolerance) return -2; return 0; }
}

int tsetlin_train_seq(TsetlinTrainer *t, const uint8_t **tokens, uint32_t len, const uint8_t *target) {
    if (!t || !t->network || !tokens || !target || !len) return -1;
    uint32_t ti[64], tt[64]; uint32_t nt = collect_trainable(t->network, ti, tt);
    if (!nt) return -1;
    BoolNetLayer *l = &t->network->layers[ti[rand() % nt]];
    uint32_t od = t->network->output_dim;
    uint8_t *buf = (uint8_t*)malloc(od); if (!buf) return -2;

    reset_mem(t->network);
    for (uint32_t s=0; s<len; s++) boolnet_forward(t->network, tokens[s], buf);
    int32_t rb = reward(buf, target, od, t->byte_max);

    uint32_t fb = 0; if (flip_bit(l, &fb)) { free(buf); return -1; }

    reset_mem(t->network);
    for (uint32_t s=0; s<len; s++) boolnet_forward(t->network, tokens[s], buf);
    int32_t ra = reward(buf, target, od, t->byte_max);

    t->step_count++;
    if (ra > rb) { t->accept_count++; t->neg_counter=0; if(ra>t->best_reward)t->best_reward=ra; free(buf); return 1; }
    else { revert_bit(l, fb); if (ra < rb) t->neg_counter++; free(buf);
        if (t->neg_tolerance>0 && t->neg_counter>=t->neg_tolerance) return -2; return 0; }
}

static int eval_acc(BoolNet *n, const uint8_t *in, const uint8_t *tg,
                    uint32_t num, uint32_t id, uint32_t od, void (*rst)(BoolNet*)) {
    int ok = 0; uint8_t *o = (uint8_t*)malloc(od); if (!o) return 0;
    for (uint32_t s=0; s<num; s++) {
        if (rst) rst(n); boolnet_forward(n, &in[s*id], o);
        if (!memcmp(o, &tg[s*od], od)) ok++;
    }
    free(o); return ok;
}

static void checkpoint(BoolNet *n, TsetlinTrainer *t, const char *dir, uint32_t ep) {
    char p[256]; snprintf(p,sizeof(p),"%s/ckpt_%04u.bin",dir,ep);
    FILE *f = fopen(p,"wb"); if(!f) return;
    fwrite(&ep,4,1,f); fwrite(&t->step_count,4,1,f); fwrite(&t->accept_count,4,1,f);
    fwrite(&t->best_reward,4,1,f);
    fwrite(&n->num_layers,4,1,f);
    for (uint32_t i=0; i<n->num_layers; i++) {
        fwrite(&n->layers[i].type,4,1,f);
        if (n->layers[i].type==LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)n->layers[i].instance;
            fwrite(&r->num_bits,4,1,f); fwrite(r->bits,1,(r->num_bits+7)/8,f);
        } else if (n->layers[i].type==LAYER_CONV1D) {
            Conv1D *c = (Conv1D*)n->layers[i].instance;
            fwrite(&c->kernel_size,4,1,f); fwrite(c->kernel_bits,1,(c->kernel_size+7)/8,f);
        }
    }
    fclose(f);
}

int tsetlin_train_full(TsetlinTrainer *t, const TrainConfig *cfg,
    const uint8_t *tr_in, const uint8_t *tr_tg, uint32_t n_tr,
    const uint8_t *vl_in, const uint8_t *vl_tg, uint32_t n_vl,
    uint32_t idim, uint32_t odim, void (*rst)(BoolNet*))
{
    if (!t || !cfg || !tr_in || !tr_tg || !n_tr) return -1;
    printf("\n=== BoolNet Training ===\n");
    printf("Epochs: %u  Steps/epoch: %u  Samples: %u  Val: %u\n\n",
           cfg->max_epochs, cfg->steps_per_epoch, n_tr, n_vl);

    int best_v = -1; uint32_t pat = 0;
    for (uint32_t ep = 1; ep <= cfg->max_epochs; ep++) {
        uint32_t ep_ok = 0;
        for (uint32_t s = 0; s < cfg->steps_per_epoch; s++) {
            uint32_t idx = rand() % n_tr;
            int rc = tsetlin_train_step(t, &tr_in[idx*idim], &tr_tg[idx*odim]);
            if (rc == 1) ep_ok++;
            if (rc == -2) { printf("  Stop: neg tolerance\n"); goto done; }
        }
        t->epoch_count++;

        int va = 0;
        if (n_vl > 0) va = eval_acc(t->network, vl_in, vl_tg, n_vl, idim, odim, rst);

        printf("  Epoch %4u | ok: %5u (%.0f%%)", ep, ep_ok, 100.0f*ep_ok/cfg->steps_per_epoch);
        if (n_vl > 0) {
            printf(" | val: %d/%u (%.0f%%)", va, n_vl, 100.0f*va/n_vl);
            if (va > best_v) { best_v = va; t->best_val_acc = best_v; pat = 0;
                if (cfg->checkpoint_dir) { char p[256]; snprintf(p,sizeof(p),"%s/best.bin",cfg->checkpoint_dir); tsetlin_export_model(t->network, p); }
                printf(" ★");
            } else pat++;
        }
        printf("\n");

        if (cfg->checkpoint_dir && cfg->checkpoint_every && (ep%cfg->checkpoint_every)==0)
            checkpoint(t->network, t, cfg->checkpoint_dir, ep);
        if (cfg->early_stop_patience && pat >= cfg->early_stop_patience)
            { printf("  Early stop\n"); break; }
    }
done:
    printf("\nDone: epochs=%u steps=%u ok=%u (%.0f%%) best_val=%d/%u\n",
           t->epoch_count, t->step_count, t->accept_count,
           t->step_count?100.0f*t->accept_count/t->step_count:0, best_v, n_vl);
    return best_v;
}

void tsetlin_get_stats(const TsetlinTrainer *t, uint32_t *st, uint32_t *ac,
                       int32_t *br, uint32_t *ep, int32_t *bv) {
    if (st) *st = t?t->step_count:0; if (ac) *ac = t?t->accept_count:0;
    if (br) *br = t?t->best_reward:-1; if (ep) *ep = t?t->epoch_count:0;
    if (bv) *bv = t?t->best_val_acc:-1;
}

int tsetlin_save(const TsetlinTrainer *t, const char *path) {
    if (!t || !path) return -1;
    FILE *f = fopen(path,"wb"); if(!f) return -2;
    fwrite(&t->neg_tolerance,4,1,f); fwrite(&t->neg_counter,4,1,f);
    fwrite(&t->step_count,4,1,f); fwrite(&t->accept_count,4,1,f);
    fwrite(&t->byte_max,4,1,f); fwrite(&t->best_reward,4,1,f);
    fwrite(&t->epoch_count,4,1,f); fwrite(&t->best_val_acc,4,1,f);
    fclose(f); return 0;
}

TsetlinTrainer* tsetlin_load(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path,"rb"); if(!f) return NULL;
    TsetlinTrainer *t = (TsetlinTrainer*)calloc(1,sizeof(TsetlinTrainer));
    if (!t) { fclose(f); return NULL; }
    if (fread(&t->neg_tolerance,4,1,f)!=1||fread(&t->neg_counter,4,1,f)!=1||
        fread(&t->step_count,4,1,f)!=1||fread(&t->accept_count,4,1,f)!=1||
        fread(&t->byte_max,4,1,f)!=1||fread(&t->best_reward,4,1,f)!=1||
        fread(&t->epoch_count,4,1,f)!=1||fread(&t->best_val_acc,4,1,f)!=1)
    { free(t); fclose(f); return NULL; }
    fclose(f); return t;
}

int tsetlin_export_model(BoolNet *net, const char *path) {
    if (!net || !path) return -1;
    FILE *f = fopen(path,"wb"); if(!f) return -2;
    uint32_t magic=0x424E4554, ver=1;
    fwrite(&magic,4,1,f); fwrite(&ver,4,1,f);
    fwrite(&net->num_layers,4,1,f); fwrite(&net->input_dim,4,1,f); fwrite(&net->output_dim,4,1,f);
    for (uint32_t i=0; i<net->num_layers; i++) {
        fwrite(&net->layers[i].type,4,1,f); fwrite(&net->layers[i].uid,4,1,f);
        if (net->layers[i].type==LAYER_ROUTER) {
            BoolRouter *r=(BoolRouter*)net->layers[i].instance;
            fwrite(&r->num_bits,4,1,f); fwrite(r->bits,1,(r->num_bits+7)/8,f);
        } else if (net->layers[i].type==LAYER_CONV1D) {
            Conv1D *c=(Conv1D*)net->layers[i].instance;
            fwrite(&c->input_length,4,1,f); fwrite(&c->kernel_size,4,1,f);
            fwrite(&c->stride,4,1,f); fwrite(&c->dilation,4,1,f);
            fwrite(c->kernel_bits,1,(c->kernel_size+7)/8,f);
        } else if (net->layers[i].type==LAYER_MEMORY) {
            MemIntLayer *m=(MemIntLayer*)net->layers[i].instance;
            static const uint32_t cs[]={1,2,4,8};
            uint32_t sz = (m->precision<4)?cs[m->precision]:1;
            fwrite(&m->precision,1,1,f); fwrite(&m->length,4,1,f);
            fwrite(&m->max_value,8,1,f); fwrite(&m->decay,8,1,f);
            fwrite(m->cells,sz,m->length,f);
        }
    }
    fclose(f); return 0;
}
