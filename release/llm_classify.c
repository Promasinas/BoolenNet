/**
 * llm_classify.c — 级联树分类对话 (Agent 2 设计)
 *
 * 架构: one-hot 输入 → 路由树(选叶子) → 叶子 Router XOR → argmax → 答案
 * 每个叶子专属于一个训练对, Router 由 XOR 闭式解计算。
 *
 * 与 byte 流版本的区别: 输出是分类索引(非 byte 序列)。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_TOKENS 16
#define N_TRAIN  5
#define D        4   /* depth=4, 2^4=16 leaves = 16 tokens */

static const char *tokens[N_TOKENS] = {
    "hello","hi","hey","yo",
    "how are you","good morning","good evening","nice to meet",
    "whats up","tell me more","help","who are you",
    "bye","thanks","ok","see you"
};
static const char *answers[N_TOKENS] = {
    "Hello!","Hi there!","Hey you!","Yo!",
    "Im doing well","Good morning!","Good evening!","Nice to meet you",
    "Not much!","Sure thing","How can I help?","Im BoolBot",
    "Goodbye!","Youre welcome!","Got it","Take care!"
};
/* 训练对: input_token → output_token */
static int pairs[N_TRAIN][2] = {
    {0,1}, {4,4}, {11,11}, {12,12}, {13,13}
};

/* 编码: N_TOKENS 维 one-hot */
static void onehot(int tid, unsigned char *v) {
    memset(v, 0, N_TOKENS);
    if (tid >= 0 && tid < N_TOKENS) v[tid] = 255;
}

/* 路由树 */
typedef struct { int bp; unsigned char rv; int l,r,tid; } Node;
static Node nd[64]; static int nn;
static int build(int *ix, int nx, int d) {
    int m=nn++; if(nx<=1||d>=D){nd[m].tid=ix[0];nd[m].l=nd[m].r=-1;return m;}
    unsigned char qv[N_TOKENS][N_TOKENS];
    for(int i=0;i<N_TOKENS;i++) onehot(i,qv[i]);
    int bb=0,bv=qv[ix[0]][0],bs=999;
    for(int b=0;b<N_TOKENS;b++) for(int i=0;i<nx;i++){
        unsigned char val=qv[ix[i]][b]; int nl=0,nr=0;
        for(int j=0;j<nx;j++){if(qv[ix[j]][b]<=val)nl++;else nr++;}
        int sc=(nl>nr)?nl-nr:nr-nl; if(sc<bs&&nl>0&&nr>0){bs=sc;bb=b;bv=val;}
    }
    nd[m].bp=bb; nd[m].rv=bv;
    int *L=malloc(nx*4),*R=malloc(nx*4),nl=0,nr=0;
    for(int i=0;i<nx;i++){if(qv[ix[i]][bb]<=bv)L[nl++]=ix[i];else R[nr++]=ix[i];}
    nd[m].l=build(L,nl,d+1); nd[m].r=build(R,nr,d+1); free(L);free(R); return m;
}
static int route(const unsigned char *v) {
    int n=0; while(nd[n].l>=0) n=(v[nd[n].bp]<=nd[n].rv)?nd[n].l:nd[n].r;
    return nd[n].tid;
}

/* 叶子路由器: 存储输入→输出映射 */
static unsigned char leaf_rb[N_TOKENS][N_TOKENS]; /* rb[tid][j] = input XOR target */

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet LLM Classify — 级联路由分类    ║\n");
    printf("║  one-hot输入 → 树路由 → XOR → argmax    ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 1. 路由树 */
    int all[N_TOKENS];
    for(int i=0;i<N_TOKENS;i++)all[i]=i;
    build(all,N_TOKENS,0);
    printf("树: %d 节点, %d 叶子\n\n", nn, nn-N_TOKENS);

    /* 2. 每个叶子: XOR 闭式解 */
    for(int t=0;t<N_TOKENS;t++){
        unsigned char in[N_TOKENS], tg[N_TOKENS];
        onehot(t,in);
        /* 找这个 token 的目标 */
        int target = t; /* 默认恒等 */
        for(int p=0;p<N_TRAIN;p++)
            if(pairs[p][0]==t) target = pairs[p][1];

        memset(tg,0,N_TOKENS); tg[target]=255;
        for(int j=0;j<N_TOKENS;j++) leaf_rb[t][j] = in[j] ^ tg[j];
    }

    /* 3. 训练集 */
    printf("=== 训练对 ===\n");
    for(int p=0;p<N_TRAIN;p++){
        int in_tok=pairs[p][0], out_tok=pairs[p][1];
        unsigned char vec[N_TOKENS], out[N_TOKENS];
        onehot(in_tok,vec);
        int leaf=route(vec);
        for(int j=0;j<N_TOKENS;j++) out[j]=vec[j]^leaf_rb[leaf][j];
        int pred=0; for(int j=1;j<N_TOKENS;j++) if(out[j]>out[pred])pred=j;
        printf("  \"%s\" → leaf[%d] → \"%s\" %s\n",
               tokens[in_tok], leaf, answers[pred], pred==out_tok?"✅":"❌");
    }

    /* 4. 全词表测试 */
    printf("\n=== 全词表 ===\n");
    int ok=0;
    for(int i=0;i<N_TOKENS;i++){
        int target=i;
        for(int p=0;p<N_TRAIN;p++) if(pairs[p][0]==i) target=pairs[p][1];

        unsigned char vec[N_TOKENS], out[N_TOKENS];
        onehot(i,vec); int leaf=route(vec);
        for(int j=0;j<N_TOKENS;j++) out[j]=vec[j]^leaf_rb[leaf][j];
        int pred=0; for(int j=1;j<N_TOKENS;j++) if(out[j]>out[pred])pred=j;
        if(pred==target)ok++;
        printf("  %-16s → %-16s %s\n", tokens[i], answers[pred], pred==target?"✅":"❌");
    }
    printf("\n准确率: %d/%d (%.0f%%)\n", ok, N_TOKENS, 100.0*ok/N_TOKENS);

    /* 5. 交互 */
    printf("\n═══ 对话 ═══\n");
    char in[256];
    while(1){
        printf("\n> "); fflush(stdout);
        if(!fgets(in,sizeof(in),stdin))break; in[strcspn(in,"\n")]=0;
        if(!in[0])continue;
        if(!strcmp(in,"quit")){printf("再见!\n");break;}

        /* 找最近 token */
        int best_t=0, best_m=0;
        for(int t=0;t<N_TOKENS;t++){
            int m=0, li=(int)strlen(in), lt=(int)strlen(tokens[t]);
            for(int i=0;i<li&&i<lt;i++) if(in[i]==tokens[t][i]) m++;
            if(m>best_m){best_m=m; best_t=t;}
        }

        unsigned char vec[N_TOKENS], out[N_TOKENS];
        onehot(best_t,vec); int leaf=route(vec);
        for(int j=0;j<N_TOKENS;j++) out[j]=vec[j]^leaf_rb[leaf][j];
        int pred=0; for(int j=1;j<N_TOKENS;j++) if(out[j]>out[pred])pred=j;
        printf("BoolBot: %s\n", answers[pred]);
    }
    return 0;
}
