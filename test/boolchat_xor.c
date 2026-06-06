/**
 * boolchat_xor.c — Coordinate-Descent Trained Dialogue Bot (24 pairs)
 *
 * Categories: greetings, identity, tech, small-talk, farewells, emotions.
 * Pure XOR router per pair, Hamming-distance matching at inference.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_BYTES  96
#define N_PAIRS  24

static const char *inputs[N_PAIRS] = {
    /* -- greetings (6) -- */
    "hello", "hi", "hey", "good morning",
    "good afternoon", "good evening",
    /* -- identity (4) -- */
    "who are you", "what is your name",
    "are you human", "who created you",
    /* -- tech (4) -- */
    "what is boolnet", "how does training work",
    "what is a router", "what is tsetlin",
    /* -- small-talk (4) -- */
    "how are you", "what can you do",
    "tell me a joke", "tell me something interesting",
    /* -- farewells (3) -- */
    "bye", "see you later", "good night",
    /* -- emotions (3) -- */
    "thanks", "sorry", "you are amazing"
};

static const char *outputs[N_PAIRS] = {
    /* -- greetings -- */
    "Hello! Nice to meet you! How can I help you today?",
    "Hi there! What would you like to talk about?",
    "Hey! Great to see you. What is on your mind?",
    "Good morning! Ready for a productive and wonderful day ahead?",
    "Good afternoon! Hope your day is going well so far.",
    "Good evening! The perfect time for some thoughtful conversation.",

    /* -- identity -- */
    "I am BoolBot, a Boolean neural network chatbot. I learn through XOR routing and coordinate descent optimization.",
    "My name is BoolBot. I was trained on dialogue pairs using pure Boolean operations.",
    "No, I am not human. I am a pure Boolean network running on XOR logic gates and memory triggers.",
    "I was created by the BoolNet framework. My training uses coordinate descent on binary router weights.",

    /* -- tech -- */
    "BoolNet is a pure Boolean neural network. No gradients, no floats. Just XOR routers and Tsetlin automata learning through bit-flip trials.",
    "Training works by coordinate descent: try flipping each router bit, keep it if the output gets closer to the target. No backpropagation needed.",
    "A Boolean router XORs input bits with stored weights. Bit=1 flips the input, bit=0 passes it unchanged. It is the core learning unit.",
    "Tsetlin automaton is a state-based learning machine. Each state decides an action, and feedback moves it between states. Pure logic, no math.",

    /* -- small-talk -- */
    "I am doing great! Every bit is in its optimal position. Thanks for asking!",
    "I can answer questions about BoolNet, chat about technology, tell jokes, and learn new dialogue patterns through training.",
    "Why did the Boolean cross the road? To XOR the other side! (I have more jokes if you want them!)",
    "Did you know? Boolean networks like me can learn without any floating point math. Every operation is pure logic gates!",

    /* -- farewells -- */
    "Goodbye! Have a wonderful day. Come back anytime for more Boolean chat!",
    "See you later! Your router weights will be waiting for you next time.",
    "Good night! Sleep well and dream of optimal bit configurations.",

    /* -- emotions -- */
    "You are welcome! I am happy to help. That is what BoolBots do best!",
    "No need to apologize! Communication is just XOR operations. Every interaction helps me learn.",
    "Thank you! I try my best. Positive feedback reinforces my optimal bit patterns!"
};

static void encode(const char *s, uint8_t *v) {
    memset(v, 0, N_BYTES);
    int len = (int)strlen(s);
    for (int i = 0; i < len && i < N_BYTES; i++) v[i] = (uint8_t)s[i];
    v[N_BYTES-2] = (uint8_t)(len & 0xFF);
    v[N_BYTES-1] = (uint8_t)((len >> 8) & 0xFF);
}
static void decode(const uint8_t *v, char *o, int m) {
    int j = 0;
    for (int i = 0; i < N_BYTES && j < m-1; i++) {
        if (v[i] >= 32 && v[i] < 127) o[j++] = (char)v[i];
        else if (!v[i]) break;
    }
    o[j] = 0;
}

static int coord_optimize(uint8_t *router, const uint8_t *in, const uint8_t *tg) {
    int changed = 1, sweeps = 0;
    while (changed && sweeps < 5) {
        changed = 0; sweeps++;
        for (int byte = 0; byte < N_BYTES; byte++) {
            for (int bit = 0; bit < 8; bit++) {
                uint8_t mask = (uint8_t)(1u << bit);
                int before = 0, after = 0;
                for (int i = 0; i < N_BYTES; i++) {
                    uint8_t x = (in[i] ^ router[i]) ^ tg[i];
                    while (x) { before += x & 1; x >>= 1; }
                }
                router[byte] ^= mask;
                for (int i = 0; i < N_BYTES; i++) {
                    uint8_t x = (in[i] ^ router[i]) ^ tg[i];
                    while (x) { after += x & 1; x >>= 1; }
                }
                if (after >= before) { router[byte] ^= mask; }
                else { changed++; }
            }
        }
    }
    return sweeps;
}

int main(void) {
    srand((unsigned)time(NULL));
    printf("================================================\n");
    printf("  BoolChat XOR — 24 Dialogue Pairs\n");
    printf("  %d bytes per message, Coordinate Descent\n", N_BYTES);
    printf("  Categories: greetings|identity|tech|chat|bye|emo\n");
    printf("================================================\n\n");

    uint8_t qv[N_PAIRS][N_BYTES], av[N_PAIRS][N_BYTES], routers[N_PAIRS][N_BYTES];
    for (int i = 0; i < N_PAIRS; i++) {
        encode(inputs[i], qv[i]);
        encode(outputs[i], av[i]);
        memset(routers[i], 0, N_BYTES);
    }

    clock_t t0 = clock();
    int total_sw = 0, perfect = 0;

    for (int p = 0; p < N_PAIRS; p++) {
        int sw = coord_optimize(routers[p], qv[p], av[p]);
        total_sw += sw;
        uint8_t out[N_BYTES];
        for (int i = 0; i < N_BYTES; i++) out[i] = qv[p][i] ^ routers[p][i];
        char txt[160]; decode(out, txt, sizeof(txt));
        int dist = 0;
        for (int i = 0; i < N_BYTES; i++) {
            uint8_t x = out[i] ^ av[p][i];
            while (x) { dist += x & 1; x >>= 1; }
        }
        if (dist == 0) perfect++;
        const char *cats[] = {"greet","greet","greet","greet","greet","greet",
            "ident","ident","ident","ident",
            "tech","tech","tech","tech",
            "chat","chat","chat","chat",
            "bye","bye","bye",
            "emo","emo","emo"};
        printf("  [%s] \"%s\" -> \"%s\" %s\n",
               cats[p], inputs[p],
               dist == 0 ? txt : "PARTIAL",
               dist == 0 ? "PERFECT" : "");
    }
    printf("\n%d ms, %d sweeps, %d/%d PERFECT\n\n",
           (int)((clock()-t0)*1000/CLOCKS_PER_SEC), total_sw, perfect, N_PAIRS);

    printf("=== Chat (quit/list) ===\n");
    char in[256];
    while (1) {
        printf("\nYou: "); fflush(stdout);
        if (!fgets(in, sizeof(in), stdin)) break;
        in[strcspn(in, "\n")] = 0;
        if (!in[0]) continue;
        if (!strcmp(in, "quit") || !strcmp(in, "exit")) {
            printf("BoolBot: Goodbye!\n"); break;
        }
        if (!strcmp(in, "list")) {
            printf("I know these topics:\n");
            for (int i = 0; i < N_PAIRS; i++)
                printf("  [%s] %s\n",
                       (const char*[]){"greet","greet","greet","greet","greet","greet",
                        "ident","ident","ident","ident",
                        "tech","tech","tech","tech",
                        "chat","chat","chat","chat",
                        "bye","bye","bye",
                        "emo","emo","emo"}[i], inputs[i]);
            continue;
        }

        uint8_t v[N_BYTES]; encode(in, v);
        int bp = 0, bd = 999999;
        for (int p = 0; p < N_PAIRS; p++) {
            int d = 0;
            for (int i = 0; i < N_BYTES; i++) {
                uint8_t x = v[i] ^ qv[p][i];
                while (x) { d += x & 1; x >>= 1; }
            }
            if (d < bd) { bd = d; bp = p; }
        }
        uint8_t o[N_BYTES];
        for (int i = 0; i < N_BYTES; i++) o[i] = v[i] ^ routers[bp][i];
        char reply[200]; decode(o, reply, sizeof(reply));
        int threshold = N_BYTES * 8 / 4; /* allow up to 25% bit difference */
        printf("BoolBot: %s\n",
               (bd < threshold && reply[0]) ? reply :
               "I am not sure about that. Try: hello, who are you, what is boolnet, tell me a joke, thanks, bye...");
    }
    return 0;
}
