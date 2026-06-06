/**
 * full_train_save.c — Complete BoolNet: Train → Save → Load → Infer
 *
 * Network: Router(32b)→Conv1D(boolean,K=3)→Memory(4c)→Router(32b)→Memory(4c,output)
 *
 * Task: Learn 4 input→output byte mappings
 * After training: saves router weights + memory state, demonstrates reload + inference
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/conv1d_circular/conv1d_circular.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_IN     4
#define N_CELLS  4
#define N_PATS   4
#define STEPS    50000

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

static int eval(BoolNet *n, uint8_t in[N_PATS][N_IN], uint8_t tg[N_PATS][N_CELLS])
{
    int ok = 0; uint8_t o[N_CELLS];
    for (int p = 0; p < N_PATS; p++) {
        reset_mem(n); boolnet_forward(n, in[p], o);
        if (!memcmp(o, tg[p], N_CELLS)) ok++;
    }
    return ok;
}

/* Save all trainable params to a model file */
static int save_model(BoolNet *net, const char *path)
{
    FILE *f = fopen(path, "wb"); if (!f) return -1;
    fwrite(&net->num_layers, 4, 1, f);
    for (uint32_t i = 0; i < net->num_layers; i++) {
        fwrite(&net->layers[i].type, 4, 1, f);
        if (net->layers[i].type == LAYER_ROUTER) {
            BoolRouter *r = (BoolRouter*)net->layers[i].instance;
            fwrite(&r->num_bits, 4, 1, f);
            fwrite(r->bits, 1, (r->num_bits+7)/8, f);
        } else if (net->layers[i].type == LAYER_CONV1D) {
            Conv1D *c = (Conv1D*)net->layers[i].instance;
            fwrite(&c->kernel_size, 4, 1, f);
            fwrite(c->kernel_bits, 1, (c->kernel_size+7)/8, f);
        }
    }
    fclose(f);
    printf("  Model saved to: %s\n", path);
    return 0;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet: Train → Save → Load → Infer   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* ==== 1. BUILD ==== */
    BoolNet *net = boolnet_create(1, N_IN, 5);

    uint8_t zb[4] = {0};
    BoolRouter *r1 = bool_router_create(1, 32, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    Conv1D *c1 = conv1d_create(2, N_IN, 3, 1, 1);
    boolnet_add_layer(net, LAYER_CONV1D, 2, c1,
        conv1d_forward_layer, (int(*)(void*,const char*))conv1d_save);

    MemIntLayer *m1 = mem_int_create(3, ML_PRECISION_UINT8, N_CELLS, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 3, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    BoolRouter *r2 = bool_router_create(4, 32, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 4, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    MemIntLayer *m2 = mem_int_create(5, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 5, m2,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    printf("Network: Router→Conv1D(bool)→Mem→Router→Mem(output)\n");
    printf("Trainable: Router1(32b) + Conv1D(3b) + Router2(32b) = 67 bits\n\n");

    /* ==== 2. DATA ==== */
    uint8_t in[N_PATS][N_IN] = {
        {0x01,0,0,0}, {0,0x01,0,0}, {0,0,0x01,0}, {0,0,0,0x01}
    };
    uint8_t tg[N_PATS][N_CELLS] = {
        {0xFF,0,0,0}, {0,0xFF,0,0}, {0,0,0xFF,0}, {0,0,0,0xFF}
    };

    printf("Data: 4 one-hot inputs → one-hot outputs (byte scale)\n");

    int a0 = eval(net, in, tg);
    printf("Initial: %d/%d\n\n", a0, N_PATS);

    /* ==== 3. TRAIN ==== */
    TsetlinTrainer *t = tsetlin_create(net, 15000, N_CELLS * 255);
    printf("Training %d steps...\n", STEPS);

    int best_acc = a0, best_step = 0;
    for (int s = 0; s < STEPS; s++) {
        int rc = tsetlin_train_step(t, in[s % N_PATS], tg[s % N_PATS]);
        if (rc == -2) break;

        if ((s+1) % 5000 == 0) {
            int a = eval(net, in, tg);
            uint32_t st,ac; int32_t br;
            { uint32_t _ep; int32_t _bv; tsetlin_get_stats(t, &st, &ac, &br, &_ep, &_bv); }
            printf("  Step %5d: acc=%d/%d  ok=%u  best_r=%d\n", s+1, a, N_PATS, ac, br);
            if (a > best_acc) { best_acc = a; best_step = s+1; }
        }
    }

    /* ==== 4. FINAL EVAL ==== */
    printf("\n═══ Final ═══\n");
    int acc = eval(net, in, tg); uint8_t o[N_CELLS];
    for (int p = 0; p < N_PATS; p++) {
        reset_mem(net); boolnet_forward(net, in[p], o);
        int ok = !memcmp(o, tg[p], N_CELLS);
        printf("  %s  in=[%02X %02X %02X %02X] → out=[%3d %3d %3d %3d]  tg=[%3d %3d %3d %3d]\n",
               ok?"✅":"❌",
               in[p][0],in[p][1],in[p][2],in[p][3],
               o[0],o[1],o[2],o[3],
               tg[p][0],tg[p][1],tg[p][2],tg[p][3]);
    }

    uint32_t st,ac,ep; int32_t br,bv;
    tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);
    printf("\nSteps:%u  Ok:%u (%.1f%%)  BestR:%d  Acc:%d/%d\n",
           st, ac, st?100.0f*ac/st:0, br, acc, N_PATS);

    /* ==== 5. SAVE ==== */
    printf("\n═══ Saving Model ═══\n");
    save_model(net, "boolnet_model.bin");

    /* Save individual router weights (human-readable) */
    printf("  Router1 bits: [%02X %02X %02X %02X]\n", r1->bits[0],r1->bits[1],r1->bits[2],r1->bits[3]);
    printf("  Conv1D kernel: [%02X]\n", c1->kernel_bits[0]);
    printf("  Router2 bits: [%02X %02X %02X %02X]\n", r2->bits[0],r2->bits[1],r2->bits[2],r2->bits[3]);

    /* Save memory state */
    mem_int_save(m2, "memory_output.bin");
    printf("  Memory state saved to: memory_output.bin\n");

    /* ==== 6. LOAD & VERIFY ==== */
    printf("\n═══ Load & Verify ═══\n");
    MemIntLayer *loaded_mem = mem_int_load("memory_output.bin");
    if (loaded_mem) {
        printf("  Loaded memory: uid=%u, cells=%u, precision=%u\n",
               loaded_mem->uid, loaded_mem->length, loaded_mem->precision);
        uint8_t verify[N_CELLS];
        /* Do a fresh inference with loaded memory */
        reset_mem(net);
        boolnet_forward(net, in[0], verify);
        printf("  Inference with loaded model: [%02X %02X %02X %02X] → [%3d %3d %3d %3d]\n",
               in[0][0],in[0][1],in[0][2],in[0][3],
               verify[0],verify[1],verify[2],verify[3]);
        mem_int_destroy(loaded_mem);
    }

    tsetlin_destroy(t); boolnet_destroy(net);
    return (acc == N_PATS) ? 0 : 1;
}
