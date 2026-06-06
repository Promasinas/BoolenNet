# Implementation Plan: 多路由上采样

**Branch**: `006-upsampling` | **Date**: 2026-06-06

## Summary

M 个等大布尔路由器并行处理同一输入，结果拼接为 M×N 输出。
依赖 bool_router 模块。纯 C 编译。

## Technical Context

**Language**: C (C99+)
**Dependencies**: bool_router.o
**Target**: Windows/MinGW

## Constitution Check

| 原则 | 状态 |
|------|------|
| IV. 布尔路由器 | ⚠️ 依赖 |
| VI. 多路由上采样 | ✅ 本模块 |

## Structure

```text
src/
├── upsampling.h / .c
└── Makefile
```
