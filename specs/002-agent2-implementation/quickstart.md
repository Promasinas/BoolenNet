# Quickstart: Agent 2 — Memory Layer

**Feature**: Agent 2 — Memory & Activation Management
**Date**: 2026-06-06

## 前提条件

- Windows + MinGW/MSYS2（GCC, Make）
- BoolNet 项目已初始化

## 编译

```bash
cd src/
make
# → memory_layer.o
```

## 示例程序

```c
#include "memory_layer.h"
#include <stdio.h>

int main() {
    // 创建: uid=2, uint16精度, 100 cells, max=1000, decay=5
    MemLayer *mem = memlayer_create(2, ML_PRECISION_UINT16, 100, 1000, 5);

    // Forward: 触发 cell 0 和 cell 42
    uint8_t signal[100] = {0};
    signal[0] = 1;  signal[42] = 1;
    memlayer_forward(mem, signal);

    // 查询触发状态
    uint8_t mask[100];
    memlayer_query(mem, mask);
    printf("cell[0]=%d cell[42]=%d\n", mask[0], mask[42]); // 1 1

    // 保存到文件
    memlayer_save(mem, "memory.bin");

    // 从文件加载
    MemLayer *loaded = memlayer_load("memory.bin");

    // 往返验证
    int ok = memlayer_verify_roundtrip(mem);
    printf("roundtrip: %s\n", ok == 0 ? "PASS" : "FAIL");

    memlayer_destroy(mem);
    memlayer_destroy(loaded);
    return 0;
}
```

## 编译运行示例

```bash
gcc -Wall -std=c99 -o example.exe example.c src/memory_layer.c
./example.exe
```

## 参数指南

| 参数 | 建议 | 说明 |
|------|------|------|
| precision | ML_PRECISION_UINT16 | 平衡容量和内存 |
| length | 256–65536 | 记忆单元数 |
| max_value | 1000 | 触发恢复上限 |
| decay | 1–10 | 每步自减值 |

## 持久化格式

```
[uid:4B LE][precision:1B][length:4B LE][max_value:8B LE][decay:8B LE][cells:N×cellsize]
                                             25 bytes header
```
