/**
 * train_long.c — BoolNet Cascade Long Training (~1 hour)
 *
 * 逐叶子 Tsetlin 训练, 从随机初始化到收敛。
 * 每 60 秒报告进度, 收敛后自动停止。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define N    64
#define N_QA 16
#define MAX_DEPTH 4
#define STEPS_PER_LEAF 50000000  /* 50M per leaf */

static const char *questions[N_QA] = {
    "hello", "who are you", "weather", "time",
    "what is boolnet", "how to train", "goodbye", "thanks",
    "what can you do", "who made you", "tsetlin", "router",
    "save model", "load weights", "your name", "help"
};
static const char *answers[N_QA] = {
    "Hello! How can I help you?",
    "I am BoolBot, a Boolean router cascade chatbot.",
    "Sorry, I cannot access real-time weather.",
    "Please check your system clock.",
    "BoolNet is a pure Boolean neural network framework.",
    "Use Tsetlin training: flip bit, forward, reward, keep/revert.",
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

static void encode(const char *txt, unsigned char *v)
{
    memset(v, 0, N);
    int len = (int)strlen(txt);
    if (!len) { v[0] = 0xFF; return; }
    for (int i = 0; i < len && i < N; i++)
        v[i] = (unsigned char)txt[i];
    v[N-2] = (unsigned char)(len & 0xFF);
    v[N-1] = (unsigned char)((len>>8) & 0xFF);
}

int main(void)
{
    srand((unsigned)time(NULL));
    time_t start_time = time(NULL);

    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet Long Training (target: ~1hr)   ║\n");
    printf("║  16 leaves × 50M steps = 800M total     ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    unsigned char qv[N_QA][N], av[N_QA][N];
    for (int i = 0; i < N_QA; i++) {
        encode(questions[i], qv[i]);
        encode(answers[i], av[i]);
    }

    /* 16 个叶子路由器, 随机初始化 */
    unsigned char rbits[N_QA][N];
    for (int i = 0; i < N_QA; i++)
        for (int j = 0; j < N; j++)
            rbits[i][j] = (unsigned char)(rand() & 0xFF);

    int total_accept = 0, total_steps = 0;
    int converged = 0;
    int best_dists[N_QA];
    for (int i = 0; i < N_QA; i++) best_dists[i] = 999999;

    printf("开始训练: %s", ctime(&start_time));
    printf("目标: 所有 16 个叶子距离收敛到 0\n\n");

    for (int qi = 0; qi < N_QA && !converged; qi++) {
        unsigned char *rb = rbits[qi];
        int accept = 0, best_d = 999999;

        for (int s = 0; s < STEPS_PER_LEAF; s++) {
            unsigned char out[N];
            int db = 0;
            for (int i = 0; i < N; i++) {
                out[i] = qv[qi][i] ^ rb[i];
                int d = (int)out[i] - (int)av[qi][i];
                db += (d < 0 ? -d : d);
            }

            int fb = rand() % (N*8);
            rb[fb/8] ^= (unsigned char)(1u << (fb%8));

            int da = 0;
            for (int i = 0; i < N; i++) {
                out[i] = qv[qi][i] ^ rb[i];
                int d = (int)out[i] - (int)av[qi][i];
                da += (d < 0 ? -d : d);
            }

            if (da < db) {
                accept++;
                if (da < best_d) best_d = da;
            } else {
                rb[fb/8] ^= (unsigned char)(1u << (fb%8));
            }

            total_steps++;

            /* 每分钟报告 */
            if (time(NULL) - start_time >= 60 && s % 1000000 == 0) {
                int elapsed = (int)(time(NULL) - start_time);
                printf("[%02d:%02d] Leaf[%d] \"%s\": dist=%d acc=%d (%.1f%%)\n",
                       elapsed/60, elapsed%60, qi, questions[qi],
                       best_d, accept, 100.0*accept/(s+1));
                fflush(stdout);
            }

            if (best_d == 0 && s > 1000) {
                printf("  ✅ Leaf[%d] 收敛于 %d 步!\n", qi, s+1);
                break;
            }
        }
        best_dists[qi] = best_d;
        total_accept += accept;
        if (best_d > 0) converged = 0; /* 未收敛则标记 */
    }

    /* 汇总 */
    int elapsed = (int)(time(NULL) - start_time);
    printf("\n═══ 训练结束 ═══\n");
    printf("总时间: %d:%02d:%02d\n", elapsed/3600, (elapsed/60)%60, elapsed%60);
    printf("总步数: %d  接受: %d (%.2f%%)\n\n", total_steps, total_accept,
           100.0*total_accept/total_steps);

    int all_zero = 1;
    printf("各叶子最终距离:\n");
    for (int i = 0; i < N_QA; i++) {
        printf("  Leaf[%2d] \"%s\": dist=%d\n", i, questions[i], best_dists[i]);
        if (best_dists[i] > 0) all_zero = 0;
    }

    if (all_zero) {
        printf("\n🎉 全部收敛! 保存模型到 weights/cascade_trained.bin\n");
        FILE *f = fopen("weights/cascade_trained.bin", "wb");
        if (f) {
            uint32_t nqa = N_QA; fwrite(&nqa, 4, 1, f);
            for (int i = 0; i < N_QA; i++)
                fwrite(rbits[i], 1, N, f);
            fclose(f);
        }
    } else {
        printf("\n⚠ 未完全收敛。可能需要更多时间或调整策略。\n");
    }

    return 0;
}
