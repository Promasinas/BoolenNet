# Research: Memory 层模块

## 1. 多精度支持 → 预处理器宏

使用 `#define` 宏为每种精度生成独立函数（u8_forward, u16_forward 等），编译期展开为零开销。

## 2. 序列化格式 → 紧凑二进制

`[uid:4B][precision:1B][length:4B][max_value:8B][decay:8B][cells:N*size]`
Little-endian，总大小 25 + N × sizeof(type) 字节。

## 3. 防溢出自减 → 条件分支

```c
if (decay >= cell_value) cell_value = 0;
else cell_value -= decay;
```

## 4. UID 分配 → 调用方传入

Memory 层只接收 UID，不负责生成。由框架统一分配。

## 5. 接口设计

```c
MemLayer* memlayer_create(uid, precision, length, max_value, decay);
void      memlayer_destroy(MemLayer*);
void      memlayer_forward(MemLayer*, const uint8_t* router_signal);
void      memlayer_query(const MemLayer*, uint8_t* trigger_mask);
int       memlayer_save(const MemLayer*, const char* filepath);
MemLayer* memlayer_load(const char* filepath);
```
