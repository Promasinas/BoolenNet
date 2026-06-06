# v0.8 — BoolChat v3/v4 (CoordDescent 优化)

**日期**: 2026-06-07 | **大小**: 63–64KB | **架构**: 31 节点路由树 + 纠错编码

## 功能

使用 CoordDescent 替代 Tsetlin 进行训练，支持多变体纠错编码。

## 运行

```bash
./boolchat_v3.exe    # CoordDescent + 多变体训练
./boolchat_v4.exe    # 最新实验版
```

## v3 测试结果

| 指标 | 数据 |
|------|------|
| 路由树 | 31 节点 |
| 训练算法 | CoordDescent (确定性贪心) |
| 每叶子变体 | 5 个 |
| 精确匹配 | 0/16 |
| 问题 | CoordDescent 陷入局部最优 |

## v4 改进

- 增强的 CoordDescent 扫描策略
- 改进的 XOR 编码方案

## 研究结论

v3/v4 的纠错编码在数学上被证明不可能完全精确——参见 `5c6eecf`。路由树方案 (v0.5/v0.7) 是正确的方向。
