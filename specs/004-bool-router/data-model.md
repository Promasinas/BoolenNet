# Data Model: 布尔路由器

## Entity: BoolRouter

| 字段 | 类型 | 说明 |
|------|------|------|
| uid | uint32_t | 全局唯一标识符 |
| length | uint32_t | 位序列长度（正整数） |
| bits | uint8_t* | 路由参数位数组（ceil(N/8) 字节） |

## Bits 打包格式

bits[0] 的 bit0–bit7 = 输入位 0–7，LSB 优先。

## 核心运算

```
output[i] = input[i] XOR router_bit[i]  # 每个位独立
```
