# v0.6 — Simple LLM (Router→Memory 对话模型)

**日期**: 2026-06-07 | **大小**: 62KB | **架构**: Router(128b)→Memory(128c)→Router(128b)→Memory(16c)

## 功能

BoolNet 首个语言模型架构。Router 学习路由，Memory 触发恢复完成信号放大。

## 运行

```bash
./simple_llm.exe
```

## 架构

```
Input(16B one-hot) → Router(128b) → Memory(128c,decay=1)
                   → Router(128b) → Memory(16c,decay=0) → Output(argmax)
                     256 可训位
```

## 测试结果

| 指标 | 数据 |
|------|------|
| 训练时间 | 0.0 秒 |
| 接受翻转 | ~100 次 Tsetlin |
| 准确率 | 4/5 (80%) |
| Router1 flipped | 1–2/128 bits |
| Router2 flipped | 1–3/128 bits |
| 对话对 | "hello"→"Hi there!" 等 5 组 |

## 已知限制

- "hello"→"Hello!" 而非 "Hi there!" (1 个 case 未收敛)
- Tsetlin 随机性导致训练结果不稳定
