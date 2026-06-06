# Quickstart: 布尔路由器

```c
#include "bool_router.h"

// 创建路由器: UID=1, 长度8, 路由参数全0（恒等）
uint8_t bits[1] = {0x00};
BoolRouter *r = bool_router_create(1, 8, bits);

// 输入 [1,0,1,0,1,0,1,0] → 包为 0x55
uint8_t input[1]  = {0x55};
uint8_t output[1];

// 路由参数 [1,1,1,1,1,1,1,1] → 0xFF（全翻转）
uint8_t flip[1] = {0xFF};
BoolRouter *r2 = bool_router_create(2, 8, flip);

bool_router_forward(r,  input, output);  // output = 0x55 (不变)
bool_router_forward(r2, input, output);  // output = 0xAA (全翻)

bool_router_destroy(r);
bool_router_destroy(r2);
```
