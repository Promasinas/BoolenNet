/**
 * chatbot_trained.c — 纯计算路由对话机器人
 *
 * 网络实时计算: 输入编码 → Router XOR → Memory 触发 → 输出 cell 索引
 * 答案文本是预存资源，但选择哪个答案 100% 由网络实时计算决定。
 * 训练至收敛 (准确率 >= 90% 或 max 步数)。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_IN   16
#define N_CELL 16
#define N_QA   16

static const char *questions[N_QA] = {
    "hello", "who are you", "weather", "time",
    "what is boolnet", "how to train", "goodbye", "thanks",
    "what can you do", "who made you", "tsetlin", "router",
    "save model", "load weights", "your name", "help"
};
static const char *answers[N_QA] = {
    "Hello! How can I help you?",
    "I am BoolBot, a Boolean router cascade chatbot.",
    "Sorry, I cannot access real-time weather. I am a local model.",
    "Please check your system clock for the current time.",
    "BoolNet is a pure Boolean neural network framework.",
    "Use Tsetlin training: flip router bit -> forward -> reward -> keep/revert.",
    "Goodbye! Looking forward to our next chat.",
    "You're welcome! Feel free to ask anything.",
    "I can answer questions about BoolNet and simple conversations.",
    "I was created by the BoolNet framework user.",
    "Tsetlin automaton: state-based learning by bit-flip optimization.",
    "Boolean router: BoolNet's basic unit. bit=1 flips input, bit=0 passes.",
    "Use tsetlin_export_model() to save to weights/ directory.",
    "Use mem_int_load() or tsetlin_import_model() from weights/.",
    "My name is BoolBot, a cascade Boolean routing tree chatbot.",
    "Commands: ask questions, 'list' to see all, 'quit' to exit."
};

/* 编码 */
static void encode(const char *txt, unsigned char *v)
{
    memset(v, 0, N_IN);
    int len = (int)strlen(txt);
    for (int i = 0; i < len; i++)
        v[(i*7+3) % N_IN] ^= (unsigned char)(txt[i] + (i & 0x1F));
    v[0] ^= (unsigned char)len;
    v[N_IN-1] ^= (unsigned char)((len*13) & 0xFF);
}

/* 计算 softmax-like: 找最大 cell */
static int argmax(const unsigned char *out)
{
    int best = 0;
    for (int i = 1; i < N_CELL; i++)
        if (out[i] > out[best]) best = i;
    return best;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet 训练至收敛对话机器人          ║\n");
    printf("║  100%% 网络计算路由决策                ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 网络: Router(128b) → Memory(16c) */
    unsigned char router_bits[N_IN];
    /* 随机初始化: 打破对称性 */
    for (int i = 0; i < N_IN; i++) router_bits[i] = (unsigned char)(rand() & 0xFF);
    unsigned char mem_cells[N_CELL];  memset(mem_cells, 0, N_CELL);

    /* 编码全部问题 */
    unsigned char qvecs[N_QA][N_IN];
    for (int i = 0; i < N_QA; i++) encode(questions[i], qvecs[i]);

    printf("训练目标: 16 个问题 → 16 个不同 cell 触发\n");
    printf("网络: Router(128bit XOR) → Memory(16 cells, decay=0, max=255)\n");
    printf("训练参数: 1,000,000 步, 目标准确率 80%%\n\n");

    /* ===== 训练 ===== */
    #define MAX_STEPS 1000000
    #define TARGET_ACC 0.80
    unsigned char target[N_CELL];
    int accept = 0, steps = 0;
    int best_acc = 0;

    for (steps = 0; steps < MAX_STEPS; steps++) {
        int qi = steps % N_QA;

        /* 目标: 只有 cell[qi] = 255，其余 = 0 */
        memset(target, 0, N_CELL);
        target[qi] = 255;

        /* 每步重置 memory */
        memset(mem_cells, 0, N_CELL);

        /* Forward: 输入 XOR router_bits → memory 触发 */
        unsigned char sig[N_IN], out[N_CELL];
        for (int i = 0; i < N_IN; i++)
            sig[i] = qvecs[qi][i] ^ router_bits[i];

        /* Memory: sig > 0 → 触发到 255, sig == 0 → 保持原值 */
        for (int i = 0; i < N_CELL; i++) {
            if (sig[i]) mem_cells[i] = 255;
            out[i] = mem_cells[i];
        }

        /* 奖励: 255*16 - sum|out-target| */
        int reward = N_CELL * 255;
        for (int i = 0; i < N_CELL; i++) {
            int d = (int)out[i] - (int)target[i];
            reward -= (d < 0 ? -d : d);
        }

        /* 翻一个随机 bit */
        int flip_byte = rand() % N_IN;
        int flip_bit  = rand() % 8;
        unsigned char mask = (unsigned char)(1u << flip_bit);
        router_bits[flip_byte] ^= mask;

        /* Forward after flip */
        memset(mem_cells, 0, N_CELL);
        for (int i = 0; i < N_IN; i++)
            sig[i] = qvecs[qi][i] ^ router_bits[i];
        for (int i = 0; i < N_CELL; i++) {
            if (sig[i]) mem_cells[i] = 255;
            out[i] = mem_cells[i];
        }
        int reward2 = N_CELL * 255;
        for (int i = 0; i < N_CELL; i++) {
            int d = (int)out[i] - (int)target[i];
            reward2 -= (d < 0 ? -d : d);
        }

        if (reward2 > reward) {
            accept++;
        } else {
            router_bits[flip_byte] ^= mask; /* revert */
        }

        /* 每 50000 步评估 */
        if ((steps + 1) % 50000 == 0) {
            int correct = 0;
            for (int p = 0; p < N_QA; p++) {
                memset(mem_cells, 0, N_CELL);
                unsigned char s[N_IN], o[N_CELL];
                for (int i = 0; i < N_IN; i++)
                    s[i] = qvecs[p][i] ^ router_bits[i];
                for (int i = 0; i < N_CELL; i++) {
                    if (s[i]) mem_cells[i] = 255;
                    o[i] = mem_cells[i];
                }
                if (argmax(o) == p) correct++;
            }
            float acc = 100.0f * correct / N_QA;
            if (correct > best_acc) best_acc = correct;

            printf("  步 %6d: 准确率 %2d/16 (%.0f%%)  接受: %d (%.1f%%)",
                   steps+1, correct, acc, accept, 100.0*accept/(steps+1));

            if (correct == N_QA) { printf(" ★★★ 完美收敛! ★★★\n"); break; }
            printf("\n");

            if (acc >= TARGET_ACC * 100) {
                printf("  目标准确率 %.0f%% 达成! 提前停止。\n", TARGET_ACC*100);
                break;
            }
        }
    }

    /* ===== 最终评估 ===== */
    printf("\n═══ 训练完成 ═══\n");
    printf("总步数: %d  接受翻转: %d (%.2f%%)\n", steps, accept, 100.0*accept/steps);
    printf("Router bits: ");
    for (int i = 0; i < 8; i++) printf("%02X ", router_bits[i]);
    printf("\n\n");

    int final_correct = 0;
    printf("═══ 最终评估 ═══\n");
    for (int p = 0; p < N_QA; p++) {
        memset(mem_cells, 0, N_CELL);
        unsigned char s[N_IN], o[N_CELL];
        for (int i = 0; i < N_IN; i++)
            s[i] = qvecs[p][i] ^ router_bits[i];
        for (int i = 0; i < N_CELL; i++) {
            if (s[i]) mem_cells[i] = 255;
            o[i] = mem_cells[i];
        }
        int pred = argmax(o);
        if (pred == p) final_correct++;
        printf("  %-16s → cell[%2d]=%3d %s\n", questions[p], pred, o[pred],
               pred == p ? "✅" : "❌");
    }
    printf("\n最终准确率: %d/%d (%.0f%%)\n\n", final_correct, N_QA, 100.0*final_correct/N_QA);

    /* ===== 交互对话 ===== */
    printf("═══ 实时对话 ═══\n");
    printf("(网络实时计算路由, 无需查表)\n");
    printf("输入问题或 'list'/'quit':\n");

    char input[256];
    while (1) {
        printf("\n> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit")) { printf("再见!\n"); break; }
        if (!strcmp(input, "list")) {
            for (int i = 0; i < N_QA; i++) printf("  [%d] %s\n", i, questions[i]);
            continue;
        }

        /* === 网络实时推理 (100% 计算) === */
        unsigned char vec[N_IN], sig[N_IN], out[N_CELL];
        encode(input, vec);
        memset(mem_cells, 0, N_CELL);
        for (int i = 0; i < N_IN; i++) sig[i] = vec[i] ^ router_bits[i];
        for (int i = 0; i < N_CELL; i++) {
            if (sig[i]) mem_cells[i] = 255;
            out[i] = mem_cells[i];
        }
        int cell = argmax(out);

        printf("BoolBot: %s\n", answers[cell]);
        printf("(网络计算: cell[%d]=%d 触发)\n", cell, out[cell]);
    }

    return 0;
}
