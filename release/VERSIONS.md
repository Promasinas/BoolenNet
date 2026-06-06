# Release 版本索引

按开发时间排列，每个版本包含可执行文件 + 源码 + 文档。

| 版本 | 目录 | 架构 | 准确率 | 亮点 |
|------|------|------|--------|------|
| v0.1 | [`v0.1-chatbot/`](v0.1-chatbot/) | Router→Memory (2层) | 6% | 首个对话机器人 |
| v0.2 | [`v0.2-cascade/`](v0.2-cascade/) | 31节点级联树 | 路由正确 | 引入路由树架构 |
| v0.3 | [`v0.3-variants/`](v0.3-variants/) | 多架构变体 | — | 字节流/训练流水线/预训练 |
| v0.4 | [`v0.4-boolchat/`](v0.4-boolchat/) | 纯XOR路由 | 路由正确 | XOR替代Tsetlin |
| v0.5 | [`v0.5-boolchat-v2/`](v0.5-boolchat-v2/) | 127节点树+N-gram | **100%** | 🏆 64 Q&A路由 |
| v0.6 | [`v0.6-simple-llm/`](v0.6-simple-llm/) | Router→Mem→Router→Mem | 80% | 首个LLM架构 |
| v0.7 | [`v0.7-llm-classify/`](v0.7-llm-classify/) | 9节点级联分类 | **100%** | 🏆 16类完美分类 |
| v0.8 | [`v0.8-boolchat-v3v4/`](v0.8-boolchat-v3v4/) | CoordDescent+纠错 | 0%精确 | 研究：纠错数学不可行 |

## 推荐使用

- **最佳分类**: `v0.7-llm-classify/` — 16 类 100% 准确
- **最佳 Q&A**: `v0.5-boolchat-v2/` — 64 Q&A 100% 路由
- **研究参考**: `v0.6-simple-llm/` — LLM 架构原型
- **历史存档**: `v0.1–v0.4` — 早期探索版本
