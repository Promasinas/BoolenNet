# Simple LLM Design — 固定对话模型

**Target Agent**: Agent 2（代码生成）
**From Agent**: Agent 1 / Agent 3（联合设计）
**Date**: 2026-06-06
**Status**: 设计完成，待 Agent 2 实现

---

## 1. 设计目标

使用 BoolNet 现有模块（Router + Memory）构建一个能完成 **3–5 段固定对话** 的简单语言模型。

**输入**: 用户消息编码为 one-hot token（如 "hello" = token[0]）
**输出**: 模型回复编码为 one-hot token（如 "hi there" = token[3]）
**训练**: Tsetlin 自动学习 Router 权重，将输入 token 映射到正确的输出 token

## 2. 核心设计思路

### 关键洞察：Memory 层天然解决信号放大

之前训练失败的根因是：**用 Router 输出 byte 值去匹配目标 byte 值**。

但 Memory 层的工作方式是：
```
router_signal[i] = 1  →  cell[i] = max_value (255)
router_signal[i] = 0  →  cell[i] = max(0, cell[i] - decay)
```

Memory 自身将 **1-bit 触发信号** 放大为 **255-byte 输出值**。因此正确的架构是：

```
输入(1-hot byte) → Router(学习路由) → Memory(触发放大+输出)
```

**不需要在 Router 层面放大信号**——Memory 的触发机制天然完成这件事。

## 3. 网络架构

```
┌─────────────────────────────────────────────────────────┐
│                    SimpleDialogue                       │
│                                                         │
│  Input(16B one-hot)                                      │
│       │                                                  │
│       ▼                                                  │
│  ┌──────────────┐   Router 1: 128 bits                  │
│  │  Router 1    │   学习: 输入token→上下文触发位置        │
│  │  (128-bit)   │                                        │
│  └──────┬───────┘                                        │
│         │ router_signal[128]                             │
│         ▼                                                │
│  ┌──────────────┐   Memory 1: 128 cells, uint8           │
│  │  Memory 1    │   max=255, decay=1 (有衰减)            │
│  │  (128-cell)  │   作用: 对话上下文/状态记忆             │
│  └──────┬───────┘                                        │
│         │ cell_values[128] (query→trigger_mask)          │
│         ▼                                                │
│  ┌──────────────┐   Router 2: 128→N bits                │
│  │  Router 2    │   学习: 上下文→输出token位置            │
│  │  (128→N-bit) │   N = 输出词表大小                     │
│  └──────┬───────┘                                        │
│         │ router_signal[N]                               │
│         ▼                                                │
│  ┌──────────────┐   Memory 2: N cells, uint8             │
│  │  Memory 2    │   max=255, decay=0 (输出层无衰减)      │
│  │  (N-cell)    │   输出: cell_values → 取argmax即为token │
│  └──────┬───────┘                                        │
│         │                                                │
│         ▼                                                │
│  Output: {"hi there"} → token[3]                        │
└─────────────────────────────────────────────────────────┘
```

### 参数配置

| 参数 | 值 | 说明 |
|------|-----|------|
| 输入维度 | 16 | 16 个可能的用户输入 token |
| 输出维度 | 16 | 16 个可能的模型回复 token |
| Router 1 位数 | 128 | 比输入大，提供充足路由空间 |
| Memory 1 单元数 | 128 | 对话上下文记忆 |
| Memory 1 max/decay | 255/1 | 有衰减→旧上下文逐渐遗忘 |
| Memory 2 单元数 | 16 | 输出层，每个 cell 对应一个回复 token |
| Memory 2 max/decay | 255/0 | 无衰减→输出保持稳定 |
| 可训参数 | 128+128=256 bits | 两个 Router 的权重 |

## 4. 对话数据

### 词表编码

```
输入 token (16):
  0: "hello"        4: "how are you"   8: "what's up"    12: "bye"
  1: "hi"           5: "good morning"  9: "tell me more" 13: "thanks"
  2: "hey"          6: "good evening"  10: "help"        14: "ok"
  3: "yo"           7: "nice to meet"  11: "who are you" 15: "see you"

输出 token (16):
  0: "Hello!"       4: "I'm doing well"  8: "Not much!"     12: "Goodbye!"
  1: "Hi there!"    5: "Good morning!"   9: "Sure thing"    13: "You're welcome!"
  2: "Hey you!"     6: "Good evening!"   10: "How can I help?" 14: "Got it"
  3: "Yo!"          7: "Nice to meet you" 11: "I'm BoolBot" 15: "Take care!"
```

### 训练对（5 段固定对话）

| # | 输入 (one-hot) | 期望输出 (one-hot) |
|---|---------------|-------------------|
| 1 | token[0]="hello" | token[1]="Hi there!" |
| 2 | token[4]="how are you" | token[4]="I'm doing well" |
| 3 | token[11]="who are you" | token[11]="I'm BoolBot" |
| 4 | token[12]="bye" | token[12]="Goodbye!" |
| 5 | token[13]="thanks" | token[13]="You're welcome!" |

## 5. 数据流示例

### 对话 1: "hello" → "Hi there!"

```
Step 1: 输入编码
  input[16] = {255, 0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0}  // token[0]=hello (byte=255)

Step 2: Router 1 前向
  router1_out[128] ← input XOR router1_bits
  训练后: router1_out[42] ≈ 1  // 学会激活位置42

Step 3: Memory 1 前向
  cell[42] 触发 → 恢复为 255
  其他 cell 衰减（旧上下文遗忘）
  query → trigger_mask[128]

Step 4: Router 2 前向
  router2_out[16] ← trigger_mask XOR router2_bits
  训练后: router2_out[1] = 1  // 学会激活输出位置1

Step 5: Memory 2 前向（输出层）
  cell[1] 触发 → 恢复为 255
  mem_int_forward_output → cell_values[16]
  output = {0, 255, 0,0,0,0,0,0, 0,0,0,0,0,0,0,0}

Step 6: 解码输出
  argmax(output) = 1 → token[1] → "Hi there!" ✅
```

## 6. 训练参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 训练步数 | 20,000 | 每步随机选一个训练对 |
| byte_max | 16×255=4080 | reward 上限 |
| neg_tolerance | 5,000 | 连续 5000 次拒绝后停止 |
| 早停 patience | 500 | val 无改善 500 epoch 后停止 |

## 7. 为什么这个设计能工作

对比之前失败的 `full_train_save`：

| | 旧设计（失败） | 新设计（可行） |
|---|---|---|
| 输入编码 | one-hot(1) | one-hot(255) |
| 输出目标 | byte值(255) | Memory触发(255) |
| Router输出用途 | 直接作为最终输出 | 作为Memory触发信号 |
| 信号放大 | 依赖Router(XOR) | 依赖Memory(触发机制) |
| 可训参数 | 67 bits | 256 bits |

**核心区别**：新设计让 Router 只学习"路由"（触发哪个 Memory cell），Memory 负责"放大"（bit→byte）。这是 BoolNet 架构的正确使用方式——Router 决定"去哪"，Memory 决定"值多少"。

## 8. 实现文件清单

```
src/simple_llm/
├── simple_llm.c          # 主程序: 构建网络 + 训练 + 推理 + 保存
├── simple_llm.h          # 公共接口
└── Makefile 片段         # 添加到 src/Makefile

新增 Makefile target:
  simple_llm: bool_router mem_int boolnet tsetlin
      gcc -Isrc/common -Isrc/bool_router -Isrc/mem_int \
          -Isrc/boolnet -Isrc/tsetlin_train \
          -o simple_llm.exe src/simple_llm/simple_llm.c \
          src/bool_router/bool_router.o src/mem_int/mem_int_layer.o \
          src/boolnet/boolnet.o src/tsetlin_train/tsetlin_train.o
```

## 9. 验收标准

1. **训练收敛**: 5 个训练对中 ≥3 个达到 100% 匹配（argmax 正确）
2. **推理可用**: 输入 "hello" → 输出 "Hi there!"
3. **模型持久化**: 能保存/加载 Router 权重 + Memory 状态
4. **对话上下文**: Memory 1 的 decay 机制让连续对话有上下文记忆
5. **零崩溃**: 训练和推理过程无 segfault/内存泄漏

## 10. 后续扩展路径

- 增加词表大小（16→64→256→1024 tokens）
- 增加 Router 层数（2→4→8 层深度路由）
- 添加 Conv1D 做 token 序列特征提取
- Memory 1 的 decay 调参实现不同"记忆长度"
- 支持多轮对话（Memory 不 reset，连续 forward）
