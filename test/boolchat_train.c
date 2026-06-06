/**
 * boolchat_train.c — Coordinate-Descent Trained Dialogue Bot
 *
 * Architecture: Router(128b)→Memory(64c,decay=1)→Router(64b)→Memory(64c,output)
 * Training: Coordinate descent (deterministic, O(2N) per sweep)
 * Data: 12 Chinese-English dialogue pairs, auto-generated encoding
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/coord_descent.h"

/* ---- Config ---- */
#define N_IN    16    /* input bytes  */
#define N_HID   64    /* hidden memory cells */
#define N_OUT   64    /* output bytes */
#define N_PAIRS 12    /* dialogue pairs */
#define SWEEPS  5

/* ---- Encoding: text → fixed-size byte vector ---- */
static void encode(const char *s, uint8_t *v, int n) {
    memset(v, 0, n);
    int len = (int)strlen(s);
    for (int i = 0; i < len && i < n; i++)
        v[i] = (uint8_t)s[i];
    v[n-2] = (uint8_t)(len & 0xFF);
    v[n-1] = (uint8_t)((len >> 8) & 0xFF);
}

/* ---- Decode: byte vector → printable string ---- */
static void decode(const uint8_t *v, int n, char *out, int max) {
    int j = 0;
    for (int i = 0; i < n && j < max - 1; i++) {
        if (v[i] >= 32 && v[i] < 127) out[j++] = (char)v[i];
        else if (v[i] == 0) break;
    }
    out[j] = 0;
}

/* ---- Dialogue pairs ---- */
static const char *inputs[N_PAIRS] = {
    "hello", "hi", "how are you", "who are you",
    "what is boolnet", "bye", "thanks", "good morning",
    "good night", "help", "weather", "tell me a joke"
};
static const char *outputs[N_PAIRS] = {
    "Hello! Nice to meet you!",
    "Hi there! How can I help?",
    "I am doing great, thanks for asking!",
    "I am BoolBot, a Boolean network chatbot.",
    "BoolNet uses XOR routers and Tsetlin learning.",
    "Goodbye! Have a wonderful day!",
    "You are welcome! Glad to help.",
    "Good morning! Ready for a great day?",
    "Good night! Sleep well and dream big.",
    "Commands: just type and chat. I learn from context.",
    "I cannot check weather, but it is always sunny in BoolNet!",
    "Why did the Boolean cross the road? To XOR the other side!"
};

/* ---- Build network ---- */
static BoolNet* build_net(void) {
    BoolNet *net = boolnet_create(1, N_IN, 4);
    uint8_t zb[8] = {0};

    /* Layer 1: Router (128b) — learn input→hidden routing */
    BoolRouter *r1 = bool_router_create(1, N_IN * 8, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r1,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 2: Memory (64c, decay=1) — context/hidden state with decay */
    MemIntLayer *m1 = mem_int_create(2, ML_PRECISION_UINT8, N_HID, 255, 1);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m1,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    /* Layer 3: Router (64b) — learn hidden→output routing */
    BoolRouter *r2 = bool_router_create(3, N_OUT * 8, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 3, r2,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* Layer 4: Output Memory (64c, decay=0) — stable output */
    MemIntLayer *m2 = mem_int_create(4, ML_PRECISION_UINT8, N_OUT, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 4, m2,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    return net;
}

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

/* ---- Main ---- */
int main(void) {
    srand((unsigned)time(NULL));
    printf("==============================================\n");
    printf("  BoolChat — Coordinate Descent Trained Bot\n");
    printf("  Router(128b)->Mem(64c)->Router(64b)->Mem(64c)\n");
    printf("  %d dialogue pairs, %d sweeps max\n", N_PAIRS, SWEEPS);
    printf("==============================================\n\n");

    /* Encode all pairs */
    uint8_t qv[N_PAIRS][N_IN], av[N_PAIRS][N_OUT];
    for (int i = 0; i < N_PAIRS; i++) {
        encode(inputs[i],  qv[i], N_IN);
        encode(outputs[i], av[i], N_OUT);
    }

    /* Build & train */
    BoolNet *net = build_net();
    uint32_t trainable = N_IN * 8 + N_OUT * 8;  /* 128 + 64 = 192 bits */

    printf("Trainable: %u bits\n", trainable);
    printf("Training with Coordinate Descent...\n\n");

    clock_t t0 = clock();
    int total_sweeps = 0;

    for (int p = 0; p < N_PAIRS; p++) {
        int sw = coord_descent_train(net, qv[p], av[p],
                                      N_OUT * 255, SWEEPS);
        total_sweeps += sw;
        uint8_t out[N_OUT]; char txt[128];
        reset_mem(net); boolnet_forward(net, qv[p], out);
        decode(out, N_OUT, txt, sizeof(txt));
        printf("  [%2d] \"%s\" -> \"%s\"\n", p, inputs[p], txt);
    }

    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("\nTraining: %.0f ms, %d sweeps total (avg %.1f/pair)\n\n",
           sec * 1000, total_sweeps, (double)total_sweeps / N_PAIRS);

    /* Save model */
    tsetlin_export_model(net, "weights/boolchat_model.bin");
    printf("Model: weights/boolchat_model.bin\n");

    /* ---- Interactive ---- */
    printf("\n=== Interactive Chat ===\n");
    printf("Type a message (quit to exit):\n");

    char input[256];
    while (1) {
        printf("\nYou: "); fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit") || !strcmp(input, "exit")) {
            printf("BoolBot: Goodbye!\n"); break;
        }
        if (!strcmp(input, "list")) {
            for (int i = 0; i < N_PAIRS; i++)
                printf("  \"%s\"\n", inputs[i]);
            continue;
        }

        uint8_t vec[N_IN], out[N_OUT]; char reply[128];
        encode(input, vec, N_IN);
        reset_mem(net);
        boolnet_forward(net, vec, out);
        decode(out, N_OUT, reply, sizeof(reply));

        if (reply[0])
            printf("BoolBot: %s\n", reply);
        else
            printf("BoolBot: (binary output, try: hello/hi/who are you/bye/help)\n");
    }

    boolnet_destroy(net);
    return 0;
}
