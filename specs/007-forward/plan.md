# Implementation Plan: Forward 函数

**Branch**: `007-forward` | **Date**: 2026-06-06

## Summary

BoolNet 推理引擎：串联所有已注册层（Router → Conv → Upsample → Memory），
按 UID 顺序 forward。依赖所有底层模块。纯 C 实现。

## Dependencies

bool_router.o, conv1d_circular.o, upsampling.o, memory_layer.o, activation_mgr.o

## Constitution Check

| 原则 | 状态 |
|------|------|
| IV–VIII | ⚠️ 依赖底层模块 |
| IX. Forward | ✅ 本模块 |

## Structure

```text
src/
├── boolnet.h / .c     # 网络定义 + forward + save/load
└── Makefile
```
