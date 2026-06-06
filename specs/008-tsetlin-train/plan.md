# Implementation Plan: Tsetlin 自动机训练

**Branch**: `008-tsetlin-train` | **Date**: 2026-06-06

## Summary

Tsetlin Automaton 训练器：逐层随机单 bit 翻转 + forward 评估 + 奖励驱动保留/回退。
纯 C 实现，依赖 forward 模块。

## Dependencies

boolnet.o (forward), bool_router.o, memory_layer.o

## Constitution Check

| 原则 | 状态 |
|------|------|
| IX. Forward | ⚠️ 依赖 |
| X. Tsetlin | ✅ 本模块 |

## Structure

```text
src/
├── tsetlin_train.h / .c
└── Makefile
```
