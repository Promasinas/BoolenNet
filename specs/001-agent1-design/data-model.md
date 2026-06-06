# Data Model: Memory 层模块

## Entity: MemoryLayer

| 字段 | 类型 | 说明 |
|------|------|------|
| uid | uint32_t | 全局唯一标识符 |
| precision | uint8_t | 0=uint8, 1=uint16, 2=uint32, 3=uint64 |
| length | uint32_t | 记忆单元数量 |
| max_value | uint64_t | 触发恢复上限值 |
| decay | uint64_t | 每 forward 自减值 |
| cells | void* | 动态数组，类型由 precision 决定 |

## PrecisionType

| 值 | 类型 | sizeof | 范围 |
|----|------|--------|------|
| 0 | uint8_t | 1 | 0..255 |
| 1 | uint16_t | 2 | 0..65535 |
| 2 | uint32_t | 4 | 0..4.29e9 |
| 3 | uint64_t | 8 | 0..1.84e19 |

## Error Codes

ML_OK=0, ML_ERR_NULL_PARAM=-1, ML_ERR_INVALID_LENGTH=-2,
ML_ERR_INVALID_PRECISION=-3, ML_ERR_ALLOC_FAILED=-4,
ML_ERR_FILE_OPEN=-5, ML_ERR_FILE_READ=-6,
ML_ERR_FILE_WRITE=-7, ML_ERR_CORRUPTED_DATA=-8

## State: forward() 变换

```
router_signal[i]=1 → cell[i] = max_value
router_signal[i]=0 → cell[i] = max(0, cell[i] - decay)
```
