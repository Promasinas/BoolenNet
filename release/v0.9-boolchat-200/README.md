# BoolChat v0.9 — 200 Q&A Byte-Stream Chatbot

## 特点

- **200 对话对**，12 类别 (greetings/identity/tech/chat/farewells/emotions/questions/philosophy/casual/Chinese/travel/random)
- **坐标下降训练**，首次 0.3 秒达到 100% 准确率
- **纯 XOR 推理**，匹配 < 1ms
- **零依赖**，纯 C99 + 标准库
- **权重持久化**，首次训练后保存，二次启动秒开

## 运行

```bash
boolchat200.exe
```

```
You: hello
BoolBot: Hello! Nice to meet you! How can I help you today?

You: nihao
BoolBot: Nihao! Hen gaoxing jian dao ni...

You: tell me a joke
BoolBot: Why did the Boolean cross the road? To XOR the other side!
```

## 技术参数

| 参数 | 值 |
|------|-----|
| 编码 | 256 bytes/message (ASCII + length suffix) |
| 对话对 | 200 (12 类别) |
| 训练算法 | 坐标下降 (deterministic) |
| 训练时间 | ~300ms (首次) |
| 推理时间 | < 1ms |
| 权重文件 | boolchat_weights.bin (51KB) |
| 编译器 | GCC C99+ |

## 重新编译

```bash
gcc -std=c99 -O3 -o boolchat200.exe boolchat200.c -lm
```
