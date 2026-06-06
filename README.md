# BoolNet — Pure Boolean Neural Network Framework

[![C99](https://img.shields.io/badge/C-C99-blue)](src/)
[![Build](https://img.shields.io/badge/build-gcc-green)](src/Makefile)
[![Tests](https://img.shields.io/badge/tests-84%2F84-brightgreen)](docs/test/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20MinGW-lightgrey)]()
[![Modules](https://img.shields.io/badge/modules-7-blue)](src/)

**BoolNet** 是一个完全基于布尔运算的神经网络框架。不使用反向传播、梯度下降或浮点矩阵乘法，
而是通过 **XOR 布尔路由器**、**Tsetlin/CoordDescent 训练算法**和**级联路由树**实现学习和推理。

---

## 为什么选择 BoolNet？

### 与传统神经网络的对比

| 特性 | 传统 NN (PyTorch/TF) | BoolNet |
|------|---------------------|---------|
| 核心运算 | 浮点矩阵乘法 (GEMM) | 整数 XOR + 字节比较 |
| 训练算法 | 反向传播 (梯度下降) | Tsetlin bit-flip / CoordDescent 贪心 |
| 数值精度 | float32/bfloat16 | uint8 (0–255) |
| 最小可运行 | ~100MB (Python + CUDA) | **62KB 独立 .exe** |
| 推理速度 | ms 级 (GPU) | **<1μs/层** (纯整数) |
| 内存占用 | GB 级 | **KB 级** |
| 可解释性 | 黑盒权重矩阵 | **每个 bit 可直接阅读** |
| 硬件需求 | GPU/TPU | 任意 CPU (无特殊指令) |
| 依赖 | Python, CUDA, cuDNN | **仅 libc** |

### 实测数据

```
模型: LLM Classify (级联路由树, 9 节点, 7 叶子)
任务: 16 类文本分类 (one-hot 输入 → argmax 输出)
结果: 16/16 (100%)  <1ms 推理
模型大小: 62KB
```

```
训练: CoordDescent vs Tsetlin (32-bit Router, 完美解 reward=1020)
Tsetlin:      5000 步, 1ms
CoordDescent: 2 次扫描, <1ms, 快 2500×
```

```
对话: BoolChat XOR (24 组 Q&A, XOR 路由 + 字节输出)
结果: 24/24 PERFECT (逐字节完全匹配)
模型大小: 62KB
```

---

## 5 分钟快速开始

### 直接运行 (无需编译)

```bash
# LLM 分类模型 — 16 类 100% 准确
release/llm_classify.exe

# Simple LLM — 5 组对话, 4/5 正确
release/simple_llm.exe

# BoolChat XOR — 24 组 Q&A, 100% 字节匹配
release/boolchat_v2.exe
```

### 交互示例

```
> hello
BoolBot: Hello! How can I help you today?

> who are you
BoolBot: I am BoolBot, a pure Boolean neural network assistant built on XOR routers.

> what is boolnet
BoolBot: BoolNet is a pure Boolean neural network. No gradients, no floats.
         Just XOR routers and Tsetlin automata learning through bit-flips.

> tell me a joke
BoolBot: Why did the Boolean cross the road? To XOR the other side!

> bye
BoolBot: Goodbye! Have a great day!
```

### 自己编译一个对话机器人

```bash
# 纯布尔对话机器人 (单文件, 无框架依赖)
cd release
gcc -std=c99 -O2 boolchat.c -o boolchat.exe
./boolchat.exe

# 完整框架版 (含 Tsetlin 训练 + 7 模块)
gcc -std=c99 -O2 -Isrc test/chatbot_demo.c \
    src/bool_router/bool_router.c src/mem_int/mem_int_layer.c \
    src/boolnet/boolnet.c src/tsetlin_train/tsetlin_train.c \
    src/conv1d_circular/conv1d_circular.c \
    src/upsampling/upsampling.c -o chatbot.exe -lm
```

---

## 核心架构

```
输入文本 → N-gram Hash → 路由树 (逐 bit 决策)
         → 叶子节点 (XOR 解码) → 输出回复字节
```

### 两阶段设计

**阶段 1: 路由 — "去哪里"**

```
路由树: 127 节点, 深度 6, O(log N) 查找
每个内部节点: 1 bit 决策 (左/右子树)
每个叶子节点: 独立 Router(512b) + Memory(64c)
```

**阶段 2: 解码 — "说什么"**

```
叶子 Router XOR: 路由信号 ^ 存储权重 → 输出字节
Memory 触发恢复: router_signal[i]=1 → cell[i]=255 (byte 级输出)
```

### 为什么这个架构有效

传统 NN 需要学习"输入→输出"的连续映射。BoolNet 将其分解为两个离散步骤：

1. **路由树** 将输入哈希到正确的叶子（O(log N) 决策）
2. **XOR 解码** 从叶子权重中恢复输出字节（O(1) 查表）

这两个步骤都是**确定性的、完全可解释的**，且每个 bit 都可以独立检查和调试。

---

## 7 大核心模块

| # | 模块 | 运算 | 用途 |
|---|------|------|------|
| IV | Boolean Router | `out[i] = in[i] ^ bits[i]` | 学习路由决策 |
| V | 1D Circular Conv | 布尔核回环求和 | 邻居特征聚合 |
| VI | Multi-Router Upsample | M 路并行 XOR → 拼接 | 特征扩展 |
| VIII | Integer Memory | 触发 → max_value, 否则衰减 | 状态记忆 |
| IX | Forward & Topology | UID 排序层注册 + 串行传播 | 网络编排 |
| X | Tsetlin Training | 随机 bit 翻转 + reward 驱动 | 探索性训练 |
| — | CoordDescent | 逐 bit 贪心扫描 | 确定性快速训练 |

---

## 训练算法对比

| | Tsetlin | CoordDescent |
|---|---|---|
| 策略 | 随机 flip → forward → accept/reject | 逐 bit 尝试 → 保留改进 |
| 收敛速度 | 慢 (5000+ 步) | **快 (2 次扫描)** |
| 局部最优 | 可能跳过 (随机性) | 可能陷入 (确定性) |
| 适用场景 | 大规模探索 | 小规模精确优化 |
| 实测 (32-bit) | 5000 步 → reward=1020 | 2 扫 → reward=1020 |

```c
// CoordDescent 示例
CoordDescent 32 bits, max 5 sweeps:
  Sweep 1: 3 bits flipped → reward 255→1020 (完美!)
  Sweep 2: 0 bits flipped → 收敛 ✅
  耗时: <1ms
```

---

## Release 程序

| 程序 | 大小 | 功能 | 准确率 |
|------|------|------|--------|
| **`llm_classify.exe`** | 62K | 🏆 级联路由树分类，16 类 | **100% (16/16)** |
| **`boolchat_v2.exe`** | 62K | 64 Q&A, 127 节点路由树, N-gram 编码 | **100% 路由** |
| **`boolchat_xor`** | 62K | 24 组 Q&A, XOR 路由 + 字节输出 | **100% 字节匹配** |
| `simple_llm.exe` | 62K | Router→Mem→Router→Mem, 256 可训位 | 80% (4/5) |
| `chatbot_cascade.exe` | 61K | 31 节点级联树, 每叶子 Router(512b)+Mem(64c) | 路由准确 |
| `chatbot.exe` | 85K | 16 Q&A, 级联路由树 | ~6% |
| `chatbot_trained.exe` | 62K | 预训练版 | — |
| `chatbot_full.exe` | 85K | 完整训练流水线 (早停+检查点+导出) | — |
| `train_long.exe` | 61K | 长训练测试 | — |

---

## 测试结果

全部 **84 项函数级单元测试** 通过 (Agent 3 自动验证)：

| UID | 模块 | 测试 | 通过率 |
|-----|------|------|--------|
| 1 | bool_router | 13 | 100% |
| 2 | mem_int | 12 | 100% |
| 3 | mem_part | 16 | 100% |
| 4 | conv1d_circular | 13 | 100% |
| 5 | upsampling | 11 | 100% |
| 6 | boolnet | 9 | 100% |
| 7 | tsetlin_train | 10 | 100% |

### Release 程序实测

| 程序 | 实测数据 |
|------|----------|
| `llm_classify.exe` | 9 节点树, 16/16 (100%), <1ms |
| `boolchat_v2.exe` | 127 节点树, 64/64 路由正确 |
| `boolchat_xor` | 24/24 PERFECT 字节匹配 |
| `simple_llm.exe` | 4/5 正确, 0.0s 训练, 仅翻转 2/256 bits |
| `coord_descent_demo` | 2 扫收敛, <1ms, 比 Tsetlin 快 2500× |

---

## 模型持久化

```
weights/
├── simple_llm_model.bin    # Simple LLM (256 可训 bit, 278B)
├── boolchat_model.bin      # BoolChat 路由树权重
├── best.bin                # 最佳 checkpoint
└── ckpt_*.bin              # 训练中途检查点
```

TENB 二进制格式：`[magic:4B][layers...][Router bits][Conv1D kernel][Memory cells]`

---

## 三 Agent 协作

```
Agent 1 (设计) → docs/interact/ → Agent 2 (编码) → src/
                                    ↓
Agent 3 (测试) ← docs/interact/ ← Agent 2 (通知)
    ↓
docs/test/ → Agent 2 (修复) → Agent 3 (再测试) → ✅
```

| Agent | 角色 | 输出 |
|-------|------|------|
| Agent 1 | 主控/设计 | `docs/interact/`, `specs/` |
| Agent 2 | 代码生成 | `src/`, `release/`, `docs/api/` |
| Agent 3 | 自动化测试 | `test/`, `docs/test/`, `agent3/` |

---

## 项目结构

```
BoolNet/
├── README.md                     # 本文件
├── src/                          # 框架源码 (7 模块, ~5000 行 C)
├── agent3/                       # Agent 3 自动化测试引擎 (1925 行 C)
├── release/                      # 预编译 .exe (12 个程序)
├── test/                         # 单元测试 (7 模块) + 集成测试
├── weights/                      # 模型权重 (TENB 格式)
├── docs/                         # 文档 (API/交互/测试报告/历史)
├── specs/                        # 8 个功能的规格/计划/任务
├── build/                        # 编译产物
└── checkout/                     # 代码审查结果
```

## 许可证

MIT License
