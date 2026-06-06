# BoolNet Release

## 可执行文件

| 文件 | 大小 | 功能 | 推荐 |
|------|------|------|------|
| `boolchat_v2.exe` | ~62KB | **64 Q&A** byte流对话 (推荐) | ⭐ |
| `boolchat.exe` | ~62KB | 16 Q&A byte流对话 | ⭐ |
| `llm_classify.exe` | ~62KB | 16 token 分类对话 | ⭐ |
| `simple_llm.exe` | ~86KB | Agent2 实现 (训练版) | |
| `chatbot.exe` | ~86KB | BoolNet库完整版 | |
| `chatbot_cascade.exe` | ~62KB | 级联树训练版 | |

## 运行

### 双击启动
```
start_chat.bat    → 对话机器人
start_test.bat    → 编译并运行测试
```

### 命令行
```bash
./boolchat_v2.exe        # 64 Q&A
```

### 编译源码
```bash
gcc -std=c99 -O2 boolchat_v2.c -o boolchat_v2.exe
```

## 版本演进

| 版本 | 文件 | 方法 | 结果 |
|------|------|------|------|
| v1 | boolchat.c | 树+XOR闭式解 | 16/16 ✅ |
| v2 | boolchat_v2.c | N-gram+深度6树 | 64/64 ✅ |
| v3 | boolchat_v3.c | Coord Descent多变体 | 0/16 ❌ 局部最小值 |
| v4 | boolchat_v4.c | 暴力搜索256^64 | 0/16 ❌ 数学不可能 |
| LLM | llm_classify.c | one-hot+树+分类 | 16/16 ✅ |

## 核心结论

1. **XOR闭式解** (`rb = input XOR target`) 是byte映射的最优方案
2. **级联路由树** 是多分类的正确架构
3. **Tsetlin/Coord Descent** 对XOR+Manhattan距离有局部最小值
4. **模糊匹配** 需扩展树 (每种变体一个叶子)
