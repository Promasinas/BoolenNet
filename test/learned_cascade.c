/**
 * learned_cascade.c — 逐层训练的布尔级联路由树
 *
 * 每个节点 = 1 Router(32bit) + 1 Memory(1 cell)
 *   - Router XOR 匹配输入模式
 *   - Memory cell 值 > 128 → "走左边", ≤ 128 → "走右边"
 *
 * 二叉树：深度 D，叶子数 2^D。127 个路由器对应 128 个记忆槽。
 * 逐层训练：每层独立学习二分类决策。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N_BYTES  4
#define MAX_DEPTH 4   /* 2^4 = 16 leaf slots */

/* ---- 训练单个路由器：学习二分类决策 ---- */
static int train_single_router(BoolRouter *r, MemIntLayer *m,
    const uint8_t *inputs, const uint8_t *labels, int n_samples,
    int pos_class)  /* 1 = 该节点应触发(left), 0 = 不应触发(right) */
{
    /* 构建 mini 网络 */
    BoolNet *net = boolnet_create(99, N_BYTES, 2);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r,
        bool_router_forward_layer, NULL);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m,
        mem_int_forward_output, NULL);

    /* 为每个样本生成目标 */
    uint8_t *tg = (uint8_t*)malloc(n_samples);
    for (int i = 0; i < n_samples; i++)
        tg[i] = (labels[i] == pos_class) ? 0xFF : 0x00;

    TsetlinTrainer *t = tsetlin_create(net, 3000, 255);
    int accepted = 0;

    for (int ep = 0; ep < 3000; ep++) {
        int idx = rand() % n_samples;
        int rc = tsetlin_train_step(t, &inputs[idx * N_BYTES], &tg[idx]);
        if (rc == 1) accepted++;
        if (rc == -2) break;
    }

    /* 评估 */
    int ok = 0;
    for (int i = 0; i < n_samples; i++) {
        mem_int_reset(m);
        uint8_t out[1];
        boolnet_forward(net, &inputs[i * N_BYTES], out);
        int pred = (out[0] > 128) ? 1 : 0;
        if (pred == labels[i]) ok++;
    }

    printf("    准确率: %d/%d (%.0f%%)  |  接受翻转: %d  |  路由bits: [%02X %02X %02X %02X]\n",
           ok, n_samples, 100.0f*ok/n_samples, accepted,
           r->bits[0], r->bits[1], r->bits[2], r->bits[3]);

    tsetlin_destroy(t); boolnet_destroy(net); free(tg);
    return ok;
}

/* ---- 生成训练数据：每种模式一个 class ---- */
#define N_PATS 16
static uint8_t inputs[N_PATS * N_BYTES];
static uint8_t all_labels[N_PATS];

static void make_patterns(void)
{
    /* 生成全部 16 种组合：每个字节可以是 0x00 或 0xFF */
    for (int p = 0; p < N_PATS; p++) {
        for (int b = 0; b < N_BYTES; b++)
            inputs[p * N_BYTES + b] = (p & (1 << b)) ? 0xFF : 0x00;
        all_labels[p] = p;
    }
    printf("数据: %d 个模式 (全部 4-bit 组合)\n\n", N_PATS);
}

/* ---- 递归构建和训练树 ---- */
typedef struct TreeNode {
    int depth; int node_id;
    BoolRouter *router; MemIntLayer *memory;
    int *samples; int n_samples;  /* 该节点负责的样本索引 */
    struct TreeNode *left, *right;
} TreeNode;

static int next_uid = 1;
TreeNode nodes[256]; int n_nodes = 0;

static TreeNode* build_tree(int depth, int max_depth,
    int *sample_indices, int n_samples)
{
    TreeNode *nd = &nodes[n_nodes++];
    nd->depth = depth; nd->node_id = n_nodes;
    nd->samples = sample_indices; nd->n_samples = n_samples;

    if (depth >= max_depth) {
        nd->left = nd->right = NULL;
        return nd;
    }

    uint8_t zb[N_BYTES] = {0};
    nd->router  = bool_router_create((LayerUID)(next_uid++), N_BYTES*8, zb);
    nd->memory  = mem_int_create((LayerUID)(next_uid++), ML_PRECISION_UINT8, 1, 255, 0);

    /* 按特征分裂：byte[depth] 有 FF → left(1), 没有 → right(0) */
    uint8_t *node_labels = (uint8_t*)malloc(n_samples);
    int split_byte = depth % N_BYTES;  /* 每层关注不同字节位置 */
    for (int i = 0; i < n_samples; i++) {
        int si = sample_indices[i];
        node_labels[i] = (inputs[si * N_BYTES + split_byte] > 0) ? 1 : 0;
    }

    /* 训练这个节点的路由器 */
    printf("  [深度 %d, 节点 %d] 训练: %d 样本 → 二分类\n",
           depth, nd->node_id, n_samples);
    train_single_router(nd->router, nd->memory,
        inputs, node_labels, n_samples, 1);

    /* 递归分裂 */
    int *left_idx = (int*)malloc(n_samples * sizeof(int));
    int *right_idx = (int*)malloc(n_samples * sizeof(int));
    int nl = 0, nr = 0;
    for (int i = 0; i < n_samples; i++) {
        if (node_labels[i] == 1) left_idx[nl++] = sample_indices[i];
        else right_idx[nr++] = sample_indices[i];
    }

    nd->left  = (nl > 0) ? build_tree(depth+1, max_depth, left_idx, nl) : NULL;
    nd->right = (nr > 0) ? build_tree(depth+1, max_depth, right_idx, nr) : NULL;

    free(node_labels);
    return nd;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Boolean Cascade Routing Tree           ║\n");
    printf("║  Depth=%d, %d leaves                    ║\n", MAX_DEPTH, 1<<MAX_DEPTH);
    printf("║  Each node = Router(32b) + Memory(1c)   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    make_patterns();

    int all_idx[N_PATS];
    for (int i = 0; i < N_PATS; i++) all_idx[i] = i;

    printf("=== 逐层训练级联树 ===\n");
    build_tree(0, MAX_DEPTH, all_idx, N_PATS);

    /* ---- 构建完整网络用于推理 ---- */
    printf("\n=== 构建完整级联网络 ===\n");
    BoolNet *full_net = boolnet_create(1, N_BYTES, n_nodes * 2);

    /* 按深度优先遍历添加层 */
    int layer_uid = 1;
    for (int i = 0; i < n_nodes; i++) {
        if (nodes[i].router) {
            boolnet_add_layer(full_net, LAYER_ROUTER, layer_uid++,
                nodes[i].router, bool_router_forward_layer, NULL);
            boolnet_add_layer(full_net, LAYER_MEMORY, layer_uid++,
                nodes[i].memory, mem_int_forward_output, NULL);
        }
    }
    printf("完整网络: %u 层\n\n", full_net->num_layers);

    /* ---- 推理测试 ---- */
    printf("═══ 推理测试 ═══\n");
    int ok = 0;
    for (int p = 0; p < N_PATS; p++) {
        uint8_t out[1];
        /* 重置所有 memory */
        for (uint32_t i = 0; i < full_net->num_layers; i++)
            if (full_net->layers[i].type == LAYER_MEMORY)
                mem_int_reset((MemIntLayer*)full_net->layers[i].instance);

        boolnet_forward(full_net, &inputs[p * N_BYTES], out);

        /* 最终的 memory cell 值区分不同路径 */
        printf("  P%d (byte[%d]=FF): cell=%3d\n",
               p, p % N_BYTES, out[0]);
    }

    boolnet_destroy(full_net);
    printf("\n节点数: %d  |  路由器数: %d\n", n_nodes, n_nodes / 2);
    return 0;
}
