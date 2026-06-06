# Research: 布尔路由器

## 1. XOR 实现 → 按字节/按字批量 XOR

使用 `uint64_t` 按 64 位块 XOR（8 字节一批），剩余 < 8 字节逐字节处理。
单次路由 = O(N/64) 次 XOR 指令，编译器可自动向量化至 SSE/AVX。

## 2. 位打包格式 → 8 位/字节，LSB 优先

与 Memory 层 forward signal 格式一致（uint8_t 数组，每字节 1 位）。
内部运算可展开为 64 位字以提升吞吐。

## 3. 批量多路由 → 串行循环

M 个路由器对同一输入依次处理。因为 uint8_t 路由运算极快，
无需并行线程。O(M × N/64)。

## 4. 持久化格式

`[uid:4B][length:4B][bits: ceil(N/8) bytes]` — 与 Memory 层格式一致。
