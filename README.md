# BoolNet — Pure Boolean Neural Network Framework

[![C99](https://img.shields.io/badge/C-C99-blue)](src/)
[![Build](https://img.shields.io/badge/build-make-green)](src/Makefile)
[![Tests](https://img.shields.io/badge/tests-84%2F84-brightgreen)](docs/test/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20MinGW-lightgrey)]()

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
- **可解释**：每个路由器 bit 可直接阅读和调试

---

## 架构 (宪法 IV→X)

| # | 模块 | 宪法 | 描述 | 源码 |
|---|------|------|------|------|
| IV | Boolean Router | 路由器 | 逐位 XOR：`output[i] = input[i] ^ bits[i]` | [`src/bool_router/`](src/bool_router/) |
| V | 1D Circular Conv | 回环卷积 | 核溢出绕回起点，支持 stride/dilation | [`src/conv1d_circular/`](src/conv1d_circular/) |
| VI | Multi-Router Upsample | 多路由上采样 | M 个路由器并行 → M×N 拼接输出 | [`src/upsampling/`](src/upsampling/) |
| VIII | Integer Memory | Memory 层 | uint8/16/32/64，触发器恢复 + 衰减 | [`src/mem_int/`](src/mem_int/) |
| IX | Forward & Topology | Forward | 按 UID 排序层注册 + 串行传播 | [`src/boolnet/`](src/boolnet/) |
| X | Tsetlin Training | 自动机训练 | 逐 bit 翻转 + 奖励驱动学习 | [`src/tsetlin_train/`](src/tsetlin_train/) |
| — | Partition Manager | 扩展 | 浮点分区存储 + LRU + 加权和 | [`src/mem_part/`](src/mem_part/) |

---

## 网络示例

### 最小分类网络 (Router→Memory)

```c
BoolNet *net = boolnet_create(1, 4, 2);
BoolRouter *r = bool_router_create(1, 32, bits);
mem_int_add_layer(net, LAYER_ROUTER, 1, r, ...);
MemIntLayer *m = mem_int_create(2, ML_PRECISION_UINT8, 4, 255, 0);
mem_int_add_layer(net, LAYER_MEMORY, 2, m, ...);

TsetlinTrainer *t = tsetlin_create(net, 5000, 1020);
tsetlin_train_step(t, input_pattern, target_output);
```

### 完整 LLM 网络 (6 层)

```
Input(4B) → Router(32bit) → Conv1D(4,3) → Memory(4cell)
          → Upsample(M=2) → Router(64bit) → Memory(4cell) → Output(4B)
```

详见 [`test/llm_network_test.c`](test/llm_network_test.c)

---

## 快速开始

### 编译

```bash
cd src/
make all        # 编译全部 7 模块（~4000 行 C）
make train_tests  # 编译训练测试程序
make llm_test     # 编译 LLM 集成测试
```

### 运行

```bash
../test/train_test.exe        # 最小网络训练 (2 层, 10k 步)
../test/boolnet_train.exe     # 分类网络训练 (4 层, 20k 步)
../test/llm_network_test.exe  # LLM 全栈推理 (6 层, 30k 步)
```

### 要求

- **GCC** (MinGW/MSYS2 on Windows)
- **GNU Make**
- C99+ 标准库

---

## 测试结果

全部 **84 项测试通过**（Agent 3 自动验证）：

| UID | 模块 | 测试数 | 通过率 |
|-----|------|--------|--------|
| 1 | mem_int (Memory Layer) | 22 | 100% |
| 2 | mem_part (Partition Mgr) | 13 | 100% |
| 3 | conv1d_circular | 14 | 100% |
| 4 | upsampling | 12 | 100% |
| 5 | boolnet (Forward) | 13 | 100% |
| 6 | tsetlin_train | 10 | 100% |

报告：[`docs/test/`](docs/test/)

---

## 项目结构

```
BoolNet/
├── README.md
├── CLAUDE.md                     # Agent 上下文配置
│
├── src/                          # 源码 (~4000 行 C)
│   ├── Makefile                  # make all / train_tests / llm_test / clean
│   ├── common/                   # 共享类型 (LayerUID, 错误码)
│   ├── bool_router/              # IV: 布尔路由器
│   ├── conv1d_circular/          # V:  一维回环卷积
│   ├── upsampling/               # VI: 多路由上采样
│   ├── mem_int/                  # VIII: 整数 Memory 层
│   ├── mem_part/                 # 浮点分区管理器
│   ├── boolnet/                  # IX: Forward + 拓扑
│   └── tsetlin_train/            # X: Tsetlin 自动机训练
│
├── test/                         # 测试程序
│   ├── train_test.c              # 最小训练网络 (2 层)
│   ├── boolnet_train.c           # 分类训练网络 (4 层)
│   ├── llm_network_test.c        # LLM 全栈推理 (6 层)
│   └── uid_*/                    # Agent 3 单元测试
│
├── docs/                         # 文档
│   ├── api/                      # API 参考
│   ├── interact/                 # Agent 间通信
│   ├── test/                     # 测试报告 (84/84 通过)
│   └── history/                  # 设计决策历史
│
└── specs/                        # 规格文档 (8 features)
    ├── 001-agent1-design/        # Agent 1 主控设计
    ├── 002-agent2-implementation/ # Agent 2 代码生成
    ├── 003-agent3-testing/       # Agent 3 自动化测试
    ├── 004-bool-router/          # 布尔路由器
    ├── 005-circular-conv/        # 回环卷积
    ├── 006-upsampling/           # 多路由上采样
    ├── 007-forward/              # Forward 函数
    └── 008-tsetlin-train/        # Tsetlin 训练
```

---

## 三 Agent 协作

BoolNet 采用**三 Agent 分工架构**实现自动化开发流水线：

| Agent | 角色 | 职责 | 输出目录 |
|-------|------|------|----------|
| **Agent 1** | 主控/设计 | 用户沟通、设计文档、架构决策 | `docs/interact/` |
| **Agent 2** | 代码生成 | 读取设计 → 生成 C 代码 → 审查 → 修复 | `src/`, `docs/api/` |
| **Agent 3** | 自动化测试 | 检测变更 → 编译 → 运行 → 报告 → Git | `test/`, `docs/test/` |

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
