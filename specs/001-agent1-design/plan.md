# Implementation Plan: Memory 层模块

**Branch**: `001-agent1-design` | **Date**: 2026-06-06 | **Spec**: [spec.md](./spec.md)

## Summary

Memory 层是 BoolNet 框架的有状态核心组件。实现为纯 C 语言独立编译单元，通过预处理器宏覆盖四种精度变体（uint8/16/32/64）。核心接口：create / destroy / forward / query / save / load。

## Technical Context

**Language/Version**: C (C99+) — 宪章要求
**Primary Dependencies**: 无外部依赖，仅 stdint.h / stdlib.h / string.h
**Storage**: 紧凑二进制文件（25 字节头 + cells 数据）
**Target Platform**: Windows (MinGW/GCC)
**Project Type**: 库模块 → memory_layer.o
**Performance**: 10000 单元 forward < 1ms; 1000 次生命周期无泄漏; 加载 100% 精确恢复

## Constitution Check

| 原则 | 状态 | 说明 |
|------|------|------|
| I. 双语文档 | ✅ PASS | 规格中文，代码注释英文 |
| II. Agent 分工 | ✅ PASS | Agent 1 设计 → Agent 2 实现 → Agent 3 测试 |
| VII. UID 持久化 | ✅ PASS | 直接实现 |
| VIII. Memory 层 | ✅ PASS | **本模块即是此原则的实现** |
| 目录约定 | ✅ PASS | 源代码 → ./src/ |

**Gate Result**: ALL PASS

## Project Structure

```text
src/
├── boolnet_common.h   # 框架公共定义
├── memory_layer.h     # 公共接口
├── memory_layer.c     # 全部实现
└── Makefile           # 编译 memory_layer.o

specs/001-agent1-design/
├── spec.md, plan.md, research.md, data-model.md
├── quickstart.md, tasks.md
└── contracts/memory-layer-api.md
```
