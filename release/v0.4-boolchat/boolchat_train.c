/**
 * boolchat_train.c — 用 Coordinate Descent 训练的纯布尔对话机器人
 *
 * 与 boolchat.c 相同的级联树架构，但路由器参数由 Coord Descent 训练得到
 * (而非直接计算)。Coord Descent 逐比特测试，确定性收敛。
 *
 * 编译: gcc -std=c99 -O2 boolchat_train.c -o boolchat_train.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N    64
#define N_QA 16
#define D    4

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

static void encode(const char *s, unsigned char *v) {
    memset(v, 0, N);
    int len = (int)strlen(s);
    if (!len) { v[0] = 0xFF; return; }
    for (int i = 0; i < len && i < N; i++) v[i] = (unsigned char)s[i];
    v[N-2] ^= (unsigned char)(len & 0xFF);
    v[N-1] ^= (unsigned char)((len>>8) & 0xFF);
}

/* ===== 路由树 (同 boolchat.c) ===== */
typedef struct { int bp; unsigned char rv; int l, r, qa; } Node;
static Node nd[64]; static int nn;

static int build(int *ix, int nx, int d) {
    int m = nn++;
    if (nx <= 1 || d >= D) { nd[m].qa = ix[0]; nd[m].l = nd[m].r = -1; return m; }
    unsigned char qv[N_QA][N]; for (int i=0;i<N_QA;i++) encode(questions[i],qv[i]);
    int bb=0, bv=qv[ix[0]][0], bs=999;
    for (int b=0;b<N;b++) for (int i=0;i<nx;i++) {
        unsigned char val=qv[ix[i]][b]; int nl=0,nr=0;
        for(int j=0;j<nx;j++) { if(qv[ix[j]][b]<=val)nl++; else nr++; }
        int sc=(nl>nr)?nl-nr:nr-nl; if(sc<bs&&nl>0&&nr>0){bs=sc;bb=b;bv=val;}
    }
    nd[m].bp=bb; nd[m].rv=bv;
    int *L=malloc(nx*4),*R=malloc(nx*4),nl=0,nr=0;
    for(int i=0;i<nx;i++){if(qv[ix[i]][bb]<=bv)L[nl++]=ix[i];else R[nr++]=ix[i];}
    nd[m].l=build(L,nl,d+1); nd[m].r=build(R,nr,d+1); free(L);free(R); return m;
}
static int route(const unsigned char *v) {
    int n=0; while(nd[n].l>=0) n=(v[nd[n].bp]<=nd[n].rv)?nd[n].l:nd[n].r;
    return nd[n].qa;
}

/* ===== Coord Descent 训练单个叶子路由器 ===== */
static void train_leaf(unsigned char *rb, const unsigned char *qv,
                       const unsigned char *av, int qi)
{
    int total_bits = N * 8;  /* 512 bits */
    int sweeps = 0, max_sweeps = 20;

    int best_dist = 999999;
    for (int s = 0; s < max_sweeps; s++) {
        int changed = 0;
        /* 逐比特测试: 翻 → 测距离 → 保留或回退 */
        for (int byte = 0; byte < N; byte++) {
            for (int bit = 0; bit < 8; bit++) {
                unsigned char mask = (unsigned char)(1u << bit);

                /* 只计算相关字节的距离变化, 非全部 64 字节 */
                int d_before = (int)(qv[byte] ^ rb[byte]) - (int)av[byte];
                if (d_before < 0) d_before = -d_before;

                rb[byte] ^= mask;

                int d_after = (int)(qv[byte] ^ rb[byte]) - (int)av[byte];
                if (d_after < 0) d_after = -d_after;

                if (d_after < d_before) changed++;
                else rb[byte] ^= mask;
            }
        }
        sweeps++;

        /* 计算总距离 */
        int total = 0;
        for (int i = 0; i < N; i++) {
            int d = (int)(qv[i] ^ rb[i]) - (int)av[i];
            total += (d < 0 ? -d : d);
        }
        if (total < best_dist) best_dist = total;
        if (changed == 0) break;
    }

    /* 最终距离 */
    int final_dist = 0;
    for (int i = 0; i < N; i++) {
        int d = (int)(qv[i] ^ rb[i]) - (int)av[i];
        final_dist += (d < 0 ? -d : d);
    }
    printf("  Leaf[%2d] \"%s\": %d sweeps, dist=%d %s\n",
           qi, questions[qi], sweeps, final_dist, final_dist == 0 ? "✅" : "❌");
}

int main(void)
{
    srand((unsigned)time(NULL));

    /* 1. 路由树 */
    int all[N_QA]; for(int i=0;i<N_QA;i++)all[i]=i;
    build(all, N_QA, 0);

    /* 2. 编码 Q&A */
    unsigned char qv[N_QA][N], av[N_QA][N], rb[N_QA][N];
    for(int i=0;i<N_QA;i++){
        encode(questions[i], qv[i]);
        encode(answers[i], av[i]);
        for(int j=0;j<N;j++) rb[i][j] = (unsigned char)(rand() & 0xFF);
    }

    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Coord Descent 训练纯布尔对话           ║\n");
    printf("║  每叶子 512 比特, O(2N) 确定性收敛      ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 3. Coord Descent 逐叶子训练 */
    printf("=== 训练 (Coord Descent, max 5 sweeps/leaf) ===\n");
    clock_t t0 = clock();
    for (int i = 0; i < N_QA; i++)
        train_leaf(rb[i], qv[i], av[i], i);
    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("\n训练时间: %.2f 秒\n\n", sec);

    /* 4. 评估 */
    printf("═══ 评估 ═══\n");
    int ok = 0;
    for (int p = 0; p < N_QA; p++) {
        unsigned char in[N], out[N];
        encode(questions[p], in);
        int qi = route(in);
        for (int i = 0; i < N; i++) out[i] = in[i] ^ rb[qi][i];
        int correct = (qi == p) && !memcmp(out, av[p], N);
        if (correct) ok++;
        printf("  Q%-2d \"%-16s\" → leaf[%d] ", p, questions[p], qi);
        for (int i = 0; i < N && out[i]; i++)
            if (out[i] >= 32 && out[i] < 127) putchar(out[i]);
        printf(" %s\n", correct ? "✅" : "❌");
    }
    printf("\n准确率: %d/%d (%.0f%%)\n", ok, N_QA, 100.0*ok/N_QA);

    /* 5. 交互 */
    printf("\n═══ 实时对话 ═══\n(输入/list/quit)\n");
    char in[256];
    while (1) {
        printf("\n> "); fflush(stdout);
        if (!fgets(in, sizeof(in), stdin)) break;
        in[strcspn(in, "\n")] = 0;
        if (!in[0]) continue;
        if (!strcmp(in, "quit")) { printf("再见!\n"); break; }
        if (!strcmp(in, "list")) { for(int i=0;i<N_QA;i++) printf("  %s\n", questions[i]); continue; }

        unsigned char vec[N], out[N];
        encode(in, vec);
        int qi = route(vec);
        for (int i = 0; i < N; i++) out[i] = vec[i] ^ rb[qi][i];

        printf("输出: ");
        for (int i = 0; i < N && out[i]; i++)
            if (out[i] >= 32 && out[i] < 127) putchar(out[i]);
        printf("\n");
    }
    return 0;
}
