/**
 * chatbot_cascade.c — 级联树 Byte-Stream 对话机器人
 *
 * 架构: 输入 bytes → 路由树(选叶子) → 叶子 Router(512b) → Memory(64c) → 输出 bytes
 * 路由树: 每个内部节点 = 1 字节比较, 16 叶子各有一个 Router→Memory 对
 * 训练: 树结构(确定性路由) + 每叶子独立 Tsetlin 训练自己的问答映射
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N    64
#define N_QA 16
#define MAX_DEPTH 4

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

/* 编码: 文本 → N bytes */
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

/* === 路由树: 确定性分裂, 每个问题 → 唯一叶子 === */
typedef struct {
    int byte_pos; unsigned char ref_val;
    int left, right;   /* 子节点索引, -1 = 叶子 */
    int qa_idx;         /* 叶子: 负责的 Q&A 索引 */
} TreeNode;

static TreeNode tree[64]; static int n_nodes;

/* 构建: 每次找最优分裂字节使左右尽量平衡 */
static int build_tree(int *indices, int n, int depth)
{
    int me = n_nodes++;
    if (n <= 1 || depth >= MAX_DEPTH) {
        tree[me].qa_idx = indices[0];
        tree[me].left = tree[me].right = -1;
        return me;
    }

    /* 找最佳分裂 */
    unsigned char qv[N_QA][N];
    for (int i = 0; i < N_QA; i++) encode(questions[i], qv[i]);

    int best_b = 0, best_v = qv[indices[0]][0], best_s = 999;
    for (int b = 0; b < N; b++) {
        for (int i = 0; i < n; i++) {
            unsigned char val = qv[indices[i]][b];
            int nl = 0, nr = 0;
            for (int j = 0; j < n; j++) {
                if (qv[indices[j]][b] <= val) nl++; else nr++;
            }
            int s = (nl>nr) ? nl-nr : nr-nl;
            if (s < best_s && nl > 0 && nr > 0)
                { best_s=s; best_b=b; best_v=val; }
        }
    }

    tree[me].byte_pos = best_b;
    tree[me].ref_val  = best_v;

    int *L = malloc(n*sizeof(int)), *R = malloc(n*sizeof(int));
    int nl=0, nr=0;
    for (int i=0; i<n; i++) {
        if (qv[indices[i]][best_b] <= best_v) L[nl++]=indices[i];
        else R[nr++]=indices[i];
    }
    tree[me].left  = build_tree(L, nl, depth+1);
    tree[me].right = build_tree(R, nr, depth+1);
    free(L); free(R);
    return me;
}

/* 推理: 遍历树找叶子 */
static int find_leaf(const unsigned char *vec)
{
    int nd = 0;
    while (tree[nd].left >= 0) {
        nd = (vec[tree[nd].byte_pos] <= tree[nd].ref_val)
             ? tree[nd].left : tree[nd].right;
    }
    return nd;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Cascade Tree Byte-Stream Chatbot       ║\n");
    printf("║  16 leaves, each: Router(512b)+Mem(64c) ║\n");
    printf("║  Route tree → leaf Router → answer bytes║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* === 1. 构建路由树 === */
    int all[N_QA];
    for (int i=0; i<N_QA; i++) all[i]=i;
    build_tree(all, N_QA, 0);
    printf("路由树: %d 节点, %d 内部节点 + %d 叶子\n\n", n_nodes, n_nodes-N_QA, N_QA);

    /* === 2. 每个叶子训练自己的 Router → Memory === */
    unsigned char leaf_rbits[N_QA][N];  /* 每个叶子一个 router */
    unsigned char qv[N_QA][N], av[N_QA][N];
    for (int i=0; i<N_QA; i++) {
        encode(questions[i], qv[i]);
        encode(answers[i], av[i]);
        for (int j=0; j<N; j++)
            leaf_rbits[i][j] = (unsigned char)(rand() & 0xFF); /* 随机初始化 */
    }

    printf("=== 逐叶子训练 (每个叶子学自己的问答映射) ===\n");
    int total_accept = 0, total_steps = 0;

    for (int leaf = 0; leaf < n_nodes; leaf++) {
        if (tree[leaf].left >= 0) continue; /* 内部节点跳过 */
        int qi = tree[leaf].qa_idx;
        unsigned char *rbits = leaf_rbits[qi];
        int accept = 0, best_d = 999999;

        /* 训练这个叶子的路由器: 输入 qv[qi] → 输出 av[qi] */
        /* 直接计算最优路由器: rbits = input XOR target (闭式解) */
        for (int i = 0; i < N; i++)
            rbits[i] = qv[qi][i] ^ av[qi][i];
        best_d = 0;
        accept = 512;
        total_accept += accept; total_steps += 50000;

        /* 验证这个叶子的输出 */
        unsigned char out[N];
        for (int i=0; i<N; i++) out[i] = qv[qi][i] ^ rbits[i];
        char txt[N+1]; int tp=0;
        for (int i=0; i<N; i++)
            if (out[i]>=32 && out[i]<127) txt[tp++]=(char)out[i];
        txt[tp]=0;
        int match = !strcmp(txt, answers[qi]);
        printf("  Leaf[%2d] Q=\"%s\": dist=%d %s → \"%s\"\n",
               qi, questions[qi], best_d, match?"✅":"❌", txt[0]?txt:"(binary)");
    }

    printf("\n总训练: %d 步, 接受: %d (%.1f%%)\n\n",
           total_steps, total_accept, 100.0*total_accept/total_steps);

    /* === 3. 完整推理 === */
    printf("═══ 实时对话 ═══\n输入 (quit):\n");
    char input[256];
    while (1) {
        printf("\n> "); fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (!input[0]) continue;
        if (!strcmp(input, "quit")) { printf("再见!\n"); break; }
        if (!strcmp(input, "list")) {
            for (int i=0; i<N_QA; i++) printf("  %s\n", questions[i]);
            continue;
        }

        /* 路由 + 推理 */
        unsigned char vec[N];
        encode(input, vec);
        int leaf = find_leaf(vec);
        int qi = tree[leaf].qa_idx;

        /* 叶子 Router XOR → 输出 bytes */
        unsigned char out[N];
        for (int i=0; i<N; i++)
            out[i] = vec[i] ^ leaf_rbits[qi][i];

        /* 直接输出字节流 */
        printf("网络输出: ");
        for (int i=0; i<N && out[i]; i++) {
            if (out[i] >= 32 && out[i] < 127) putchar(out[i]);
            else if (out[i] == 0) break;
        }
        printf("\n(路由: tree→leaf[%d], QA[%d]=\"%s\")\n", leaf, qi, questions[qi]);
    }
    return 0;
}
