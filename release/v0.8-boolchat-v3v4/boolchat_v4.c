/**
 * boolchat_v4.c — 暴力搜索最优纠错编码
 *
 * 每 byte 位置独立搜索 256 个可能值, 选最小化对所有变体距离的值。
 * 8-bit × 64 bytes × 256 trials = 确定性全局最优, O(16K) per leaf.
 *
 * 编译: gcc -std=c99 -O2 boolchat_v4.c -o boolchat_v4.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N    96
#define N_QA 16
#define D    4
#define MAX_VAR 6

static const char *questions[N_QA] = {
    "hello","who are you","weather","time","boolnet","training",
    "goodbye","thanks","what can you do","who made you","tsetlin",
    "router","save","load","your name","help"
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
static const char *variants[N_QA][MAX_VAR] = {
    {"hello","helo","hllo","helloo","hi",NULL},
    {"who are you","who r u","who are u","who you","whos this",NULL},
    {"weather","whether","temperature","wether","rain",NULL},
    {"time","clock","what time","current time","date",NULL},
    {"boolnet","what is boolnet","bool net","boolean network","boolnet?",NULL},
    {"training","how to train","learn","learning","teach",NULL},
    {"goodbye","bye","see you","farewell","cya",NULL},
    {"thanks","thank you","thx","ty","appreciate",NULL},
    {"what can you do","capabilities","what do you do","skills","features",NULL},
    {"who made you","creator","who built you","author","who coded you",NULL},
    {"tsetlin","tsetlin automaton","what is tsetlin","tsetlin machine","tsetlin algorithm",NULL},
    {"router","boolean router","xor router","router bit","routing",NULL},
    {"save","save model","export","export model","store",NULL},
    {"load","load weights","import","restore","reload",NULL},
    {"your name","whats your name","who are you called","name","what do i call you",NULL},
    {"help","commands","menu","what can i do","options",NULL},
};

static void encode(const char *s, unsigned char *v)
{
    int i, len = (int)strlen(s);
    memset(v, 0, N); if (!len) { v[0]=0xFF; return; }
    for(i=0;i<len;i++){int p=(i*7+3)%N;v[p]^=(unsigned char)s[i];}
    for(i=0;i<len-1;i++){int p=((unsigned char)s[i]+(unsigned char)s[i+1])%N;v[p]^=(unsigned char)((s[i]*31+s[i+1]*17)&0xFF);}
    v[N-3]^=(unsigned char)(len&0xFF);v[N-2]^=(unsigned char)((len>>4)&0xFF);v[N-1]^=(unsigned char)((len*13)&0xFF);
}

typedef struct { int bp; unsigned char rv; int l,r,qa; } Node;
static Node nd[64]; static int nn;
static int build(int *ix,int nx,int d){
    int m=nn++; if(nx<=1||d>=D){nd[m].qa=ix[0];nd[m].l=nd[m].r=-1;return m;}
    unsigned char qv[N_QA][N]; for(int i=0;i<N_QA;i++)encode(questions[i],qv[i]);
    int bb=0,bv=qv[ix[0]][0],bs=99999;
    for(int b=0;b<N;b++)for(int i=0;i<nx;i++){unsigned char val=qv[ix[i]][b];int nl=0,nr=0;
    for(int j=0;j<nx;j++){if(qv[ix[j]][b]<=val)nl++;else nr++;}int sc=(nl>nr)?nl-nr:nr-nl;if(sc<bs&&nl>0&&nr>0){bs=sc;bb=b;bv=val;}}
    nd[m].bp=bb;nd[m].rv=bv;int *L=malloc(nx*4),*R=malloc(nx*4),nl=0,nr=0;
    for(int i=0;i<nx;i++){if(qv[ix[i]][bb]<=bv)L[nl++]=ix[i];else R[nr++]=ix[i];}
    nd[m].l=build(L,nl,d+1);nd[m].r=build(R,nr,d+1);free(L);free(R);return m;
}
static int route(const unsigned char *v){int n=0;while(nd[n].l>=0)n=(v[nd[n].bp]<=nd[n].rv)?nd[n].l:nd[n].r;return nd[n].qa;}

/* 暴力搜索: 每个 byte 试全部 256 值, 找对全部变体最优 */
static void train_bruteforce(unsigned char *rb, int qi)
{
    unsigned char var_vecs[MAX_VAR][N], av[N];
    int nv = 0;
    encode(answers[qi], av);
    for (int v=0; v<MAX_VAR && variants[qi][v]; v++) {
        encode(variants[qi][v], var_vecs[nv]); nv++;
    }
    if (!nv) return;

    printf("  Leaf[%2d] \"%s\" (%d vars): ", qi, questions[qi], nv);

    for (int byte = 0; byte < N; byte++) {
        int best_val = 0, best_dist = 999999999;
        /* 暴力测试全部 256 个值 */
        for (int trial = 0; trial < 256; trial++) {
            int total = 0;
            for (int v = 0; v < nv; v++) {
                int d = (int)(var_vecs[v][byte] ^ (unsigned char)trial) - (int)av[byte];
                total += (d < 0 ? -d : d);
            }
            if (total < best_dist) { best_dist = total; best_val = trial; }
        }
        rb[byte] = (unsigned char)best_val;
    }

    /* 评估质量 */
    int avg = 0, worst = 0;
    for (int v = 0; v < nv; v++) {
        int d = 0;
        for (int i = 0; i < N; i++) {
            int diff = (int)(var_vecs[v][i] ^ rb[i]) - (int)av[i];
            d += (diff < 0 ? -diff : diff);
        }
        avg += d; if (d > worst) worst = d;
    }
    printf("avg=%d worst=%d %s\n", avg/nv, worst, worst<300?"✅":"⚠️");
}

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet v4 — 暴力搜索全局最优        ║\n");
    printf("║  256^64 搜索空间, O(16K) 每叶 ⭐       ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    int all[N_QA]; for(int i=0;i<N_QA;i++)all[i]=i;
    build(all,N_QA,0);
    printf("路由树: %d 节点\n\n", nn);

    unsigned char rb[N_QA][N];
    printf("=== 暴力搜索训练 ===\n");
    for(int i=0;i<N_QA;i++) train_bruteforce(rb[i], i);

    /* 评估 */
    printf("\n═══ 精确匹配 ═══\n");
    unsigned char qv[N_QA][N],av[N_QA][N];
    for(int i=0;i<N_QA;i++){encode(questions[i],qv[i]);encode(answers[i],av[i]);}
    int ok=0;
    for(int p=0;p<N_QA;p++){
        unsigned char in[N],out[N]; encode(questions[p],in); int qi=route(in);
        for(int i=0;i<N;i++) out[i]=in[i]^rb[qi][i];
        if(qi==p&&!memcmp(out,av[p],N))ok++;
    }
    printf("准确率: %d/%d\n\n", ok, N_QA);

    /* 模糊测试 */
    printf("═══ 模糊匹配 ═══\n");
    const char *tests[] = {"helo","hi","who r u","wether","bye","thx",
                           "whats boolnet","how to learn","cya","help me",NULL};
    for(int t=0;tests[t];t++){
        unsigned char in[N],out[N]; encode(tests[t],in); int qi=route(in);
        for(int i=0;i<N;i++) out[i]=in[i]^rb[qi][i];
        printf("  \"%s\" → [%d]\"%s\": ",tests[t],qi,questions[qi]);
        for(int i=0;i<N&&out[i];i++)if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf("\n");
    }

    printf("\n═══ 对话 ═══\n(输入/quit)\n");
    char in[256];
    while(1){
        printf("\n> ");fflush(stdout);
        if(!fgets(in,sizeof(in),stdin))break;in[strcspn(in,"\n")]=0;
        if(!in[0])continue; if(!strcmp(in,"quit")){printf("再见!\n");break;}
        unsigned char vec[N],out[N]; encode(in,vec); int qi=route(vec);
        for(int i=0;i<N;i++) out[i]=vec[i]^rb[qi][i];
        for(int i=0;i<N&&out[i];i++)if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf("\n");
    }
    return 0;
}
