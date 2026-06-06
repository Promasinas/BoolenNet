# Research: Agent 2 — Memory Layer

**Feature**: Agent 2 — Memory & Activation Management
**Date**: 2026-06-06

## 1. Type-Erased Cell Storage

### Decision
使用 `void* cells` + precision 枚举实现多精度支持。通过内联 switch 分发读写操作。

### Rationale
- 宪章 VIII 要求 uint8/16/32/64 四种精度
- 类型擦除避免 4 份重复代码，单一 `memlayer_forward` 处理所有精度
- switch 在现代编译器下优化为跳转表，开销可忽略

### Alternatives Considered
- **4 个独立结构体**: 代码膨胀 4×
- **C11 `_Generic`**: 仍需手动展开每种类型
- **宏模板**: 调试困难

## 2. Binary Persistence Format

### Decision
Little-endian 二进制格式：`[uid:4B][precision:1B][length:4B][max_value:8B][decay:8B][cells:N×size]`

### Rationale
- x86/Windows 原生 LE，无需字节序转换
- 25 字节固定头 + 可变 cells 数据
- `fwrite`/`fread` 单次批量 I/O 性能最优
- 宪章 VII 要求数据持久化

### Alternatives Considered
- **JSON/文本格式**: 人类可读但膨胀严重（uint64 十进制 = 20 字节 vs 8 字节）
- **MessagePack/Protobuf**: 引入外部依赖

## 3. Forward Propagation Logic

### Decision
逐 cell 循环：`router_signal[i]==1 → cell=max_value`，否则 `cell -= min(decay, cell)`

### Rationale
- O(length) 线性复杂度，适合 CPU 顺序执行
- 防下溢：`cell >= decay ? cell - decay : 0`
- 每次 forward 独立，无跨 cell 依赖 → 未来可 SIMD 优化

## 4. Source Organization

### Decision
单个 `memory_layer.c` 包含所有函数，`boolnet_common.h` 提供共享类型。

### Rationale
- Memory 层 API 是内聚单元，拆分为多个 .c 增加链接复杂度
- 约 200 行代码量适合单文件维护
- Makefile 只需单个 `memory_layer.o` 目标
