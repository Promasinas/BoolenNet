/**
 * chatbot_demo.c — BoolNet 轻量固定对话机器人 (简化版)
 *
 * 16 组预设 Q&A。使用简化的 byte-hash 匹配直接路由到答案。
 * 演示 BoolNet 在实际对话场景中的应用。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BYTES  16
#define N_QA     16
#define N_CELLS  16   /* 1 cell per question */

static const char *questions[N_QA] = {
    "你好", "你是谁", "天气", "时间",
    "BoolNet是什么", "怎么训练", "再见", "谢谢",
    "你会做什么", "谁创造了你", "Tsetlin", "布尔路由器",
    "保存模型", "加载权重", "你的名字", "帮助"
};
static const char *answers[N_QA] = {
    "你好！有什么可以帮你的？",
    "我是BoolNet对话机器人，基于布尔路由器级联树。",
    "抱歉，我无法获取实时天气。我是本地推理模型。",
    "请查看系统时间，我的时钟精度取决于运行环境。",
    "BoolNet是纯布尔运算神经网络框架，使用布尔路由器和Tsetlin自动机。",
    "使用Tsetlin训练：随机翻转路由器bit→forward→计算reward→保留/回退。",
    "再见！期待下次对话。",
    "不客气！有问题随时问我。",
    "我可以回答BoolNet技术问题和简单对话。",
    "由BoolNet框架使用者创建和训练。",
    "Tsetlin自动机：基于状态机的学习算法，逐bit翻转优化决策。",
    "布尔路由器：BoolNet基础单元。输入与路由参数等长，bit=1翻转输入。",
    "使用 tsetlin_export_model() 保存到 weights/ 目录。",
    "从 weights/ 用 mem_int_load() 或 tsetlin_import_model() 加载。",
    "我叫BoolBot，基于级联布尔路由树的对话系统。",
    "命令: 输入中文问题, 'list' 查看问题, 'quit' 退出。"
};

/* 简单位置敏感 byte-hash 编码 */
static void encode(const char *text, uint8_t *vec)
{
    memset(vec, 0, N_BYTES);
    int len = (int)strlen(text);
    for (int i = 0; i < len; i++) {
        int pos = (i * 7 + 3) % N_BYTES;
        vec[pos] ^= (uint8_t)(text[i] + (i % 31));
    }
    vec[0] ^= (uint8_t)(len & 0xFF);
    vec[N_BYTES-1] ^= (uint8_t)((len >> 4) & 0xFF);
}

/* 计算两个向量的汉明距离 */
static int hamming(const uint8_t *a, const uint8_t *b)
{
    int d = 0;
    for (int i = 0; i < N_BYTES; i++) {
        uint8_t x = a[i] ^ b[i];
        while (x) { d += x & 1; x >>= 1; }
    }
    return d;
}

static void reset_mem(BoolNet *n) {
    for (uint32_t i = 0; i < n->num_layers; i++)
        if (n->layers[i].type == LAYER_MEMORY)
            mem_int_reset((MemIntLayer*)n->layers[i].instance);
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║     BoolNet 轻量对话机器人              ║\n");
    printf("║     16 Q&A, 级联路由树                 ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 1. 编码所有问题 */
    uint8_t qvecs[N_QA][N_BYTES];
    for (int i = 0; i < N_QA; i++)
        encode(questions[i], qvecs[i]);

    /* 2. 构建训练网络: Router(128b) → Memory(4c) */
    BoolNet *net = boolnet_create(1, N_BYTES, 2);
    uint8_t zb[N_BYTES] = {0};
    BoolRouter *r = bool_router_create(1, N_BYTES*8, zb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    MemIntLayer *m = mem_int_create(2, ML_PRECISION_UINT8, N_CELLS, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m,
        mem_int_forward_output, (int(*)(void*,const char*))mem_int_save);

    printf("网络: Router(128b)→Memory(4c,output)\n\n");

    /* 3. 训练: 每个问题 → 对应答案的 one-hot cell */
    printf("=== 训练 %d 个 Q&A ===\n", N_QA);
    uint8_t tg[N_CELLS];
    TsetlinTrainer *t = tsetlin_create(net, 10000, N_CELLS * 255);

    int steps_per_q = 2000;
    for (int s = 0; s < N_QA * steps_per_q; s++) {
        int qi = s % N_QA;
        memset(tg, 0, N_CELLS); tg[qi] = 0xFF;  /* 1-to-1 mapping */
        int rc = tsetlin_train_step(t, qvecs[qi], tg);
        if (rc == -2) break;
    }

    uint32_t st, ac; int32_t br;
    tsetlin_get_stats(t, &st, &ac, &br, NULL, NULL);
    printf("训练步数: %u  接受翻转: %u (%.0f%%)\n\n", st, ac, st?100.0f*ac/st:0);

    /* 4. 训练集评估 */
    printf("═══ 训练集评估 ═══\n");
    int correct = 0;
    for (int i = 0; i < N_QA; i++) {
        reset_mem(net);
        uint8_t out[N_CELLS];
        boolnet_forward(net, qvecs[i], out);
        int mx = 0;
        for (int j = 1; j < N_CELLS; j++)
            if (out[j] > out[mx]) mx = j;
        if (mx == i) correct++;
        printf("  %-16s → cell[%d]=%3d %s\n", questions[i], mx, out[mx],
               mx == i ? "✅" : "❌");
    }
    printf("准确率: %d/%d (%.0f%%)\n\n", correct, N_QA, 100.0f*correct/N_QA);

    /* 5. 交互模式 */
    printf("═══ 交互对话 ═══\n");
    printf("输入问题 (list/quit):\n");

    char input[256];
    while (1) {
        printf("\n你: ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit") || !strcmp(input, "退出")) {
            printf("BoolBot: 再见！\n"); break;
        }
        if (!strcmp(input, "list")) {
            for (int i = 0; i < N_QA; i++)
                printf("  [%d] %s\n", i, questions[i]);
            continue;
        }

        /* 网络推理 */
        uint8_t vec[N_BYTES], out[N_CELLS];
        encode(input, vec);
        reset_mem(net);
        boolnet_forward(net, vec, out);

        /* 找最大 cell */
        int mx = 0;
        for (int j = 1; j < N_CELLS; j++)
            if (out[j] > out[mx]) mx = j;

        /* Hamming距离匹配 (网络输出作为辅助特征) */
        int best_i = 0, best_d = 999;
        for (int i = 0; i < N_QA; i++) {
            int d = hamming(vec, qvecs[i]);
            /* 网络输出加权: 如果 cell[i] 强触发，减少距离 */
            if (out[i] > 128) d -= 50;
            if (d < best_d) { best_d = d; best_i = i; }
        }

        printf("BoolBot: %s\n", answers[best_i]);
    }

    tsetlin_destroy(t);
    boolnet_destroy(net);
    bool_router_destroy(r);
    mem_int_destroy(m);
    return 0;
}
