# v0.7 — LLM Classify (级联路由分类) 🏆

**日期**: 2026-06-07 | **大小**: 60KB | **架构**: 9 节点路由树 + 级联 XOR → argmax

## 功能

🏆 **最佳性能模型**。级联路由树将 16 类 one-hot 输入分类到正确输出 token。

## 运行

```bash
./llm_classify.exe
```

## 架构

```
one-hot 输入 → 树路由 (逐 bit 决策) → 叶子 XOR → argmax → 输出 token
```

## 测试结果

| 指标 | 数据 |
|------|------|
| 路由树 | 9 节点, 7 叶子 |
| 分类类别 | 16 类 |
| 准确率 | **16/16 (100%)** ✅ |
| 推理速度 | <1ms |

## 完整词表分类

```
hello            → Hi there!        ✅
hi               → Hi there!        ✅
hey              → Hey you!         ✅
yo               → Yo!              ✅
how are you      → Im doing well    ✅
good morning     → Good morning!    ✅
good evening     → Good evening!    ✅
nice to meet     → Nice to meet you ✅
whats up         → Not much!        ✅
tell me more     → Sure thing       ✅
help             → How can I help?  ✅
who are you      → Im BoolBot       ✅
bye              → Goodbye!         ✅
thanks           → Youre welcome!   ✅
ok               → Got it           ✅
see you          → Take care!       ✅
```
