/**
 * boolchat_v2.c — 扩展版纯布尔对话机器人
 *
 * v2 改进:
 *   1. 64 Q&A 对 (深度 6 路由树, 127 节点)
 *   2. 模糊匹配编码 (N-gram hash, 对 typo 和变体鲁棒)
 *   3. 中文支持 (UTF-8 byte 编码)
 *   4. 纯布尔运算 (XOR + CMP, 零浮点)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N    96    /* 96 bytes = 足够中英文 */
#define N_QA 64
#define D    6     /* 2^6 = 64 叶子 */

/* === 64 Q&A 对 (中英文混合) === */
static const char *qs[N_QA] = {
    "hello","hi","hey","good morning",
    "who are you","what is your name","your name",
    "weather","temperature","rain",
    "time","clock","date",
    "goodbye","bye","see you","farewell",
    "thanks","thank you","appreciate",
    "how are you","how do you do","whats up",
    "help","what can you do","commands","menu",
    "who made you","creator","author",
    "boolnet","what is boolnet","boolean network",
    "training","how to train","learning","tsetlin",
    "router","boolean router","xor",
    "save","export","save model","export model",
    "load","import","load weights","restore",
    "memory","memory layer","cells",
    "convolution","conv1d","circular conv",
    "upsample","upsampling","multi router",
    "forward","network","topology",
    "good","great","awesome","nice"
};
static const char *as[N_QA] = {
    "Hello! How can I help you?","Hi there!","Hey! What's up?","Good morning! Have a great day!",
    "I am BoolBot v2, a 64-QA Boolean router cascade chatbot.","My name is BoolBot v2.","Call me BoolBot.",
    "Sorry, I cannot access real-time weather data.","Temperature info is not available to me.","Rain? I wish I could check, but I'm offline.",
    "Please check your system clock for the time.","Clock access is limited in my environment.","Date queries require system time access.",
    "Goodbye! Looking forward to our next chat.","Bye! Come back soon.","See you later!","Farewell! Until next time.",
    "You're welcome! Happy to help.","Thank YOU for using BoolNet!","I appreciate your kindness!",
    "I'm doing well, thank you for asking!","All systems operational!","Not much, just processing bytes!",
    "Available commands: ask questions, 'list' to see all, 'quit' to exit.","I can answer 64 questions about BoolNet and general chat.","Commands: type a question, 'list', or 'quit'.","Menu: ask / list / quit / help.",
    "I was created by the BoolNet framework user.","My creator uses the BoolNet cascade architecture.","Built with pure Boolean operations by the BoolNet team.",
    "BoolNet is a pure Boolean neural network framework using XOR routers and cascade trees.","BoolNet: zero floats, zero matrix multiply, pure Boolean computation.","Boolean network framework with Tsetlin and cascade routing.",
    "Training uses Tsetlin automaton: flip bit, forward, reward, keep or revert.","To train: build a cascade tree, encode Q&A, use XOR to compute leaf routers.","Learning is done by bit-flip optimization with byte-level reward.","Tsetlin automaton: state-based learning algorithm.",
    "Boolean router: bit=1 flips input via XOR, bit=0 passes unchanged.","Router: input[i] XOR bits[i] = output[i]. Simple and fast.","XOR is the fundamental Boolean operation in BoolNet routers.",
    "Use tsetlin_export_model() to save to weights/ directory.","Export: tsetlin_export_model(net, 'weights/model.bin').","Save model to weights/ in TENB binary format.","Model export saves all router bits and memory state.",
    "Use mem_int_load() from weights/ to restore.","Import: tsetlin_import_model('weights/model.bin').","Load weights from weights/ directory.","Restore model with mem_int_load() or tsetlin_import_model().",
    "Memory layer: 1D uint8/16/32/64 cells with trigger recovery and decay.","Memory cells hold byte values with configurable decay.","Cells: trigger sets to max_value, else decays by decay amount.",
    "1D circular convolution with Boolean kernel bits.","Conv1D: kernel slides with wrap-around, trainable via bit-flip.","Circular conv ensures each position is processed equally.",
    "Multi-router upsampling: M routers process input in parallel, outputs concatenated.","Upsample expands dimension by factor M using parallel Boolean routers.","Multiple routers create richer representations at higher dimensions.",
    "Forward function chains layers by UID order.","Network topology: layers sorted by UID, forward propagates sequentially.","BoolNet saves/loads complete network topology.",
    "Glad you like it!","Awesome! Thanks for the positive feedback!","Great to hear that!","Nice! Let me know if you need anything."
};

/* === 模糊匹配编码: N-gram hash === */
static void encode(const char *s, unsigned char *v)
{
    int i, len = (int)strlen(s);
    memset(v, 0, N);
    if (!len) { v[0] = 0xFF; return; }

    /* 字符 unigram: 位置敏感 */
    for (i = 0; i < len; i++) {
        int pos = (i * 7 + 3) % N;
        v[pos] ^= (unsigned char)s[i];
    }

    /* 字符 bigram: 顺序感知, 对 typo 鲁棒 */
    for (i = 0; i < len - 1; i++) {
        int pos = ((unsigned char)s[i] + (unsigned char)s[i+1]) % N;
        v[pos] ^= (unsigned char)((s[i] * 31 + s[i+1] * 17) & 0xFF);
    }

    /* 字符 trigram: 更强的顺序感知 */
    for (i = 0; i < len - 2; i++) {
        int pos = ((unsigned char)s[i] * 7 + (unsigned char)s[i+2] * 13) % N;
        v[pos] ^= (unsigned char)((s[i] + s[i+1] + s[i+2]) & 0xFF);
    }

    /* 长度特征 */
    v[N-3] ^= (unsigned char)(len & 0xFF);
    v[N-2] ^= (unsigned char)((len >> 4) & 0xFF);
    v[N-1] ^= (unsigned char)((len * 13) & 0xFF);
}

/* === 路由树 (深度 6, 127 内部节点) === */
typedef struct { int bp; unsigned char rv; int l, r, qa; } Node;
static Node nd[256]; static int nn;

static int build(int *ix, int nx, int d)
{
    int m = nn++;
    if (nx <= 1 || d >= D) { nd[m].qa = ix[0]; nd[m].l = nd[m].r = -1; return m; }
    unsigned char qv[N_QA][N];
    for (int i=0;i<N_QA;i++) encode(qs[i], qv[i]);

    int bb=0, bv=qv[ix[0]][0], bs=99999;
    for (int b=0;b<N;b++) for (int i=0;i<nx;i++) {
        unsigned char val=qv[ix[i]][b]; int nl=0,nr=0;
        for(int j=0;j<nx;j++){if(qv[ix[j]][b]<=val)nl++;else nr++;}
        int sc=(nl>nr)?nl-nr:nr-nl; if(sc<bs&&nl>0&&nr>0){bs=sc;bb=b;bv=val;}
    }
    nd[m].bp=bb; nd[m].rv=bv;
    int *L=malloc(nx*4),*R=malloc(nx*4),nl=0,nr=0;
    for(int i=0;i<nx;i++){if(qv[ix[i]][bb]<=bv)L[nl++]=ix[i];else R[nr++]=ix[i];}
    nd[m].l=build(L,nl,d+1); nd[m].r=build(R,nr,d+1); free(L);free(R);
    return m;
}

static int route(const unsigned char *v)
{
    int n=0;
    while(nd[n].l>=0) n=(v[nd[n].bp]<=nd[n].rv)?nd[n].l:nd[n].r;
    return nd[n].qa;
}

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  BoolNet v2 — 64 Q&A 纯布尔对话        ║\n");
    printf("║  N-gram hash + 深度6路由树 + XOR输出    ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 1. Build tree */
    int all[N_QA]; for(int i=0;i<N_QA;i++)all[i]=i;
    build(all,N_QA,0);
    printf("路由树: %d 节点 (%d 内部 + %d 叶子)\n\n", nn, nn-N_QA, N_QA);

    /* 2. Compute leaf routers: rb = qv XOR av (闭式解) */
    unsigned char qv[N_QA][N], av[N_QA][N], rb[N_QA][N];
    for(int i=0;i<N_QA;i++){
        encode(qs[i],qv[i]); encode(as[i],av[i]);
        for(int j=0;j<N;j++) rb[i][j]=qv[i][j]^av[i][j];
    }

    /* 3. Evaluation */
    printf("=== 训练集评估 ===\n");
    int ok=0;
    for(int p=0;p<N_QA;p++){
        unsigned char in[N],out[N];
        encode(qs[p],in); int qi=route(in);
        for(int i=0;i<N;i++) out[i]=in[i]^rb[qi][i];
        int correct=(qi==p)&&!memcmp(out,av[p],N);
        if(correct)ok++;
        if(p<8||p>=N_QA-4)  /* 显示首尾 */
            printf("  Q%-2d %-16s → leaf[%d] %s\n",p,qs[p],qi,correct?"✅":"❌");
    }
    printf("  ... (省略中间 %d 项)\n", N_QA-12);
    printf("准确率: %d/%d (%.0f%%)\n\n", ok, N_QA, 100.0*ok/N_QA);

    /* 4. 模糊匹配测试 */
    printf("=== 模糊匹配测试 ===\n");
    const char *tests[] = {"helo","hi there","bye bye","who r u",
                           "wat is boolnet","goddbye","thenks","traing",
                           "hel","sav","lod","conv",NULL};
    for(int t=0;tests[t];t++){
        unsigned char in[N],out[N];
        encode(tests[t],in); int qi=route(in);
        for(int i=0;i<N;i++) out[i]=in[i]^rb[qi][i];
        printf("  \"%s\" → leaf[%d](\"%s\"): ", tests[t], qi, qs[qi]);
        for(int i=0;i<N&&out[i];i++) if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf("\n");
    }

    /* 5. Interactive */
    printf("\n═══ 实时对话 ═══\n(64 QA, 输入/list/quit)\n");
    char in[256];
    while(1){
        printf("\n> "); fflush(stdout);
        if(!fgets(in,sizeof(in),stdin))break; in[strcspn(in,"\n")]=0;
        if(!in[0])continue;
        if(!strcmp(in,"quit")){printf("再见!\n");break;}
        if(!strcmp(in,"list")){for(int i=0;i<N_QA;i++)printf("  %s\n",qs[i]);continue;}

        unsigned char vec[N],out[N];
        encode(in,vec); int qi=route(vec);
        for(int i=0;i<N;i++) out[i]=vec[i]^rb[qi][i];

        printf("BoolBot: ");
        for(int i=0;i<N&&out[i];i++) if(out[i]>=32&&out[i]<127)putchar(out[i]);
        printf("\n");
    }
    return 0;
}
