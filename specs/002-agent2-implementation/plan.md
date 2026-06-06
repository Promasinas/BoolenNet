# Implementation Plan: Agent 2 — Memory & Activation Management

**Branch**: `002-agent2-implementation` | **Date**: 2026-06-06 | **Spec**: [spec.md](./spec.md)

## Summary

Agent 2 实现分区记忆与激活管理系统：P 个固定大小分区，每分区存储 D 维浮点激活向量。
支持 O(1) 直接寻址读写、稀疏触发器加权求和释放（含负权重）、LRU 淘汰、状态监控。
已有底层组件 `activation.c/h`、`partition.c/h`、`trigger.c/h`，本模块组合为统一高层接口 `activation_mgr`。

## Technical Context

**Language/Version**: C (C99+)

**Primary Dependencies**: 标准 C 库（`<stdlib.h>`, `<string.h>`, `<math.h>` — `fmaf` 加权累加）

**Storage**: 纯内存（v1 无持久化）— P × D × sizeof(float) + LRU 链表 + 位图 + 时间戳

**Testing**: Agent 3 自动化测试框架

**Target Platform**: Windows (MinGW/GCC)

**Project Type**: 库模块 → `activation_mgr.o`

**Performance Goals**:
- Read/Write: O(1) (SC-001)
- Trigger 加权求和: O(K)，K = 被触发的已占用分区数 (SC-002, SC-004)
- 稀疏触发器不遍历全部分区

**Constraints**:
- 单线程顺序执行
- P ∈ [256, 65536], D 初始化时固定 (FR-014)
- LRU 默认淘汰 (FR-007)
- 激活向量维度校验 (FR-006)

**Scale/Scope**: 主模块 `activation_mgr.c/h` (~400 行) + 3 个已有子组件

## Constitution Check

| 原则 | 状态 | 说明 |
|------|------|------|
| I. 双语文档 | ✅ PASS | 代码注释英文 |
| II. 多Agent协作 | ✅ PASS | Agent 2 职责：`./src/`、`./docs/api/`、`./checkout/` |
| III. 历史上下文 | ✅ PASS | 基于分区寻址架构 |
| IV. 布尔路由器 | ⚠️ 依赖 | 消费 Agent 1 路由器输出的 partition address |
| VII. Layer UID | ✅ PASS | 分区可关联 UID |
| VIII. Memory 层 | ⚠️ 互补 | Memory 层管理逐单元状态，本模块管理分区级激活向量 |
| IX. Forward 函数 | ⚠️ 依赖 | Forward 调用 trigger 获取加权和 |
| 目录约定 | ✅ PASS | 源码 → `./src/`，API 文档 → `./docs/api/` |

**Gate Result**: ALL PASS

## Project Structure

```text
src/
├── activation_mgr.h     # 统一接口: write/read/trigger/status/reset
├── activation_mgr.c     # 实现: 分区数组 + LRU 链表 + 状态位图
├── activation.c/h       # 激活向量工具 (已存在)
├── partition.c/h        # 分区管理 (已存在)
├── trigger.c/h          # 触发器处理 (已存在)
└── Makefile

docs/api/
└── activation_mgr_api.md
```

**Structure Decision**: `activation_mgr` 组合已有子组件，提供统一高层接口。

## Complexity Tracking

> 无宪章违规，无需记录。
