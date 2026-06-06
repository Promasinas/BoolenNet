/**
 * simple_llm.c — Simple Dialogue Model Implementation
 *
 * Network: Router(128b)→Memory(128c,decay=1)→Router(128b)→Memory(16c,output,decay=0)
 * Task: 5 fixed dialogue pairs, Tsetlin-trained Router weights
 * See: docs/interact/simple_llm_design.md
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../boolnet/boolnet.h"
#include "../bool_router/bool_router.h"
#include "../mem_int/mem_int_layer.h"
#include "../tsetlin_train/tsetlin_train.h"

#define V  16
#define M1 128
#define M2 16
#define STEPS 20000
#define BYTE_MAX (M2 * 255)

const char *sl_input_tokens[V] = {
    "hello","hi","hey","yo",
    "how are you","good morning","good evening","nice to meet",
    "whats up","tell me more","help","who are you",
    "bye","thanks","ok","see you"
};
const char *sl_output_tokens[V] = {
    "Hello!","Hi there!","Hey you!","Yo!",
    "Im doing well","Good morning!","Good evening!","Nice to meet you",
    "Not much!","Sure thing","How can I help?","Im BoolBot",
    "Goodbye!","Youre welcome!","Got it","Take care!"
};

const uint8_t sl_train_pairs[5][2] = {
    {0,  1},
    {4,  4},
    {11, 11},
    {12, 12},
    {13, 13}
};

static void make_onehot(uint8_t *buf, int pos, int len, uint8_t val) {
    memset(buf, 0, len); buf[pos % len] = val;
}
static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}
static int eval_pair(BoolNet *n, int in_idx, int out_idx, int *ok) {
    uint8_t in[V], out[M2];
    memset(in,0,V); memset(out,0,M2);
    make_onehot(in, in_idx, V, 255);
    reset_mem(n);
    int rc = boolnet_forward(n, in, out);
    int max_pos = 0;
    for (int i = 1; i < M2; i++) if (out[i] > out[max_pos]) max_pos = i;
    *ok = (max_pos == out_idx);
    (void)rc;
    return max_pos;
}

void* sl_build_network(void) {
    BoolNet *net = boolnet_create(1, V, 4);
    uint8_t zb[M1/8]; memset(zb, 0, sizeof(zb));

    BoolRouter *r1 = bool_router_create(1, M1, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, M1, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    BoolRouter *r2 = bool_router_create(3, M1, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, M2, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    return net;
}

int sl_train(void *vn, const char *model_path) {
    BoolNet *net = (BoolNet*)vn;
    TsetlinTrainer *t = tsetlin_create(net, 5000, BYTE_MAX);

    printf("Training %d steps on %d dialogue pairs...\n", STEPS, 5);
    for (int s = 0; s < STEPS; s++) {
        int p = s % 5;
        uint8_t in[V], tg[M2];
        make_onehot(in, sl_train_pairs[p][0], V, 255);
        make_onehot(tg, sl_train_pairs[p][1], M2, 255);
        int rc = tsetlin_train_step(t, in, tg);
        if (rc == -2) { printf("  Stop @ step %d\n", s+1); break; }

        if ((s+1) % 4000 == 0) {
            int correct = 0;
            for (int q = 0; q < 5; q++) {
                int ok; eval_pair(net, sl_train_pairs[q][0], sl_train_pairs[q][1], &ok);
                correct += ok;
            }
            uint32_t st,ac; int32_t br; uint32_t ep; int32_t bv;
            tsetlin_get_stats(t, &st, &ac, &br, &ep, &bv);
            printf("  Step %5d: acc=%d/5  ok=%u  best_r=%d\n", s+1, correct, ac, br);
        }
    }

    if (model_path) tsetlin_export_model(net, model_path);
    tsetlin_destroy(t);
    return 0;
}

int sl_infer(void *vn, int token_idx, int *out_idx, uint8_t *out_values) {
    BoolNet *net = (BoolNet*)vn;
    uint8_t in[V], out[M2];
    make_onehot(in, token_idx, V, 255);
    reset_mem(net);
    boolnet_forward(net, in, out);
    if (out_values) memcpy(out_values, out, M2);
    int max_pos = 0;
    for (int i = 1; i < M2; i++) if (out[i] > out[max_pos]) max_pos = i;
    if (out_idx) *out_idx = max_pos;
    return max_pos;
}

int sl_demo(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║     BoolNet Simple LLM - Fixed Dialogue     ║\n");
    printf("║  Router(128b)->Mem(128c)->Router(128b)->Mem(16c) ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    srand((unsigned)time(NULL));
    BoolNet *net = (BoolNet*)sl_build_network();
    printf("Network: %u layers, %u bytes input, %u bytes output\n",
           net->num_layers, V, M2);
    printf("Trainable: Router1(128b)+Router2(128b)=256 bits\n\n");

    printf("Before training:\n");
    int init_correct = 0;
    for (int p = 0; p < 5; p++) {
        int ok; int pred = eval_pair(net, sl_train_pairs[p][0], sl_train_pairs[p][1], &ok);
        printf("  \"%s\" -> pred=%d(\"%s\") exp=%d(\"%s\") %s\n",
               sl_input_tokens[sl_train_pairs[p][0]], pred, sl_output_tokens[pred],
               sl_train_pairs[p][1], sl_output_tokens[sl_train_pairs[p][1]],
               ok ? "OK" : "??");
        init_correct += ok;
    }
    printf("  Initial: %d/5 correct\n\n", init_correct);

    clock_t t0 = clock();
    sl_train(net, "weights/simple_llm_model.bin");
    printf("  Time: %.1f sec\n", (double)(clock()-t0)/CLOCKS_PER_SEC);

    printf("\n=== After training ===\n");
    int final_correct = 0;
    for (int p = 0; p < 5; p++) {
        int ok; int pred = eval_pair(net, sl_train_pairs[p][0], sl_train_pairs[p][1], &ok);
        printf("  %s  \"%s\" -> \"%s\"\n",
               ok ? "OK" : "??",
               sl_input_tokens[sl_train_pairs[p][0]],
               sl_output_tokens[pred]);
        final_correct += ok;
    }
    printf("  Final: %d/5 correct\n", final_correct);

    BoolRouter *r1 = NULL, *r2 = NULL;
    for (uint32_t i = 0; i < net->num_layers; i++) {
        if (net->layers[i].type == LAYER_ROUTER) {
            if (!r1) r1 = (BoolRouter*)net->layers[i].instance;
            else     r2 = (BoolRouter*)net->layers[i].instance;
        }
    }
    int flips = 0;
    if (r1) for (uint32_t i = 0; i < r1->num_bits; i++)
        if ((r1->bits[i/8]>>(i&7))&1) flips++;
    printf("\n  Router1 flipped: %d/128 bits\n", flips);
    flips = 0;
    if (r2) for (uint32_t i = 0; i < r2->num_bits; i++)
        if ((r2->bits[i/8]>>(i&7))&1) flips++;
    printf("  Router2 flipped: %d/128 bits\n", flips);
    printf("  Model: weights/simple_llm_model.bin\n");

    printf("\n=== Interactive Test ===\n");
    int test_inputs[] = {0, 4, 11, 12, 13};
    for (int i = 0; i < 5; i++) {
        int ti = test_inputs[i];
        int out_idx; uint8_t out_vals[M2];
        sl_infer(net, ti, &out_idx, out_vals);
        printf("  \"%s\" -> \"%s\" (cell[%d]=%d)\n",
               sl_input_tokens[ti], sl_output_tokens[out_idx], out_idx, out_vals[out_idx]);
    }

    boolnet_destroy(net);
    return (final_correct >= 3) ? 0 : 1;
}

int main(void) { printf("START\n"); fflush(stdout); int r = sl_demo(); printf("END rc=%d\n", r); return r; }
