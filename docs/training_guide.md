# BoolNet 训练指南

## 训练架构

BoolNet 使用 Tsetlin 自动机进行训练。核心算法：

```
for each training step:
    1. 随机选一层 (Router 或 Conv1D)
    2. 随机翻转一个 bit
    3. Forward 传播 (Memory reset → 逐层 forward)
    4. 计算 reward = byte_max - sum(|output - target|)
    5. 如果 reward 提高 → 保留翻转; 否则 → 回退
    6. 如果连续失败超过 neg_tolerance → 停止
```

## 快速训练

### 单步训练 (已验证收敛)

```bash
cd test
gcc -std=c99 -O2 -I../src train_test.c ../src/*.o -o train_test.exe -lm
./train_test.exe
```

输出：
```
=== BoolNet Boolean Training ===
Input [01,01,01,01] → Target [01,00,00,00]
Step 1000: LEARNED!  Router bits: [00 01 01 01]
Best reward: 1020 (perfect)
```

### 完整训练流程

```c
#include "tsetlin_train.h"

// 1. 构建网络
BoolNet *net = boolnet_create(uid, input_dim, max_layers);
// ... 添加层 ...

// 2. 配置训练
TrainConfig cfg = {
    .max_epochs       = 50,     // 最大轮数
    .steps_per_epoch  = 2000,   // 每轮步数
    .neg_tolerance    = 10000,  // 连续负奖励容忍
    .byte_max         = 1020,   // 奖励上限
    .early_stop_patience = 10,   // 早停耐心
    .checkpoint_every = 10,     // 每 N 轮保存检查点
    .checkpoint_dir   = "weights/"
};

// 3. 训练
TsetlinTrainer *t = tsetlin_create(net, cfg.neg_tolerance, cfg.byte_max);
int best_val = tsetlin_train_full(t, &cfg,
    train_in, train_tg, num_train,   // 训练数据
    val_in,   val_tg,   num_val,     // 验证数据
    input_dim, output_dim, reset_fn);

// 4. 导出
tsetlin_export_model(net, "weights/model.bin");
```

## 训练参数调优

| 参数 | 说明 | 建议值 |
|------|------|--------|
| `max_epochs` | 最大训练轮数 | 50-200 |
| `steps_per_epoch` | 每轮训练步数 | sample_count × 100 |
| `neg_tolerance` | 连续负奖励停止阈值 | sample_count × 500 |
| `byte_max` | 奖励范围 | output_dim × 255 |
| `early_stop_patience` | 无改善即停的轮数 | 10-30 |

## 级联树训练

对于多模式分类，使用级联树结构：

```
Layer 0: Router₀ → Memory₀ (决策: 左/右)
Layer 1: Router₁ → Memory₁ (细分)
...
Leaf:   Routerₙ → Memoryₙ (输出)
```

每层独立训练为二分类器。详见 `test/learned_cascade.c`。

## 模型导出

```c
// 导出完整网络
tsetlin_export_model(net, "weights/my_model.bin");

// 导出单个 Memory 状态
mem_int_save(memory_layer, "weights/memory_001.bin");

// 加载
MemIntLayer *loaded = mem_int_load("weights/memory_001.bin");
```

## 训练日志

训练过程输出格式：
```
=== BoolNet Training ===
Epochs: 50  Steps/epoch: 2000  Samples: 4  Val: 4

  Epoch    1 | ok:    45 (2%) | val: 0/4 (0%) ★
  Epoch    2 | ok:    50 (3%) | val: 0/4 (0%)
  ...
  Early stop

Done: epochs=9 steps=18000 ok=384 (2%) best_val=0/4
```

- `ok`: 该轮接受的翻转次数
- `val`: 验证集准确率
- `★`: 新的最佳模型
- `Early stop`: 触发早停
