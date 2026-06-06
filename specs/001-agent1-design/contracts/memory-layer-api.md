# Contract: Memory Layer C API

## Types
```c
typedef enum { ML_PRECISION_UINT8=0, ML_PRECISION_UINT16=1,
               ML_PRECISION_UINT32=2, ML_PRECISION_UINT64=3 } MemLayerPrecision;

typedef struct {
    LayerUID uid; uint8_t precision; uint32_t length;
    uint64_t max_value, decay; void* cells;
} MemLayer;
```

## Functions
```c
MemLayer* memlayer_create(LayerUID, uint8_t precision, uint32_t length, uint64_t max, uint64_t decay);
void      memlayer_destroy(MemLayer*);
void      memlayer_forward(MemLayer*, const uint8_t* router_signal);
void      memlayer_query(const MemLayer*, uint8_t* trigger_mask);
int       memlayer_save(const MemLayer*, const char* filepath);
MemLayer* memlayer_load(const char* filepath);
int       memlayer_all_zero(const MemLayer*);
int       memlayer_verify_roundtrip(const MemLayer*);
```

## Constraints
- length > 0, precision ∈ {0,1,2,3}
- 非线程安全
- 调用方保证 router_signal/trigger_mask 缓冲区大小 ≥ length
- save/load 二进制格式：头部 25B + cells 数据
