# Implementation Plan: 布尔路由器

**Branch**: `004-bool-router` | **Date**: 2026-06-06 | **Spec**: [spec.md](./spec.md)

## Summary

布尔路由器是 BoolNet 最基础运算单元：输入 N 位序列与等长路由参数按位 XOR。
支持单路由、批量多路由并行、UID 持久化。实现为纯 C 独立编译单元。

## Technical Context

**Language/Version**: C (C99+)
**Primary Dependencies**: 无外部依赖，仅 stdint.h / stdlib.h
**Target Platform**: Windows (MinGW/GCC)
**Project Type**: 库模块 → bool_router.o
**Performance**: 10000 位 < 0.5ms

## Constitution Check

| 原则 | 状态 |
|------|------|
| I. 双语文档 | ✅ |
| II. Agent 分工 | ✅ |
| IV. 布尔路由器 | ✅ 本模块即此原则的实现 |
| VII. UID 持久化 | ✅ save/load 实现 |

## Project Structure

```text
src/
├── bool_router.h       # 公共接口
├── bool_router.c       # 实现
└── Makefile            # 增加 bool_router.o 目标
```
