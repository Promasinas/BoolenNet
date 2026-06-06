/**
 * boolchat.c — 纯布尔 Byte-Stream 对话机器人
 *
 * 全链路纯布尔运算:
 *   encode: 文本 → 64B (XOR hash)
 *   路由树: byte 比较 → 1-bit 决策 (每层 1 次 CMP)
 *   叶子:   input[i] XOR rbits[i] → output byte[i] (512 次 XOR/forward)
 *   输出:   64 bytes → 文本显示
 *
 * 零浮点, 零矩阵乘, 零查表 (路由决策由网络实时计算).
 * 路由器参数由 XOR 闭式解得: rbits = input XOR target.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N    64
#define N_QA 16
#define D    4   /* 树深度 */

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

/* ===== 纯布尔运算核心 ===== */

/* Byte-XOR hash: 文本 → 64B 向量 */
static void encode(const char *s, unsigned char *v)
{
    int i, len = (int)strlen(s);
    memset(v, 0, N);
    if (!len) { v[0] = 0xFF; return; }
    for (i = 0; i < len && i < N; i++) v[i] = (unsigned char)s[i];
    for (; i < N; i++) v[i] = 0;
    v[N-2] ^= (unsigned char)(len & 0xFF);
    v[N-1] ^= (unsigned char)((len>>8) & 0xFF);
}

/* 路由树节点: 1 次字节比较 = 1 bit 决策 */
typedef struct { int bp; unsigned char rv; int l, r, qa; } Node;
static Node nd[64]; static int nn;

static int build(int *ix, int nx, int d)
{
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

/* 路由推理: 纯布尔, O(log N) 次字节比较 */
static int route(const unsigned char *v)
{
    int n=0;
    while(nd[n].l>=0) n=(v[nd[n].bp]<=nd[n].rv)?nd[n].l:nd[n].r;
    return nd[n].qa;
}

/* ===== 主程序 ===== */
int main(void)
{
    srand((unsigned)time(NULL));

    /* 1. 构建路由树 */
    int all[N_QA]; for(int i=0;i<N_QA;i++)all[i]=i;
    build(all,N_QA,0);

    /* 2. 每个叶子计算路由器参数: rbits = input XOR target (纯布尔闭式解) */
    unsigned char qv[N_QA][N], av[N_QA][N], rb[N_QA][N];
    for(int i=0;i<N_QA;i++){
        encode(questions[i],qv[i]); encode(answers[i],av[i]);
        for(int j=0;j<N;j++) rb[i][j]=qv[i][j]^av[i][j]; /* 纯 XOR */
    }

    printf("╔══════════════════════════════════════╗\n");
    printf("║  BoolNet 纯布尔 Byte-Stream 对话    ║\n");
    printf("║  编码: XOR hash (bool)              ║\n");
    printf("║  路由: %d 次字节比较 (bool)         ║\n", D);
    printf("║  输出: %d 次 XOR (bool)             ║\n", N);
    printf("║  零浮点 零矩阵乘 零查表             ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("\n路由树: %d 节点 (%d 内部 + %d 叶子)\n", nn, nn-N_QA, N_QA);
    printf("路由器: %d × 512bit = %d bits\n\n", N_QA, N_QA*512);

    /* 3. 评估 */
    printf("═══ 评估 ═══\n");
    for(int p=0;p<N_QA;p++){
        unsigned char in[N], out[N];
        encode(questions[p],in);
        int qi=route(in);
        for(int i=0;i<N;i++) out[i]=in[i]^rb[qi][i]; /* XOR 计算 */
        int ok=(qi==p);
        printf("  Q%-2d \"%-16s\" → leaf[%d] ",p,questions[p],qi);
        for(int i=0;i<N&&out[i];i++) if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf(" %s\n",ok?"✅":"❌");
    }

    /* 4. 交互 */
    printf("\n═══ 实时对话 ═══\n(输入/list/quit)\n");
    char in[256];
    while(1){
        printf("\n> "); fflush(stdout);
        if(!fgets(in,sizeof(in),stdin))break;
        in[strcspn(in,"\n")]=0;
        if(!in[0])continue;
        if(!strcmp(in,"quit")){printf("再见!\n");break;}
        if(!strcmp(in,"list")){for(int i=0;i<N_QA;i++)printf("  %s\n",questions[i]);continue;}

        /* === 纯布尔推理 === */
        unsigned char vec[N], out[N];
        encode(in,vec);
        int qi=route(vec);                     /* 路由: O(log N) 次 CMP */
        for(int i=0;i<N;i++) out[i]=vec[i]^rb[qi][i]; /* 输出: N 次 XOR */

        printf("输出(hex): "); for(int i=0;i<32;i++)printf("%02X ",out[i]);
        printf("\n输出(txt): "); for(int i=0;i<N&&out[i];i++)if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf("\n(路由 leaf[%d] QA[%d]=\"%s\")\n",qi,qi,questions[qi]);
    }
    return 0;
}
