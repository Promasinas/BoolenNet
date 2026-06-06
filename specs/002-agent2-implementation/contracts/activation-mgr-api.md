# Contract: Activation Manager API

**Provider**: Agent 2 (activation_mgr)
**Location**: `src/activation_mgr.h`
**Consumers**: Agent 1 (Boolean Router), Forward function

## API Functions

```c
// Lifecycle
int  activation_mgr_init(uint32_t P, uint32_t D);
void activation_mgr_reset(void);

// Core read/write
int activation_mgr_write(uint32_t address, const float *activation);
int activation_mgr_read(uint32_t address, float *out);

// Trigger weighted-sum
int activation_mgr_trigger(const trigger_pattern_t *pattern, float *out);

// Status
uint32_t activation_mgr_occupied_count(void);
float    activation_mgr_fill_ratio(void);
int      activation_mgr_get_status(uint32_t address, partition_status_t *out);
int      activation_mgr_get_bitmask(uint8_t *out_bitmask);
```

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | OK | Success |
| -1 | ERR_BAD_ADDRESS | Address out of [0, P-1] |
| -2 | ERR_NULL_PARAM | NULL pointer |
| -3 | ERR_NOT_INIT | Called before init |
| -4 | ERR_DIM_MISMATCH | Pattern dim != configured D |
| -5 | ERR_ALLOC | Memory allocation failure |
