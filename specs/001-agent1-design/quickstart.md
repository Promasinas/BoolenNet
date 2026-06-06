# Quickstart: Memory 层模块

## 编译
```bash
cd src && make
```

## 基本使用
```c
#include "memory_layer.h"

// 创建: UID=1, uint8, 长度10, 上限255, 自减1
MemLayer* mem = memlayer_create(1, ML_PRECISION_UINT8, 10, 255, 1);

// Forward: 路由信号触发位置 0,3,7
uint8_t sig[10] = {1,0,0,1,0,0,0,1,0,0};
memlayer_forward(mem, sig);

// 查询触发状态
uint8_t mask[10];
memlayer_query(mem, mask);

// 持久化
memlayer_save(mem, "mem.bin");
MemLayer* loaded = memlayer_load("mem.bin");

// 清理
memlayer_destroy(mem);
memlayer_destroy(loaded);
```

## 精度变体
```c
MemLayer* m8  = memlayer_create(1, ML_PRECISION_UINT8,  100, 255, 1);
MemLayer* m16 = memlayer_create(2, ML_PRECISION_UINT16, 100, 65535, 5);
MemLayer* m32 = memlayer_create(3, ML_PRECISION_UINT32, 1000, 100000, 10);
MemLayer* m64 = memlayer_create(4, ML_PRECISION_UINT64, 10000, UINT64_MAX, 50);
```
