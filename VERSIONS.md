# BoolNet Release 版本索引

## 版本总览

| 版本 | 目录 | 功能 | Q&A | 准确率 | 架构 | 推荐 |
|------|------|------|-----|--------|------|------|
| v0.1 | `v0.1-chatbot` | 初版对话 | 16 | 混合 | Hamming+网络 | |
| v0.2 | `v0.2-cascade` | 级联树训练 | 16 | 实验 | 树+Tsetlin | |
| v0.3 | `v0.3-variants` | 多变体实验 | 16 | 实验 | Byte流+训练 | |
| v0.4 | `v0.4-boolchat` | **Byte流对话** | 16 | **100%** | 树+XOR | ⭐ |
| v0.5 | `v0.5-boolchat-v2` | **Byte流对话(大)** | 64 | **100%** | N-gram+树+XOR | ⭐ |
| v0.6 | `v0.6-simple-llm` | Simple LLM | 5对 | 1/5 | Router+Memory | |
| v0.7 | `v0.7-llm-classify` | **分类对话** | 16 tokens | **100%** | one-hot+树+XOR | ⭐ |
| v0.8 | `v0.8-boolchat-v3v4` | 纠错编码实验 | 16 | 0/16 | 暴力搜索 | 研究用 |

## 推荐使用

```bash
# Byte流 64 Q&A (最大规模)
release/v0.5-boolchat-v2/boolchat_v2.exe

# Byte流 16 Q&A (最稳定)
release/v0.4-boolchat/boolchat.exe

# 分类对话 16 tokens
release/v0.7-llm-classify/llm_classify.exe
```

## 关键里程碑

| 版本 | 发现 |
|------|------|
| v0.4 | XOR闭式解 `rb = input XOR target` 是byte映射最优方案 |
| v0.5 | N-gram编码+深度6树 → 64 Q&A 100% |
| v0.7 | one-hot分类避免XOR噪声, 16/16 |
| v0.8 | **证明**: XOR+Manhattan有局部最小值, 训练无法收敛 |

## 训练方法演进

```
Tsetlin (v0.2) → Coord Descent (v0.3) → 暴力搜索 (v0.8)
     ↓                  ↓                    ↓
  0.5%接受率        局部最小值          数学上不可能
     ↓                  ↓                    ↓
           XOR闭式解 = 最优方案 (v0.4+)
```

## 编译

所有版本均为独立 C 文件，无外部依赖：
```bash
gcc -std=c99 -O2 <source>.c -o <output>.exe
```
