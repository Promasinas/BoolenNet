# Implementation Plan: 一维回环卷积

**Branch**: `005-circular-conv` | **Date**: 2026-06-06 | **Spec**: [spec.md](./spec.md)

## Summary

一维回环卷积：核滑出末尾时回环取输入开头。支持 stride、dilation，无 padding。
纯 C 实现，通过宏支持 int8/16/32/64 类型。

## Technical Context

**Language**: C (C99+), 无外部依赖
**Target**: Windows (MinGW/GCC)
**Performance**: 10000 元素 kernel_size=3 < 2ms

## Constitution Check

| 原则 | 状态 |
|------|------|
| I. 双语文档 | ✅ |
| V. 回环卷积 | ✅ 本模块 |
| VII. UID | ✅ save/load |

## Structure

```text
src/
├── conv1d_circular.h / .c
└── Makefile
```
