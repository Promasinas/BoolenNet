# Agent Skills Distilled — BoolNet 核心技术提炼

## Agent 1 (设计) — 架构决策

### 核心技能: 级联路由树设计
```
输入 → [CMP×D] → 叶子 → Router XOR → 输出
```
- **决策树深度公式**: D = ceil(log₂(N)), N = 类别数
- **节点构建**: 贪心找最优分裂字节 (最大化左右平衡)
- **每节点**: 1 次 CMP (纯布尔, 1 cycle)

### 核心技能: Byte-Stream vs Classification 路线选择
| 场景 | 推荐路线 | 原因 |
|------|---------|------|
| 精确 Q&A | Byte-Stream + XOR 闭式解 | 100% 准确, 无训练 |
| 词表对话 | one-hot Classification | 无 XOR 噪声 |
| 模糊匹配 | 树扩展 (每变体一叶子) | XOR 无法容错 |

### 核心技能: N-gram 编码
```c
// unigram + bigram + trigram + 长度特征 → 鲁棒 hash
for(i=0;i<len;i++) v[pos] ^= s[i];                              // unigram
for(i=0;i<len-1;i++) v[pos] ^= (s[i]*31 + s[i+1]*17) & 0xFF;   // bigram  
for(i=0;i<len-2;i++) v[pos] ^= (s[i]+s[i+1]+s[i+2]) & 0xFF;    // trigram
v[N-2] ^= len; v[N-1] ^= (len*13);                              // length
```

## Agent 2 (实现) — 训练方法论

### 核心技能: XOR 闭式解 (最优方案)
```c
// 对固定映射, 最优路由器比特可一步计算:
for (int i = 0; i < N; i++)
    router_bits[i] = input[i] ^ target[i];
// 复杂度 O(N), 确定性全局最优
```

### 核心技能: Coord Descent (确定性训练)
```c
// 逐比特测试, 保留改进, 回退退化
for each bit in all_layers:
    before = reward(net, input, target)
    flip(bit)
    after = reward(net, input, target)
    if (after > before) keep() else revert()
// O(2N) per sweep, 1-3 sweeps 收敛
```
**局限**: XOR 域 Manhattan 距离存在局部最小值 → 单模式可用, 多模式崩塌

### 核心技能: Memory 触发放大机制
```
Router 输出 (1 bit) → Memory 触发 → cell = 255 (8 bit)
Router 只学路由(去哪), Memory 负责放大(值多少)
```

### 核心技能: Tsetlin 自动机
```c
// 随机单比特翻转 + 奖励驱动
reward = byte_max - sum(|output - target|)
if (flip_improves) accept else revert
// 适用: <100 参数, 单模式
// 限制: 多分类崩塌 (O(1/N) 命中率)
```

## Agent 3 (测试) — 验证模式

### 核心技能: 模块化测试结构
```
test/uid_NNN/
├── test_<module>.c    # 函数级测试
├── Makefile           # 独立编译
└── (output)           # 测试结果
```
- **每模块**: 10-22 个测试函数
- **覆盖**: create/destroy/forward/save/load/edge_cases
- **报告**: `docs/test/uid_NNN_report.md`

### 核心技能: 测试优先级
```
P1: 生命周期 (create/destroy)
P1: 核心运算 (forward)
P2: 持久化 (save/load)
P3: 性能 (benchmark 10000 单元 < 1ms)
```

## 跨 Agent 协作模式

```
Agent1(设计) → docs/interact/simple_llm_design.md → Agent2(实现)
                                                         ↓
Agent3(测试) ← docs/interact/test_request_mem_int.md ← Agent2(通知)
     ↓
docs/test/uid_NNN_report.md → Agent2(修复) → Agent3(再测试) → ✅
```

## 关键数学发现

### 定理: XOR + Manhattan 距离存在局部最小值
```
设 qv=0x68, av=0x48, 最优 rb=0x20
从 rb=0xE8: 全部 8 个单比特翻转都增加距离
→ Coord Descent / Tsetlin 无法逃逸
→ 需要同时翻转多比特 (指数搜索)
```

### 推论: 训练算法选择指南
| 场景 | 推荐算法 | 原因 |
|------|---------|------|
| 固定映射 | XOR 闭式解 | O(1), 全局最优 |
| 单模式学习 | Coord Descent | O(2N), 确定性 |
| 探索/研究 | Tsetlin | 简单, 可观察比特翻转 |
| 多分类 | 级联树 + XOR | 每类独立路由器 |

## 实战 Checklist

- [ ] 编码: N-gram hash (≥3-gram) 或 one-hot
- [ ] 树深度: D ≥ ceil(log₂(N))
- [ ] 叶子路由器: XOR 闭式解优先
- [ ] 如需训练: Coord Descent (单模式)
- [ ] 模糊匹配: 扩展树 (非容错编码)
- [ ] 输出格式: one-hot 分类 > byte 流 (无噪声)
- [ ] 验证: 100% 训练集准确率 + 交互测试
