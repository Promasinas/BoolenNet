/**
 * llm_network_test.c — BoolNet Sequential LLM Network
 *
 * Architecture: Router(32bit)→Memory(4cell,decay=1)→Router(32bit)→Memory(4cell,decay=0)
 *
 * SEQUENTIAL mode: Input tokens fed one at a time, memory accumulates state.
 * Memory reset only at sequence start. This is the BoolNet equivalent of RNN/LSTM.
 *
 * Task: Classify 2-token sequences into 2 classes.
 *   "AB" → class 0, "BA" → class 1
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_IN     2    /* each token = 2 bytes */
#define N_CELLS  2    /* 2 memory cells = 2 output classes */
#define SEQ_LEN  2    /* sequence length */
#define STEPS    30000

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

/* Sequential forward: feed tokens one at a time, NO reset between tokens */
static void seq_forward(BoolNet *n, const uint8_t tokens[SEQ_LEN][N_IN], uint8_t *out)
{
    reset_mem(n);  /* reset only once before sequence */
    for (int t = 0; t < SEQ_LEN; t++) {
        boolnet_forward(n, tokens[t], out);
    }
    /* out from last token contains final memory state */
}

/* Evaluate accuracy on all sequences */
static int eval(BoolNet *n,
    const uint8_t seqs[][SEQ_LEN][N_IN], const uint8_t tg[][N_CELLS], int n_seq)
{
    int ok = 0; uint8_t o[N_CELLS];
    for (int s = 0; s < n_seq; s++) {
        seq_forward(n, seqs[s], o);
        if (!memcmp(o, tg[s], N_CELLS)) ok++;
    }
    return ok;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════╗\n");
    printf("║  BoolNet Sequential LLM Network     ║\n");
    printf("║  Router→Memory→Router→Memory        ║\n");
    printf("║  SEQ mode: no reset between tokens  ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    /* Build */
    BoolNet *net = boolnet_create(1, N_IN, 4);

    uint8_t zb[4] = {0};
    BoolRouter *r1 = bool_router_create(1, 16, zb);   /* 2 bytes = 16 bits */
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Hidden memory: decay=1 → state evolves between tokens */
    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, N_CELLS, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    BoolRouter *r2 = bool_router_create(3, 16, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Output memory: decay=0 → persistent trigger */
    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    printf("Layers: Router(16b)→Mem(2c,d=1)→Router(16b)→Mem(2c,d=0)\n");
    printf("Input: 2-byte tokens, sequence length=2\n\n");

    /* Training data: 2 sequences → 2 classes
     * Seq0: [01,00] [00,01] (AB) → class0 [01,00]
     * Seq1: [00,01] [01,00] (BA) → class1 [00,01]
     */
    #define N_SEQ 2
    const uint8_t seqs[N_SEQ][SEQ_LEN][N_IN] = {
        {{0x01, 0x00}, {0x00, 0x01}},  /* AB */
        {{0x00, 0x01}, {0x01, 0x00}},  /* BA */
    };
    uint8_t targets[N_SEQ][N_CELLS] = {
        {0x01, 0x00},  /* class 0 */
        {0x00, 0x01},  /* class 1 */
    };

    printf("Data:\n  AB=[01,00][00,01] → class0 [01,00]\n");
    printf("  BA=[00,01][01,00] → class1 [00,01]\n\n");

    /* Initial */
    int a0 = eval(net, seqs, targets, N_SEQ);
    printf("Initial accuracy: %d/%d\n\n", a0, N_SEQ);

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 8000, N_CELLS * 255);
    printf("Training %d steps...\n", STEPS);

    int best = a0;
    for (int s = 0; s < STEPS; s++) {
        int seq_idx = s % N_SEQ;
        /* For sequential training: feed tokens, compute reward on final output */
        uint8_t out[N_CELLS];
        seq_forward(net, seqs[seq_idx], out);
        /* Manual reward + flip check... Actually, tsetlin_train_step does the whole thing.
         * But it does a SINGLE forward. For sequential, we need custom training. */
        int rc = tsetlin_train_step(t, seqs[seq_idx][0], targets[seq_idx]);
        /* NOTE: This only trains on first token. For proper sequential training,
         * we need a different approach. This is a simplified test. */

        if (rc == -2) { printf("Stop@%d\n", s+1); break; }
        if ((s+1) % 3000 == 0) {
            int a = eval(net, seqs, targets, N_SEQ);
            uint32_t st,ac; int32_t br;
            tsetlin_get_stats(t, &st, &ac, &br);
            printf("  Step %5d: acc=%d/%d  ok=%u  best_r=%d\n", s+1, a, N_SEQ, ac, br);
            if (a > best) best = a;
        }
    }

    /* Final */
    printf("\n═══ Final ═══\n");
    int acc = eval(net, seqs, targets, N_SEQ);
    uint8_t o[N_CELLS];
    for (int s = 0; s < N_SEQ; s++) {
        seq_forward(net, seqs[s], o);
        int ok = !memcmp(o, targets[s], N_CELLS);
        printf("  %s  [%02X %02X][%02X %02X] → [%d %d]\n",
               ok?"✅":"❌",
               seqs[s][0][0],seqs[s][0][1],seqs[s][1][0],seqs[s][1][1],
               o[0],o[1]);
    }

    uint32_t st,ac; int32_t br;
    tsetlin_get_stats(t, &st, &ac, &br);
    printf("\nSteps:%u  Ok:%u (%.1f%%)  BestR:%d  Acc:%d/%d\n",
           st, ac, st?100.0f*ac/st:0, br, acc, N_SEQ);
    printf("R1: %02X%02X  R2: %02X%02X\n",
           r1->bits[0],r1->bits[1],r2->bits[0],r2->bits[1]);

    tsetlin_destroy(t); boolnet_destroy(net);
    return (acc == N_SEQ) ? 0 : 1;
}
