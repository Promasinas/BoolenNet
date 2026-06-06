# Contract: Boolean Router C API

## Types
```c
typedef struct {
    uint32_t uid, length;
    uint8_t *bits;
} BoolRouter;
```

## Functions
```c
BoolRouter* bool_router_create(uint32_t uid, uint32_t length, const uint8_t *bits);
void        bool_router_destroy(BoolRouter *r);
void        bool_router_forward(const BoolRouter *r, const uint8_t *input, uint8_t *output);
int         bool_router_forward_batch(BoolRouter **routers, uint32_t count,
                                      const uint8_t *input, uint8_t **outputs);
int         bool_router_save(const BoolRouter *r, const char *path);
BoolRouter* bool_router_load(const char *path);
```

## 运算
```
output[i] = input[i] ^ router->bits[i]   // per-bit XOR
```
