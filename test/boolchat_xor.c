/**
 * boolchat_xor.c — XOR Dialogue Bot with Coordinate Descent
 *
 * Pure XOR: router_bits = input ^ target (closed form).
 * Coordinate descent finds this in O(2N) without Memory layer bottleneck.
 * Text output requires arbitrary byte values → skip Memory, use direct XOR.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 64
#define N_PAIRS 12

static const char *inputs[N_PAIRS] = {
    "hello", "hi", "how are you", "who are you",
    "what is boolnet", "bye", "thanks", "good morning",
    "good night", "help", "weather", "tell me a joke"
};
static const char *outputs[N_PAIRS] = {
    "Hello! Nice to meet you!",
    "Hi there! How can I help?",
    "I am doing great, thanks for asking!",
    "I am BoolBot, a Boolean network chatbot.",
    "BoolNet uses XOR routers and Tsetlin learning.",
    "Goodbye! Have a wonderful day!",
    "You are welcome! Glad to help.",
    "Good morning! Ready for a great day?",
    "Good night! Sleep well and dream big.",
    "Commands: type and chat. I learn from context.",
    "I cannot check weather, but it is sunny in BoolNet!",
    "Why did the Boolean cross the road? To XOR the other side!"
};

static void encode(const char *s, uint8_t *v) {
    memset(v, 0, N); int len=(int)strlen(s);
    for(int i=0;i<len&&i<N;i++) v[i]=(uint8_t)s[i];
    v[N-2]=(uint8_t)(len&0xFF); v[N-1]=(uint8_t)((len>>8)&0xFF);
}
static void decode(const uint8_t *v, char *o, int m) {
    int j=0; for(int i=0;i<N&&j<m-1;i++){if(v[i]>=32&&v[i]<127)o[j++]=(char)v[i];else if(!v[i])break;} o[j]=0;
}
/* Coordinate descent on raw XOR bits. Pure integer, no BoolNet deps. */
static int coord_optimize(uint8_t *router, const uint8_t *in, const uint8_t *tg) {
    int changed=1, sweeps=0;
    while(changed && sweeps<5) {
        changed=0; sweeps++;
        for(int byte=0; byte<N; byte++) {
            for(int bit=0; bit<8; bit++) {
                uint8_t mask = (uint8_t)(1u << bit);
                /* Current distance */
                int before=0;
                for(int i=0;i<N;i++){uint8_t x=(in[i]^router[i])^tg[i];while(x){before+=x&1;x>>=1;}}
                /* Flip bit */
                router[byte] ^= mask;
                int after=0;
                for(int i=0;i<N;i++){uint8_t x=(in[i]^router[i])^tg[i];while(x){after+=x&1;x>>=1;}}
                if(after >= before) { router[byte] ^= mask; } /* revert if worse */
                else { changed++; }
            }
        }
    }
    return sweeps;
}

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== BoolChat XOR: Coordinate Descent ===\n");
    printf("%d pairs, %d bytes each\n\n", N_PAIRS, N);

    uint8_t qv[N_PAIRS][N], av[N_PAIRS][N], routers[N_PAIRS][N];
    for(int i=0;i<N_PAIRS;i++){encode(inputs[i],qv[i]);encode(outputs[i],av[i]);memset(routers[i],0,N);}

    clock_t t0=clock(); int total_sw=0;

    for(int p=0;p<N_PAIRS;p++){
        int sw=coord_optimize(routers[p], qv[p], av[p]);
        total_sw+=sw;
        /* Verify */
        uint8_t out[N]; for(int i=0;i<N;i++) out[i]=qv[p][i]^routers[p][i];
        char txt[128]; decode(out,txt,128);
        int dist=0; for(int i=0;i<N;i++){uint8_t x=out[i]^av[p][i];while(x){dist+=x&1;x>>=1;}}
        printf("  [%d] \"%s\" -> \"%s\" %s\n",p,inputs[p],txt,dist==0?"PERFECT":"");
    }
    printf("\n%lld ms, %d sweeps total\n\n",(long long)((clock()-t0)*1000/CLOCKS_PER_SEC),total_sw);

    /* Chat */
    printf("=== Chat (quit/list) ===\n");
    char in[256];
    while(1){printf("\n> ");fflush(stdout);if(!fgets(in,sizeof(in),stdin))break;in[strcspn(in,"\n")]=0;
        if(!in[0])continue;if(!strcmp(in,"quit")){printf("Bye!\n");break;}
        if(!strcmp(in,"list")){for(int i=0;i<N_PAIRS;i++)printf("  %s\n",inputs[i]);continue;}
        uint8_t v[N]; encode(in,v);
        int bp=0,bd=999999;
        for(int p=0;p<N_PAIRS;p++){int d=0;for(int i=0;i<N;i++){uint8_t x=v[i]^qv[p][i];while(x){d+=x&1;x>>=1;}}if(d<bd){bd=d;bp=p;}}
        uint8_t o[N]; for(int i=0;i<N;i++) o[i]=v[i]^routers[bp][i];
        char rp[128]; decode(o,rp,128);
        printf("Bot: %s\n(dist=%d via \"%s\")\n",rp[0]?rp:"(no match)",bd,inputs[bp]);
    }
    return 0;
}
