/**
 * chatbot_bytestream.c — 纯 byte 流对话机器人
 *
 * 不是分类! 输入 bytes → 网络实时计算 → 输出 bytes → 直接是回答文本。
 * 每个输出 byte 就是一个字符。无查表，无 class index。
 *
 * 架构: 文本编码(64B) → Router XOR(512bit) → Memory(64c, output cell values)
 * 训练: 输入编码 → 目标输出编码 → Tsetlin 最小化逐字节差异
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 64  /* 64 bytes in, 64 bytes out */

/* Q&A 数据 */
static const char *questions[] = {
    "hello", "who are you", "weather", "time",
    "what is boolnet", "how to train", "goodbye", "thanks",
    "what can you do", "who made you", "tsetlin", "router",
    "save model", "load weights", "your name", "help"
};
static const char *answers[] = {
    "Hello! How can I help you?",
    "I am BoolBot, a Boolean router cascade chatbot.",
    "Sorry, I cannot access real-time weather.",
    "Please check your system clock.",
    "BoolNet is a pure Boolean neural network framework.",
    "Use Tsetlin training: flip bit, forward, reward, keep or revert.",
    "Goodbye! Looking forward to our next chat.",
    "You're welcome! Feel free to ask anything.",
    "I answer questions about BoolNet and simple conversations.",
    "I was created by the BoolNet framework user.",
    "Tsetlin automaton: state-based learning by bit-flip optimization.",
    "Boolean router: bit=1 flips input, bit=0 passes unchanged.",
    "Use tsetlin_export_model() to save to weights/ directory.",
    "Use mem_int_load() from weights/ to load.",
    "My name is BoolBot, a cascade Boolean routing tree chatbot.",
    "Commands: ask, list to see all, quit to exit."
};
#define N_QA 16

/* 编码: 文本 → N 字节 */
static void encode(const char *txt, unsigned char *v)
{
    memset(v, 0, N);
    int len = (int)strlen(txt);
    if (!len) { v[0] = 0xFF; return; }
    for (int i = 0; i < len && i < N; i++)
        v[i] = (unsigned char)txt[i];
    /* 长度标记 */
    v[N-2] = (unsigned char)(len & 0xFF);
    v[N-1] = (unsigned char)((len>>8) & 0xFF);
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet Byte-Stream Chatbot            ║\n");
    printf("║  Input bytes → Network → Output bytes   ║\n");
    printf("║  NO classification, NO lookup table     ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 网络: Router(512bit XOR) → Memory(64c, output cell values) */
    unsigned char rbits[N];  /* router bits */
    for (int i = 0; i < N; i++) rbits[i] = (unsigned char)(rand() & 0xFF);
    unsigned char cells[N];  /* memory cells */

    /* 编码所有 Q&A */
    unsigned char qv[N_QA][N], av[N_QA][N];
    for (int i = 0; i < N_QA; i++) {
        encode(questions[i], qv[i]);
        encode(answers[i], av[i]);
    }

    printf("网络: Router(512bit) → Memory(64 cells, output values)\n");
    printf("训练数据: %d 对 (question bytes → answer bytes)\n", N_QA);
    printf("训练目标: 最小化 |output[i] - target[i]| 逐字节差异\n\n");

    /* ===== 训练 ===== */
    #define MAX_STEPS 2000000
    int accept = 0, steps = 0;
    int best_dist = 999999;

    printf("=== 训练 (max %d 步) ===\n", MAX_STEPS);

    for (steps = 0; steps < MAX_STEPS; steps++) {
        int qi = steps % N_QA;

        /* 当前网络输出 */
        memset(cells, 0, N);
        unsigned char out[N];
        for (int i = 0; i < N; i++) {
            unsigned char sig = qv[qi][i] ^ rbits[i];
            cells[i] = sig;  /* 直接存 XOR 结果, 非二值 */
            out[i] = cells[i];
        }

        /* 逐字节距离 */
        int dist_before = 0;
        for (int i = 0; i < N; i++) {
            int d = (int)out[i] - (int)av[qi][i];
            dist_before += (d < 0 ? -d : d);
        }

        /* 翻一个 bit */
        int fb = rand() % (N * 8);
        rbits[fb/8] ^= (unsigned char)(1u << (fb % 8));

        /* 翻转后输出 */
        memset(cells, 0, N);
        for (int i = 0; i < N; i++) {
            unsigned char sig = qv[qi][i] ^ rbits[i];
            cells[i] = sig;  /* 直接存 XOR 结果, 非二值 */
            out[i] = cells[i];
        }

        int dist_after = 0;
        for (int i = 0; i < N; i++) {
            int d = (int)out[i] - (int)av[qi][i];
            dist_after += (d < 0 ? -d : d);
        }

        if (dist_after < dist_before) {
            accept++;
            if (dist_after < best_dist) best_dist = dist_after;
        } else {
            rbits[fb/8] ^= (unsigned char)(1u << (fb % 8)); /* revert */
        }

        /* 每 200K 步报告 */
        if ((steps+1) % 200000 == 0) {
            /* 快速评估: 平均距离 */
            long total_dist = 0;
            for (int p = 0; p < N_QA; p++) {
                memset(cells, 0, N);
                for (int i = 0; i < N; i++) {
                    if (qv[p][i] ^ rbits[i]) cells[i] = 255;
                }
                for (int i = 0; i < N; i++) {
                    int d = (int)cells[i] - (int)av[p][i];
                    total_dist += (d < 0 ? -d : d);
                }
            }
            printf("  步 %7d: 平均距离 %.0f  接受 %d (%.2f%%)  最佳距离 %d\n",
                   steps+1, (double)total_dist/N_QA, accept,
                   100.0*accept/(steps+1), best_dist);
        }
    }

    /* ===== 最终评估 ===== */
    printf("\n═══ 训练完成 ═══\n");
    printf("步数: %d  接受: %d (%.2f%%)\n\n", steps, accept, 100.0*accept/steps);

    printf("═══ 网络输出 vs 期望 ═══\n");
    for (int p = 0; p < N_QA; p++) {
        memset(cells, 0, N);
        unsigned char out[N];
        for (int i = 0; i < N; i++) {
            if (qv[p][i] ^ rbits[i]) cells[i] = 255;
            out[i] = cells[i];
        }

        /* 将输出字节转为可读字符串 */
        char out_str[N+1];
        int pos = 0;
        for (int i = 0; i < N && out[i]; i++) {
            if (out[i] >= 32 && out[i] < 127)
                out_str[pos++] = (char)out[i];
        }
        out_str[pos] = 0;

        printf("  Q: %-16s\n", questions[p]);
        printf("    期望: %s\n", answers[p]);
        printf("    输出: %s\n", out_str[0] ? out_str : "(空)");
        printf("\n");
    }

    /* ===== 交互 ===== */
    printf("═══ 实时对话 ═══\n");
    printf("(网络实时计算 byte 流输出)\n");
    printf("输入问题或 'quit':\n");

    char input[256];
    while (1) {
        printf("\n> "); fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit")) { printf("再见!\n"); break; }

        unsigned char vec[N], out[N];
        encode(input, vec);
        memset(cells, 0, N);
        for (int i = 0; i < N; i++) {
            if (vec[i] ^ rbits[i]) cells[i] = 255;
            out[i] = cells[i];
        }

        /* 直接输出 byte 流 */
        printf("网络输出(bytes): ");
        for (int i = 0; i < 32; i++) printf("%02X ", out[i]);
        printf("\n");

        /* 可读文本 */
        char text[N+1]; int tp = 0;
        for (int i = 0; i < N && out[i]; i++)
            if (out[i] >= 32 && out[i] < 127)
                text[tp++] = (char)out[i];
        text[tp] = 0;
        printf("可读文本: %s\n", text[0] ? text : "(二进制)");
    }
    return 0;
}
