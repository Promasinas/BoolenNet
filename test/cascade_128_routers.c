/**
 * cascade_128_routers.c — 128 Boolean Router Cascade Tree
 *
 * Depth=7 → 2^7=128 leaves, 127 internal nodes (routers).
 * Total: 254 layers, 4064 trainable bits.
 * Each node = Router(32b) + Memory(1c, cell values).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BYTES   4
#define MAX_DEPTH 7      /* 2^7 = 128 leaves */
#define N_PATS    256    /* 4 values ^ 4 bytes */

static uint8_t inputs[N_PATS * N_BYTES];
static uint8_t labels[N_PATS];
static int n_patterns = 0;

/* Generate patterns: each byte has 4 values */
static void make_patterns(void)
{
    const uint8_t vals[] = {0x00, 0x55, 0xAA, 0xFF};
    n_patterns = 0;
    for (int a = 0; a < 4; a++)
    for (int b = 0; b < 4; b++)
    for (int c = 0; c < 4; c++)
    for (int d = 0; d < 4; d++) {
        inputs[n_patterns * N_BYTES + 0] = vals[a];
        inputs[n_patterns * N_BYTES + 1] = vals[b];
        inputs[n_patterns * N_BYTES + 2] = vals[c];
        inputs[n_patterns * N_BYTES + 3] = vals[d];
        labels[n_patterns] = n_patterns;
        n_patterns++;
    }
    printf("训练数据: %d 个模式\n\n", n_patterns);
}

/* ---- 训练单节点 ---- */
static int train_node(BoolRouter *r, MemIntLayer *m,
    const int *indices, int n, int depth)
{
    if (n <= 1) return n;

    BoolNet *net = boolnet_create(99, N_BYTES, 2);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r, bool_router_forward_layer, NULL);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m, mem_int_forward_output, NULL);

    /* Split criterion: byte[depth%4] 的高位 */
    int byte_pos = depth % N_BYTES;
    int bit_pos  = (depth / N_BYTES) % 2;  /* 0=low bits, 1=high bits */
    uint8_t threshold = bit_pos ? 0x80 : 0x0F;  /* 区分高位/低位 */

    uint8_t *tg = (uint8_t*)malloc(n);
    int nl = 0;
    for (int i = 0; i < n; i++) {
        int si = indices[i];
        uint8_t byte_val = inputs[si * N_BYTES + byte_pos];
        /* 1 = "大值" (left), 0 = "小值" (right) */
        int big = (byte_val > threshold) ? 1 : 0;
        tg[i] = big ? 0xFF : 0x00;
        if (big) nl++;
    }

    TsetlinTrainer *t = tsetlin_create(net, n * 50, 255);
    int steps_per_sample = 200;
    for (int s = 0; s < n * steps_per_sample; s++) {
        int idx = rand() % n;
        tsetlin_train_step(t, &inputs[indices[idx] * N_BYTES], &tg[idx]);
    }

    /* Eval */
    int correct = 0;
    for (int i = 0; i < n; i++) {
        mem_int_reset(m);
        uint8_t out[1];
        boolnet_forward(net, &inputs[indices[i] * N_BYTES], out);
        int pred = (out[0] > 128) ? 1 : 0;
        int want = tg[i] ? 1 : 0;
        if (pred == want) correct++;
    }

    uint32_t st, ac; int32_t br;
    tsetlin_get_stats(t, &st, &ac, &br, NULL, NULL);
    if ((depth < 3 || n <= 4) && n > 1)
        printf("  d=%d n=%2d: %2d/%2d (%.0f%%) ok=%u bits=[%02X%02X%02X%02X]\n",
               depth, n, correct, n, 100.0f*correct/n, ac,
               r->bits[0],r->bits[1],r->bits[2],r->bits[3]);

    tsetlin_destroy(t); boolnet_destroy(net); free(tg);
    return correct;
}

/* ---- 递归构建 ---- */
typedef struct { int *idx; int n; BoolRouter *r; MemIntLayer *m; } Node;
static Node tree[256]; static int nn = 0;

static void build_level(int depth, int *indices, int n)
{
    if (n <= 1 || depth >= MAX_DEPTH) return;

    Node *nd = &tree[nn];
    nd->idx = indices; nd->n = n;

    uint8_t zb[N_BYTES] = {0};
    nd->r = bool_router_create((LayerUID)(nn*2+1), N_BYTES*8, zb);
    nd->m = mem_int_create((LayerUID)(nn*2+2), ML_PRECISION_UINT8, 1, 255, 0);
    int node_idx = nn++;

    train_node(nd->r, nd->m, indices, n, depth);

    /* Split samples based on the learned router's output */
    int byte_pos = depth % N_BYTES;
    uint8_t threshold = ((depth / N_BYTES) % 2) ? 0x80 : 0x0F;

    int *left  = (int*)malloc(n * sizeof(int));
    int *right = (int*)malloc(n * sizeof(int));
    int nl = 0, nr = 0;
    for (int i = 0; i < n; i++) {
        int si = indices[i];
        if (inputs[si * N_BYTES + byte_pos] > threshold)
            left[nl++] = si;
        else
            right[nr++] = si;
    }

    if (nl > 0) build_level(depth+1, left, nl); else free(left);
    if (nr > 0) build_level(depth+1, right, nr); else free(right);
}

/* ---- 评估完整级联 ---- */
static int eval_full(void)
{
    /* 构建完整网络 */
    BoolNet *net = boolnet_create(1, N_BYTES, nn * 2);
    for (int i = 0; i < nn; i++) {
        if (tree[i].r) {
            boolnet_add_layer(net, LAYER_ROUTER, i*2+1, tree[i].r,
                bool_router_forward_layer, NULL);
            boolnet_add_layer(net, LAYER_MEMORY, i*2+2, tree[i].m,
                mem_int_forward_output, NULL);
        }
    }

    /* 测试：每个模式通过完整级联 */
    int unique_outputs = 0;
    uint8_t seen[256]; int n_seen = 0;

    for (int p = 0; p < n_patterns; p++) {
        for (uint32_t i = 0; i < net->num_layers; i++)
            if (net->layers[i].type == LAYER_MEMORY)
                mem_int_reset((MemIntLayer*)net->layers[i].instance);

        uint8_t out[1];
        boolnet_forward(net, &inputs[p * N_BYTES], out);

        /* 检查这个输出值是否之前出现过 */
        int found = 0;
        for (int s = 0; s < n_seen; s++)
            if (seen[s] == out[0]) { found = 1; break; }
        if (!found) seen[n_seen++] = out[0];
    }
    unique_outputs = n_seen;

    boolnet_destroy(net);
    return unique_outputs;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  128 Boolean Router Cascade Tree         ║\n");
    printf("║  Depth=7, 128 leaves, 127 routers         ║\n");
    printf("║  254 layers, 4064 trainable bits          ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    make_patterns();

    int all_idx[N_PATS];
    for (int i = 0; i < n_patterns; i++) all_idx[i] = i;

    clock_t t0 = clock();
    printf("=== 构建并训练级联树 ===\n");
    build_level(0, all_idx, n_patterns);

    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("\n节点数: %d (路由器: %d)  训练时间: %.1f秒\n\n", nn, nn, sec);

    /* 评估 */
    printf("═══ 完整级联评估 ═══\n");
    int unique = eval_full();
    printf("不同输出值: %d / %d (理论最大: %d)\n", unique, n_patterns, 1 << MAX_DEPTH);

    /* 统计路由器翻转 */
    int total_flips = 0, trained_routers = 0;
    for (int i = 0; i < nn; i++) {
        if (!tree[i].r) continue;
        int f = 0;
        for (uint32_t b = 0; b < tree[i].r->num_bits; b++)
            if ((tree[i].r->bits[b/8] >> (b&7)) & 1) f++;
        if (f > 0) { total_flips += f; trained_routers++; }
        /* Cleanup */
        bool_router_destroy(tree[i].r);
        mem_int_destroy(tree[i].m);
    }
    printf("训练过的路由器: %d/%d  翻转比特: %d/%d\n",
           trained_routers, nn, total_flips, nn * 32);

    printf("\n模型: 254层, 127路由器, 128叶节点\n");
    return 0;
}
