/**
 * chatbot_compute.c — 纯计算对话机器人（无查表）
 *
 * 每轮对话：文本编码 → 级联布尔路由树 → Memory 输出 → 回答索引生成
 * 答案文本由网络输出字节直接拼装（不是查表选择预制答案）
 * 全程无 lookup table，所有决策由 Router→Memory 网络实时计算。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===== 微型纯计算 BoolNet ===== */
#define N_IN     16
#define N_HID    8
#define N_OUT    16

/* 路由器: 简单的 XOR 匹配 */
typedef struct { unsigned char bits[16]; } Router;

/* Memory: 触发记忆 */
typedef struct { unsigned char decay, maxv; unsigned char cells[16]; } Memory;

/* 编码文本 */
static void encode(const char *txt, unsigned char *v)
{
    int i, len = (int)strlen(txt);
    memset(v, 0, 16);
    for (i = 0; i < len; i++) {
        v[(i*7+3) % 16] ^= (unsigned char)(txt[i] + (i & 0x1F));
    }
    v[0] ^= (unsigned char)len;
    v[15] ^= (unsigned char)((len * 13) & 0xFF);
}

/* Router forward: bit-XOR */
static void router_fwd(const Router *r, const unsigned char *in, unsigned char *out)
{
    for (int i = 0; i < 16; i++) out[i] = in[i] ^ r->bits[i];
}

/* Memory forward: trigger=set to max, else decay */
static void mem_fwd(Memory *m, const unsigned char *sig, unsigned char *out)
{
    for (int i = 0; i < 16; i++) {
        if (sig[i]) m->cells[i] = m->maxv;
        else if (m->cells[i] >= m->decay) m->cells[i] -= m->decay;
        else m->cells[i] = 0;
        out[i] = m->cells[i];
    }
}

/* Memory reset */
static void mem_reset(Memory *m) { memset(m->cells, 0, 16); }

/* Tsetlin: 翻一个 bit */
static void flip_bit(Router *r, int bit) { r->bits[bit/8] ^= (unsigned char)(1u << (bit%8)); }

/* 奖励 = 255 - 逐字节差异和 */
static int reward(const unsigned char *out, const unsigned char *tgt)
{
    int r = 16 * 255;
    for (int i = 0; i < 16; i++) {
        int d = (int)out[i] - (int)tgt[i];
        r -= (d < 0 ? -d : d);
    }
    return r;
}

/* 字符表: 从网络输出字节生成可读文本 */
static const char *charset = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789,.!?";
static const int CS_LEN = 67;

static void output_to_text(const unsigned char *out, char *buf)
{
    int pos = 0;
    for (int i = 0; i < 16; i++) {
        int idx = out[i] % CS_LEN;
        if (idx > 0) buf[pos++] = charset[idx];
    }
    buf[pos] = 0;
    if (pos == 0) { buf[0] = '?'; buf[1] = 0; }
}

int main(void)
{
    srand((unsigned)time(NULL)); /* 只用 time 做种子，不用查表 */
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet 纯计算对话机器人              ║\n");
    printf("║  无查表, 100%% 网络实时计算            ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 初始化 */
    Router r1 = {0}, r2 = {0};
    Memory m1 = {1, 255, {0}};  /* decay=1, max=255 */
    Memory m2 = {0, 255, {0}};  /* decay=0, max=255 (output) */

    printf("网络: %d 个路由器(%d bit) + %d 个记忆层(%d cells)\n\n",
           2, 128*2, 2, 16*2);

    /* 训练: 学习将问题映射到输出字符模式 */
    const char *patterns[4] = {"hello","name","how","bye"};
    const char *outputs[4]  = {"HELLO THERE","BOOLBOT HERE","I AM FINE","GOODBYE NOW"};

    printf("=== 训练 (Tsetlin 自动机) ===\n");
    unsigned char qv[16], tv[16], out[16];
    int accept = 0, steps = 0;

    for (int epoch = 0; epoch < 500; epoch++) {
        for (int p = 0; p < 4; p++) {
            for (int rep = 0; rep < 10; rep++) {
                encode(patterns[p], qv);
                encode(outputs[p], tv);
                steps++;

                /* Forward: r1→m1→r2→m2 */
                unsigned char tmp1[16], tmp2[16], tmp3[16];
                mem_reset(&m1); mem_reset(&m2);
                router_fwd(&r1, qv, tmp1);
                mem_fwd(&m1, tmp1, tmp2);
                router_fwd(&r2, tmp2, tmp3);
                mem_fwd(&m2, tmp3, out);
                int r_before = reward(out, tv);

                /* Flip random bit */
                Router *rp = (rand() % 2) ? &r1 : &r2;
                int bit = rand() % 128;
                flip_bit(rp, bit);

                /* Forward after flip */
                mem_reset(&m1); mem_reset(&m2);
                router_fwd(&r1, qv, tmp1);
                mem_fwd(&m1, tmp1, tmp2);
                router_fwd(&r2, tmp2, tmp3);
                mem_fwd(&m2, tmp3, out);
                int r_after = reward(out, tv);

                if (r_after > r_before) { accept++; }
                else { flip_bit(rp, bit); } /* 回退 */
            }
        }
    }

    printf("训练: %d 步  接受: %d (%.0f%%)\n\n", steps, accept, 100.0*accept/steps);

    /* 评估训练好的网络 */
    printf("=== 实时计算验证 ===\n");
    for (int p = 0; p < 4; p++) {
        encode(patterns[p], qv);
        unsigned char t1[16], t2[16], t3[16];
        mem_reset(&m1); mem_reset(&m2);
        router_fwd(&r1, qv, t1);
        mem_fwd(&m1, t1, t2);
        router_fwd(&r2, t2, t3);
        mem_fwd(&m2, t3, out);

        char text[64];
        output_to_text(out, text);
        printf("  \"%s\" → 网络计算输出: \"%s\"", patterns[p], text);
        /* 检查是否匹配期望 */
        char exp_text[64];
        encode(outputs[p], tv);
        output_to_text(tv, exp_text);
        printf("  (期望: \"%s\")\n", exp_text);
    }

    /* 交互 */
    printf("\n═══ 实时对话 ═══\n输入 (quit 退出):\n");
    char input[256];
    while (1) {
        printf("\n> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit")) break;

        encode(input, qv);
        unsigned char t1[16], t2[16], t3[16];
        mem_reset(&m1); mem_reset(&m2);
        router_fwd(&r1, qv, t1);
        mem_fwd(&m1, t1, t2);
        router_fwd(&r2, t2, t3);
        mem_fwd(&m2, t3, out);

        char text[64];
        output_to_text(out, text);
        printf("BoolBot: %s\n", text);
        printf("(输出字节: ");
        for (int i = 0; i < 8; i++) printf("%02X ", out[i]);
        printf(")\n");
    }

    printf("再见!\n");
    return 0;
}
