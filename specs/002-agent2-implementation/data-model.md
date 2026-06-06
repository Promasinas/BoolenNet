# Data Model: Agent 2 — Memory Layer

**Feature**: Agent 2 — Memory & Activation Management
**Date**: 2026-06-06

## Entity: MemLayer

| 字段 | 类型 | 说明 |
|------|------|------|
| `uid` | `LayerUID` (uint32_t) | 全局唯一层标识符 |
| `precision` | `uint8_t` | 精度枚举 (0=uint8,1=uint16,2=uint32,3=uint64) |
| `length` | `uint32_t` | 记忆单元数量 |
| `max_value` | `uint64_t` | 触发恢复上限，钳位到精度最大值 |
| `decay` | `uint64_t` | 每次 forward 自减值 |
| `cells` | `void*` | 动态分配的 cells 数组 (length × cellsize) |

## Entity: Binary File Format

| 偏移 | 大小 | 字段 | 说明 |
|------|------|------|------|
| 0 | 4 | uid | uint32 LE |
| 4 | 1 | precision | uint8 |
| 5 | 4 | length | uint32 LE |
| 9 | 8 | max_value | uint64 LE |
| 17 | 8 | decay | uint64 LE |
| 25 | N×S | cells | N=length, S=cell_size(precision) |

## Precision Types

| 枚举值 | 类型 | sizeof | 最大值 |
|--------|------|--------|--------|
| 0 | uint8_t | 1 | 255 |
| 1 | uint16_t | 2 | 65535 |
| 2 | uint32_t | 4 | 4294967295 |
| 3 | uint64_t | 8 | 18446744073709551615 |

## State Transitions

```
memlayer_create → [READY]
    │
    ├── memlayer_forward(router_signal) → cells updated (trigger + decay)
    ├── memlayer_query(trigger_mask) → mask[i] = cells[i] > 0
    ├── memlayer_save(filepath) → binary blob on disk
    ├── memlayer_load(filepath) → new MemLayer* from disk
    ├── memlayer_all_zero() → bool
    └── memlayer_destroy → [FREED]
```

## Validation Rules

| 检查 | 条件 | 结果 |
|------|------|------|
| 空指针 | layer/router_signal/filepath == NULL | 返回错误码或安全跳过 |
| 无效精度 | precision > 3 | memlayer_create 返回 NULL |
| 长度为零 | length == 0 | memlayer_create 返回 NULL |
| 文件不存在 | fopen 返回 NULL | memlayer_load 返回 NULL |
| 损坏数据 | 文件大小 ≠ header + N×cellsize | memlayer_load 返回 NULL |
