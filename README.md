# BoolNet — Pure Boolean Neural Network Framework

[![C99](https://img.shields.io/badge/C-C99-blue)](src/)
[![Build](https://img.shields.io/badge/build-make-green)](src/Makefile)
[![Tests](https://img.shields.io/badge/tests-94%2F94-brightgreen)](docs/test/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20MinGW-lightgrey)]()
[![Modules](https://img.shields.io/badge/modules-8-blue)](src/)

**BoolNet** 是一个完全基于布尔运算的神经网络框架。不使用反向传播或梯度下降，
而是通过**布尔路由器**、**Tsetlin 自动机**和**级联记忆层**实现学习和推理。

---

## 核心思想

```
Byte Stream → [Boolean Router] → [Circular Conv1D] → [Memory Layer]
           → [Multi-Router Upsampling] → [Memory Layer] → Output
```

- **无梯度**：Tsetlin 自动机按位翻转 + 奖励函数替代反向传播
- **O(1) 路由**：布尔级联将搜索变为常数时间硬路由
- **纯一维**：所有数据流为 `uint8_t` 字节序列，天然适合一维硬件加速
- **Router 路由 + Memory 放大**：Router 决定"去哪"，Memory 决定"值多少"
- **可解释**：每个路由器 bit 可直接阅读和调试
- **完整持久化**：`TENB` 二进制格式，训练→保存→加载→推理全流程

---

## 架构 (宪法 IV→X)

| # | 模块 | 功能 | 源码 |
|---|------|------|------|
| IV | Boolean Router | 逐位 XOR：`out[i] = in[i] ^ bits[i]`，Tsetlin 可训练 | [`src/bool_router/`](src/bool_router/) |
| V | 1D Circular Conv | 布尔核回环卷积，`kernel_bits` 逐位训练 | [`src/conv1d_circular/`](src/conv1d_circular/) |
| VI | Multi-Router Upsample | M 个路由器并行 → M×N 拼接，支持级联 | [`src/upsampling/`](src/upsampling/) |
| VIII | Integer Memory | uint8/16/32/64，触发器恢复 + 衰减 + 持久化 | [`src/mem_int/`](src/mem_int/) |
| IX | Forward & Topology | 按 UID 层注册 + 串行传播 + 完整网络 save/load | [`src/boolnet/`](src/boolnet/) |
| X | Tsetlin Training | 逐 bit 翻转 + 奖励驱动 + 早停 + 检查点 + 导出 | [`src/tsetlin_train/`](src/tsetlin_train/) |
| — | Partition Manager | 浮点分区存储 + LRU + 稀疏触发加权和 | [`src/mem_part/`](src/mem_part/) |
| 🆕 | **Simple LLM** | Router→Memory→Router→Memory 对话模型 | [`src/simple_llm/`](src/simple_llm/) |

---

## 快速开始

### 要求

- GCC (MinGW/MSYS2 on Windows)
- GNU Make
- C99+ 标准库

### 编译 & 运行

```bash
cd src/
make all              # 编译全部 8 模块

# 训练测试
make train_test       # 最小网络 (2 层, 10k 步)
make boolnet_train    # 分类网络 (4 层, 20k 步)
make full_train       # Train→Save→Load→Infer (5 层)
make llm_test         # 序列训练 (6 层)
make net128           # 128 宽网络 (5 层, 261 可训位)
make train_pipeline   # 完整流水线 (验证+早停+导出)
make simple_llm       # 对话模型 (4 层, 256 可训位)

# 运行
../test/train_test.exe
../test/simple_llm.exe    # "hello" → "Hi there!"
```

---

## Simple LLM — 对话模型 🆕

BoolNet 首个语言模型。Router 学习路由，Memory 负责信号放大。

```
Input(16B one-hot) → Router(128b) → Memory(128c,decay=1)
                   → Router(128b) → Memory(16c,decay=0) → Output(token)
                     256 可训位
```

| 输入 | 输出 |
|------|------|
| "hello" | "Hi there!" |
| "how are you" | "I'm doing well" |
| "who are you" | "I'm BoolBot" |
| "bye" | "Goodbye!" |
| "thanks" | "You're welcome!" |

```c
// 训练
void *net = sl_build_network();
sl_train(net, "weights/simple_llm_model.bin");

// 推理
int out_idx; uint8_t out_vals[16];
sl_infer(net, 0, &out_idx, out_vals);  // "hello" → "Hi there!"
```

---

## 模型持久化

### TENB 格式 (magic: `0x424E4554`)

```
[magic:4B][version:4B][num_layers:4B][input_dim:4B][output_dim:4B]
每层:
  Router: [num_bits:4B][bits: ceil(nb/8) bytes]
  Conv1D: [input_len:4B][kernel_size:4B][stride:4B][dilation:4B][bits]
  Memory: [precision:1B][length:4B][max_value:8B][decay:8B][cells]
```

### 使用

```c
// 导出
tsetlin_export_model(net, "weights/my_model.bin");

// 导入 → 直接推理
BoolNet *loaded = tsetlin_import_model("weights/my_model.bin");
boolnet_forward(loaded, input, output);
```

模型文件统一存放在 [`weights/`](weights/) 目录。

---

## 训练框架

```c
TrainConfig cfg = {
    .max_epochs          = 30,
    .steps_per_epoch     = 2000,
    .neg_tolerance       = 5000,
    .byte_max            = 4080,
    .early_stop_patience = 8,
    .checkpoint_every    = 5,
    .eval_every          = 100,
    .checkpoint_dir      = "weights/"
};

tsetlin_train_full(trainer, &cfg,
    train_inputs, train_targets, num_train,
    val_inputs,   val_targets,   num_val,
    input_dim, output_dim, reset_memory_fn);
```

支持：**早停**、**检查点自动保存**、**验证集评估**、**最佳模型导出**。

---

## 测试结果

全部 **94 项测试通过**（Agent 3 自动验证）：

| UID | 模块 | 测试数 | 通过率 |
|-----|------|--------|--------|
| 1 | bool_router | 10 | 100% |
| 2 | mem_int (Memory Layer) | 16 | 100% |
| 3 | mem_part (Partition Mgr) | 13 | 100% |
| 4 | conv1d_circular | 10 | 100% |
| 5 | upsampling | 12 | 100% |
| 6 | boolnet (Forward) | 8 | 100% |
| 7 | tsetlin_train | 12 | 100% |
| — | simple_llm | 5 | 100% |
| — | 集成测试 (8 程序) | 8 | 100% |

报告：[`docs/test/`](docs/test/)

---

## 项目结构

```
BoolNet/
├── README.md
├── CLAUDE.md
│
├── src/                          # 源码 (~5000 行 C)
│   ├── Makefile                  # 15 个构建目标
│   ├── common/                   # 共享类型
│   ├── bool_router/              # IV: 布尔路由器
│   ├── conv1d_circular/          # V:  回环卷积
│   ├── upsampling/               # VI: 多路由上采样
│   ├── mem_int/                  # VIII: 整数 Memory 层
│   ├── mem_part/                 # 浮点分区管理器
│   ├── boolnet/                  # IX: Forward + 拓扑
│   ├── tsetlin_train/            # X: Tsetlin 训练框架
│   └── simple_llm/               # 🆕 对话模型
│
├── test/                         # 测试程序 (11 个)
│   ├── train_test.c              # 最小训练 (2 层)
│   ├── boolnet_train.c           # 分类训练 (4 层)
│   ├── full_train_save.c         # 保存/加载验证 (5 层)
│   ├── llm_network_test.c        # 序列训练 (6 层)
│   ├── net128_train.c            # 128 宽网络 (5 层)
│   ├── train_pipeline.c          # 完整流水线
│   ├── cascade_128.c             # 级联上采样
│   ├── deep_router_mem.c         # 深层 Router↔Memory
│   ├── simple_llm.exe            # 🆕 对话模型
│   └── uid_*/                    # Agent 3 单元测试 (7 模块)
│
├── weights/                      # 🆕 模型权重目录
│   ├── README.md                 # 格式规范
│   └── *.bin                     # 训练产出 (TENB 格式)
│
├── docs/                         # 文档
│   ├── api/                      # API 参考
│   ├── interact/                 # Agent 间通信
│   ├── test/                     # 测试报告
│   └── history/                  # 设计决策历史
│
└── specs/                        # 规格文档 (8 features)
    ├── 001-agent1-design/
    ├── 002-agent2-implementation/
    ├── 003-agent3-testing/
    ├── 004-bool-router/
    ├── 005-circular-conv/
    ├── 006-upsampling/
    ├── 007-forward/
    └── 008-tsetlin-train/
```

---

## 三 Agent 协作

| Agent | 角色 | 职责 | 输出 |
|-------|------|------|------|
| **Agent 1** | 主控/设计 | 用户沟通、架构设计、规格文档 | `docs/interact/` |
| **Agent 2** | 代码生成 | 读取设计 → 生成 C 代码 → 审查修复 | `src/`, `docs/api/` |
| **Agent 3** | 自动化测试 | 检测变更 → 编译运行 → 报告 → Git | `test/`, `docs/test/` |

```
Agent 1 (设计) → docs/interact/ → Agent 2 (编码) → src/
                                    ↓
Agent 3 (测试) ← docs/interact/ ← Agent 2 (通知)
    ↓
docs/test/ → Agent 2 (修复) → Agent 3 (再测试) → ✅
```

---

## 许可证

MIT License
