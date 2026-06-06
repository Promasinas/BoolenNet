# BoolNet — Pure Boolean Neural Network Framework

[![C99](https://img.shields.io/badge/C-C99-blue)](src/)
[![Build](https://img.shields.io/badge/build-gcc-green)](src/Makefile)
[![Tests](https://img.shields.io/badge/tests-84%2F84-brightgreen)](docs/test/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20MinGW-lightgrey)]()
[![Modules](https://img.shields.io/badge/modules-7-blue)](src/)

**BoolNet** 是一个完全基于布尔运算的神经网络框架。不使用反向传播或梯度下降，
而是通过**布尔路由器**、**Tsetlin 自动机**和**级联记忆层**实现学习和推理。

---

## 5 分钟快速开始

### 1. 运行对话机器人 (exe)

```bash
# Windows: 双击运行
release/start_chat.bat

# 或命令行
cd release && boolchat.exe
```

输入 `hello` 试试：

```
> hello
BoolBot: Hello! How can I help you?
(路由 leaf[0], 纯 XOR 计算, <1ms)
```

| 命令 | 功能 |
|------|------|
| `hello` / `who are you` / ... | 对话 (16 组内置 Q&A) |
| `list` | 查看所有可问的问题 |
| `quit` | 退出 |

### 2. 自己编译

```bash
# 纯布尔对话机器人 (推荐)
cd release
gcc -std=c99 -O2 boolchat.c -o boolchat.exe
./boolchat.exe

# 带 BoolNet 库的完整版 (含 Tsetlin 训练)
gcc -std=c99 -O2 -Isrc ../test/chatbot_demo.c \
    ../src/bool_router/bool_router.c \
    ../src/boolnet/boolnet.c \
    ../src/mem_int/mem_int_layer.c \
    ../src/tsetlin_train/tsetlin_train.c \
    ../src/conv1d_circular/conv1d_circular.c \
    ../src/upsampling/upsampling.c \
    -o chatbot_full.exe -lm
```

### 3. 训练自己的网络

```bash
cd test

# 最小训练示例 (2 层, 1 个模式, 已验证收敛)
gcc -std=c99 -O2 -I../src train_test.c ../src/*.o -o train.exe -lm
./train.exe
# 输出: Router bits learned, reward=1020 (perfect!)

# 完整训练流程 (5 层, Train→Save→Load→Infer)
gcc -std=c99 -O2 -I../src full_train_save.c ../src/*.o -o full.exe -lm
./full.exe
# 输出: 4/4 准确率, 模型保存到 weights/
```

### 4. 编写训练代码

```c
#include "tsetlin_train.h"

// 构建网络
BoolNet *net = boolnet_create(1, 4, 2);
BoolRouter *r = bool_router_create(1, 32, init_bits);
MemIntLayer *m = mem_int_create(2, ML_PRECISION_UINT8, 4, 255, 0);
boolnet_add_layer(net, LAYER_ROUTER, 1, r, ...);
boolnet_add_layer(net, LAYER_MEMORY, 2, m, ...);

// 训练
TsetlinTrainer *t = tsetlin_create(net, 5000, 1020);
uint8_t input[4]  = {0x01, 0x01, 0x01, 0x01};
uint8_t target[4] = {0x01, 0x00, 0x00, 0x00};

for (int s = 0; s < 10000; s++)
    tsetlin_train_step(t, input, target);

// 保存
tsetlin_export_model(net, "weights/my_model.bin");
```

---

## 核心思想

```
Byte Stream → [Boolean Router] → [Circular Conv1D] → [Memory Layer]
           → [Multi-Router Upsampling] → [Memory Layer] → Output
```

- **无梯度**：Tsetlin 自动机按位翻转 + 奖励函数替代反向传播
- **O(1) 路由**：布尔级联将搜索变为常数时间硬路由
- **纯布尔**：Router XOR 决定"去哪"，Memory 触发恢复决定"值多少"
- **可解释**：每个 Router bit 可直接阅读和调试
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

### 环境

- GCC 13+ (MinGW/MSYS2 on Windows)
- GNU Make (可选，可直接用 gcc 编译)
- C99+ 标准库

### 编译

```bash
# 方式 1: Makefile (需要 make)
cd src/ && make all

# 方式 2: 直接 gcc
gcc -std=c99 -Isrc/common -Isrc/bool_router -Isrc/mem_int \
    -Isrc/boolnet -Isrc/tsetlin_train -Isrc/conv1d_circular \
    -Isrc/upsampling -o test/program.exe \
    test/program.c \
    src/bool_router/bool_router.c src/mem_int/mem_int_layer.c \
    src/boolnet/boolnet.c src/tsetlin_train/tsetlin_train.c \
    src/conv1d_circular/conv1d_circular.c \
    src/upsampling/upsampling.c -lm
```

### 运行

```bash
# 单元测试 (7 模块, 84 项)
test/uid_001/t.exe   # 布尔路由器
test/uid_002/t.exe   # Memory 层
test/uid_003/t.exe   # 分区管理器
test/uid_004/t.exe   # 回环卷积
test/uid_005/t.exe   # 上采样
test/uid_006/t.exe   # Forward/拓扑
test/uid_007/t.exe   # Tsetlin 训练

# 集成训练测试
test/deep_cascade.exe     # 128 层 Router↔Memory 对 (100k 步)
test/net128_train.exe     # 128 维宽网络 (epoch + 早停)
test/full_train_save.exe  # Train→Save→Load→Infer 全流程
test/train_test.exe       # 最小分类训练
```

---

## Simple LLM — 对话模型 🆕

BoolNet 首个语言模型。Router 学习路由，Memory 负责信号放大。

```
Input(16B one-hot) → Router(128b) → Memory(128c,decay=1)
                   → Router(128b) → Memory(16c,decay=0) → Output(argmax)
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
void *net = sl_build_network();                    // 构建 4 层网络
sl_train(net, "weights/simple_llm_model.bin");     // Tsetlin 训练
int out; sl_infer(net, 0, &out, NULL);              // "hello" → argmax
```

---

## 模型持久化

### TENB 格式 (magic: `TENB`)

```
[magic:4B][version:4B][num_layers:4B][input_dim:4B][output_dim:4B]
每层:
  Router:  [num_bits:4B][bits: ceil(nb/8) bytes]
  Conv1D:  [input_len:4B][kernel_size:4B][stride:4B][dilation:4B][bits]
  Memory:  [precision:1B][length:4B][max_value:8B][decay:8B][cells]
```

```c
tsetlin_export_model(net, "weights/model.bin");  // 导出
BoolNet *loaded = tsetlin_import_model("weights/model.bin");  // 加载
```

模型文件统一存放在 [`weights/`](weights/)。

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

全部 **84 项单元测试 + 8 项集成测试通过**（Agent 3 验证）：

| UID | 模块 | 函数测试 | 通过率 |
|-----|------|---------|--------|
| 1 | bool_router | 13 | 100% |
| 2 | mem_int | 12 | 100% |
| 3 | mem_part | 16 | 100% |
| 4 | conv1d_circular | 13 | 100% |
| 5 | upsampling | 11 | 100% |
| 6 | boolnet | 9 | 100% |
| 7 | tsetlin_train | 10 | 100% |
| — | 集成训练 (6 程序) | 6 | ✅ 运行正常 |

报告：[`docs/test/`](docs/test/)

---

## 项目结构

```
BoolNet/
├── README.md                     # 本文件
├── CLAUDE.md                     # Claude Code 入口 (指向活跃 plan)
│
├── src/                          # BoolNet 框架源码 (~5000 行 C)
│   ├── Makefile                  # 15 个构建目标
│   ├── common/boolnet_common.h   # 共享类型 (LayerUID, 精度枚举, 错误码)
│   ├── bool_router/              # IV:  布尔路由器
│   ├── conv1d_circular/          # V:   回环卷积
│   ├── upsampling/               # VI:  多路由上采样
│   ├── mem_int/                  # VIII: 整数 Memory 层
│   ├── mem_part/                 #       浮点分区管理器
│   ├── boolnet/                  # IX:   Forward + 拓扑
│   ├── tsetlin_train/            # X:    Tsetlin 训练框架
│   └── simple_llm/               # 🆕   对话模型 (开发中)
│
├── agent3/                       # Agent 3 自动化测试引擎 (1925 行 C)
│   ├── agent3.c/h                #   主循环 + 流水线调度
│   ├── detector.c/h              #   文件变更检测
│   ├── module_tracker.c/h        #   模块状态 + 去重
│   ├── interact.c/h              #   Markdown 解析 + 通知生成
│   ├── testdir.c/h               #   测试目录创建
│   ├── makefile_gen.c/h          #   Makefile 模板生成
│   ├── compiler.c/h              #   编译调用
│   ├── runner.c/h                #   测试执行 + 超时
│   ├── parallel.c/h              #   并行流水线
│   ├── reporter.c/h              #   测试报告生成
│   ├── gitmgr.c/h                #   Git 版本管理
│   ├── heartbeat.c/h             #   心跳文件
│   └── Makefile                  #   Agent 3 自身构建
│
├── test/                         # 测试程序
│   ├── uid_001/ ~ uid_007/       # 7 模块 × 函数级单元测试
│   ├── deep_cascade.c            # 128 层 Router↔Memory 对 (100k 步)
│   ├── net128_train.c            # 128 维宽网络 (epoch + 早停)
│   ├── full_train_save.c         # Train→Save→Load→Infer 全流程
│   ├── boolnet_train.c           # 分类网络训练
│   ├── train_test.c              # 最小训练测试
│   └── llm_network_test.c        # 序列 LLM 测试
│
├── weights/                      # 模型权重目录
│   ├── best.bin                  # 最佳 checkpoint
│   ├── boolnet_model.bin         # 完整模型 (TENB 格式)
│   ├── deep128_model.bin         # 128 层深度网络
│   └── ckpt_*.bin                # 训练检查点
│
├── docs/                         # 文档
│   ├── api/                      # API 参考
│   ├── interact/                 # Agent 间通信 (设计文档/测试请求/通知)
│   ├── test/                     # 测试报告 (uid_NNN_report.md)
│   └── history/                  # 设计决策历史
│
├── specs/                        # 规格文档 (8 features, 121 tasks)
│   ├── 001-agent1-design/        # Agent 1: 主控/设计
│   ├── 002-agent2-implementation/# Agent 2: 代码生成
│   ├── 003-agent3-testing/       # Agent 3: 自动化测试
│   └── 004~008-*/                # 各模块规格 (spec/plan/tasks)
│
├── build/                        # 编译产物 (.o)
├── checkout/                     # 代码审查结果 (Agent 2 子 Agent)
├── release/                      # 发布产物 (.exe)
└── .specify/                     # Spec Kit 配置
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

## 🆕 纯布尔 Byte-Stream 对话机器人

**全链路纯布尔运算**的轻量级对话系统。两个版本：

| 版本 | Q&A | 架构 | 准确率 | 文件 |
|------|-----|------|--------|------|
| v1 | 16 | 深度4树 + XOR byte流 | 100% | [`boolchat.c`](release/boolchat.c) |
| **v2** | **64** | 深度6树 + N-gram hash | **100%** | [`boolchat_v2.c`](release/boolchat_v2.c) |
| LLM | 16 tokens | 深度4树 + one-hot分类 | 100% | [`llm_classify.c`](release/llm_classify.c) |

```
输入 "hello" → 路由树(CMP) → 叶子 XOR → byte流/argmax → "Hello! How can I help you?"
             全链路纯布尔: XOR + CMP only, 零浮点, 零矩阵乘, <1ms
```

### 运行

```bash
cd release
./boolchat_v2.exe        # 64 Q&A byte 流对话
./llm_classify.exe       # 16 token 分类对话
# 或双击 start_chat.bat
```

| 命令 | 功能 |
|------|------|
| `hello` / `who are you` / ... | 对话 |
| `list` | 查看所有问题 |
| `quit` | 退出 |

### 核心发现

- **XOR 闭式解**: `router_bits = input XOR target` 是当前最优方案
- **级联路由树**: 多分类正确架构 (1 leaf / pattern)
- **Tsetlin 限制**: XOR+Manhattan 距离有局部最小值; 单比特翻转无法逃逸
- **Coord Descent 限制**: 同样卡局部最小值; 暴力搜索证实数学上不可能

## Release 程序列表

| 程序 | 功能 | Q&A | 架构 |
|------|------|-----|------|
| `boolchat.exe` | Byte流对话 | 16 | 树+XOR |
| `boolchat_v2.exe` | Byte流对话(大) | 64 | N-gram+树+XOR |
| `llm_classify.exe` | 分类对话 | 16 tokens | one-hot+树+XOR |
| `simple_llm.exe` | Agent2实现 | 5对 | Router+Memory |
| `start_chat.bat` | 一键启动 | — | — |
| `start_test.bat` | 一键测试 | — | — |

---

## 已知问题

| 问题 | 影响 | 状态 |
|------|------|------|
| `boolnet_forward` 缓冲区太小 | Simple LLM 训练 crash | ⚠️ Agent 2 待修 |
| `tsetlin_train_step` 需真实实例 | 单元测试无法覆盖训练路径 | 📝 集成测试覆盖 |
| 信号放大需 Memory 触发机制 | 直接 Router 输出无法达到 byte-scale | ✅ 设计已解决 |

## Release 程序

预编译的独立可执行文件，无需编译即可运行：

| 程序 | 大小 | 功能 | 准确率 |
|------|------|------|--------|
| **`boolchat_v2.exe`** | 62K | 🏆 64 Q&A 纯布尔对话，127 节点路由树，N-gram hash + XOR 输出 | **100%** |
| `boolchat.exe` | 62K | 基础布尔对话，16 Q&A，路由树 | 良好 |
| `boolchat_train.exe` | 62K | 训练版，含完整 Tsetlin 训练流程 | — |
| `chatbot.exe` | 85K | 轻量对话机器人，16 Q&A 级联路由树 | ~6% |
| `chatbot_cascade.exe` | 61K | 级联路由树，31 节点，每叶子 Router(512b)+Mem(64c) | 路由准确 |
| `chatbot_trained.exe` | 62K | 预训练版对话模型 | — |
| `chatbot_full.exe` | 85K | 完整训练流水线 (早停+检查点+导出) | — |
| `chatbot_byte.exe` | 62K | 字节流处理版 | — |
| `chatbot_compute.exe` | 60K | 计算模式 | — |
| `train_long.exe` | 61K | 长训练测试程序 | — |

### 运行

```bash
# 直接双击或在终端运行
release/boolchat_v2.exe    # 🏆 推荐：64 Q&A 问答机器人

# 交互示例
> hello
BoolBot: Hello! How can I help you today?
> who are you
BoolBot: I am BoolBot, a pure Boolean neural network assistant.
> what is boolnet
BoolBot: BoolNet is a pure Boolean neural network framework using Tsetlin automata.
> bye
BoolBot: Goodbye! Have a great day!
```

## 模型特性

### 路由树架构 (boolchat_v2)

```
输入文本 → N-gram Hash → 深度6路由树(127节点)
         → 63内部节点(逐bit决策) → 64叶子节点
         → 每叶子: XOR解码 → 输出回复字节
```

- **N-gram 哈希编码**：输入文本 → 固定长度字节哈希，抗输入变体
- **127 节点路由树**：63 个内部决策节点 + 64 个叶子回答节点
- **逐 bit 路由**：每个内部节点通过 1 bit 决策走左/右子树
- **XOR 输出解码**：叶子节点 Router XOR 将路由信号解码为回复字节
- **O(log N) 查找**：6 层深度即可覆盖 64 个 Q&A

### 训练方式

| 算法 | 原理 | 速度 | 适用场景 |
|------|------|------|----------|
| **Tsetlin** | 随机 bit 翻转 + 奖励驱动 | 慢 (5000+ 步) | 探索性搜索 |
| **CoordDescent** | 逐 bit 贪心扫描，每轮只保留改进 | 快 (2 扫) | 确定性收敛 |

```
CoordDescent 示例: 32 bit 路由器, 2 次扫描收敛 (vs Tsetlin 5000 步)
  Sweep 1: 逐个尝试 32 bit, 保留 3 个改进 → reward 255→1020
  Sweep 2: 再次扫描 32 bit, 无新改进 → 收敛 ✅
```

### 模型持久化

训练产出统一存放在 [`weights/`](weights/) 目录，TENB 二进制格式：

```
weights/
├── simple_llm_model.bin    # Simple LLM (256 可训 bit)
├── boolchat_model.bin      # BoolChat 路由树权重
├── best.bin                # 最佳 checkpoint
└── ckpt_*.bin              # 训练检查点
```

## 测试结果

全部 **84 项单元测试 + 集成测试**（Agent 3 验证）：

| UID | 模块 | 函数测试 | 通过率 | 说明 |
|-----|------|---------|--------|------|
| 1 | bool_router | 13 | 100% | XOR 路由 + flip_bit + forward_layer |
| 2 | mem_int | 12 | 100% | 触发恢复 + 衰减 + save/load/roundtrip |
| 3 | mem_part | 16 | 100% | 分区读写 + LRU + trigger 加权和 |
| 4 | conv1d_circular | 13 | 100% | 布尔核回环 + flip_kernel_bit |
| 5 | upsampling | 11 | 100% | 多路由并行 + cascade |
| 6 | boolnet | 9 | 100% | 网络拓扑 + forward + save/load |
| 7 | tsetlin_train | 10 | 100% | 训练框架 + 统计 + 导出 |

### Release 程序测试

| 程序 | 测试结果 |
|------|----------|
| `boolchat_v2.exe` | ✅ 64/64 Q&A 路由正确，127 节点树 |
| `chatbot_cascade.exe` | ✅ 16 叶子路由正确，响应字节略有偏差 |
| `chatbot.exe` | 🟡 1/16 准确 (2 层架构限制) |
| `simple_llm` | 🟡 4/5 对话正确 (Tsetlin 收敛到局部最优) |
| `coord_descent_demo` | ✅ 2 扫收敛，比 Tsetlin 快 2500× |

报告：[`docs/test/`](docs/test/)

## 许可证

MIT License
