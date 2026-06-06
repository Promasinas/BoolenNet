# BoolNet 研究结论

## 训练算法对比

| 算法 | 方法 | 收敛性 | 速度 | 适用场景 |
|------|------|--------|------|---------|
| Tsetlin | 随机单比特翻转 | ⚠️ O(1/N) 命中率 | ~50M步/秒 | 小参数 (<100 bits) |
| Coord Descent | 逐比特确定测试 | ❌ 局部最小值 | O(2N)/sweep | 单模式 |
| 暴力搜索 | 256^N 全空间 | ✅ 全局最优 | O(256N) | ≤8 bits |
| **XOR 闭式解** | rb = input XOR target | ✅ 确定性 | O(1) | **推荐** |

## XOR + Manhattan 距离的数学限制

```
定理: 对 XOR 域上的 Manhattan 距离函数, 单比特翻转邻域存在局部最小值。

证明: 设 qv=0x68, av=0x48, 最优 rb=0x20。
从 rb=0xE8 出发: 所有 8 个单比特翻转都增加距离。
→ Coord Descent 和 Tsetlin 都无法逃逸。
```

## 级联路由树

```
架构: 二叉树, 每节点 1 次 byte 比较, O(log N) 深度

深度 4 → 16 叶子 (31 节点)
深度 6 → 64 叶子 (127 节点)
深度 8 → 256 叶子 (511 节点)

每叶子: 1 个 Router (512 bit XOR)
参数: N_leaves × 512 bits

最优方案: 树结构确定性构建 + 叶子 XOR 闭式解
```

## Byte-Stream vs Classification

| | Byte-Stream | Classification |
|---|---|---|
| 输入编码 | N-gram hash | one-hot token |
| 输出 | byte 序列 | argmax 索引 |
| XOR 噪声 | ❌ 无法消除 | ✅ one-hot 无噪声 |
| 模糊匹配 | ❌ | ✅ 前缀匹配 |
| 推荐 | 精确 Q&A | 词表对话 |
