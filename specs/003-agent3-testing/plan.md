# Implementation Plan: Agent 3 — 自动化测试

**Branch**: `003-agent3-testing` | **Date**: 2026-06-06 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/003-agent3-testing/spec.md`

## Summary

Agent 3 是 BoolNet 三 Agent 架构中的测试 Agent，负责对 `./src/` 中的 C 语言模块执行完全自动化的测试流水线：

```
检测代码变更 → 创建测试目录(按UID) → 生成Makefile → 编译 → 运行 → 生成测试文档 → Git提交
```

通过 `./docs/interact/` 中的约定式 Markdown 文档与 Agent 1/Agent 2 通信，支持多模块完全并行测试。
以心跳文件（`./docs/interact/.agent3_heartbeat`）提供存活性信号，60 秒去重窗口合并重复触发，
测试失败不重试直接报告。

## Technical Context

**Language/Version**: C (C99+) + Bash shell scripts

**Primary Dependencies**: GCC/MinGW, GNU Make, Git CLI（均为 Windows 平台系统工具）

**Storage**: 文件系统 — Markdown 文档于 `./docs/interact/`、`./docs/test/`；测试代码于 `./test/`

**Testing**: Agent 3 自身为测试自动化工具，通过端到端验证（放置源码 → 观察完整流水线执行）确认正确性

**Target Platform**: Windows（MinGW/MSYS2 环境，Bash 可用）

**Project Type**: CLI 工具 / CI-like Agent

**Performance Goals**:
- 单模块完整测试流水线（不含测试运行时间）< 5 分钟
- 3 模块并行效率 ≥ 67%（SC-007）

**Constraints**:
- 并发编译进程数 ≤ CPU 核心数（自动检测，FR-016）
- 轮询周期 ≤ 60 秒（FR-001）
- 单模块测试超时由 Agent 2 在 `## Test Params` 的 `timeout` 字段指定，无默认值（FR-011）
- 去重窗口 60 秒（与轮询周期对齐）
- 心跳文件每 60 秒更新，3 分钟未更新视为异常（FR-018）
- 72 小时持续运行无内存泄漏（SC-006）
- 测试失败不执行自动重试（FR-017）

**Scale/Scope**: 支持 1–N 个模块并行测试（N ≤ CPU 核心数）

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| 原则 | 状态 | 说明 |
|------|------|------|
| I. 双语文档与沟通 | ✅ PASS | 交互文档使用中文，代码注释使用英文 |
| II. 多Agent协作分工 | ✅ PASS | 严格遵循 Agent 3 职责：测试目录 `./test/`、结果 `./docs/test/`、交互 `./docs/interact/` |
| III. 历史上下文参考 | ✅ PASS | 设计参考 `./docs/history/` 中 BoolNet 架构讨论 |
| IV. 布尔路由器 | N/A | Agent 3 不实现，仅测试其正确性 |
| V. 一维回环卷积 | N/A | Agent 3 不实现 |
| VI. 多路由上采样 | N/A | Agent 3 不实现 |
| VII. Layer UID 与持久化 | ✅ PASS | 测试目录按 Layer UID 命名，测试文档按 UID 索引 |
| VIII. Memory 层设计 | N/A | Agent 3 不实现 |
| IX. Forward 函数 | N/A | Agent 3 不实现 |
| X. Tsetlin 自动机训练 | N/A | Agent 3 不实现 |
| 开发顺序（步骤 4） | ✅ PASS | 检测新代码 → 自动测试 → 结果放 `./docs/test/` → Git 版本管理 |
| 目录结构约定 | ✅ PASS | 输出严格限制在 `./test/`、`./docs/test/`、`./docs/interact/` |

**Gate Result**: ALL PASS — 无违规。

## Project Structure

### Documentation (this feature)

```text
specs/003-agent3-testing/
├── plan.md              # 本文件
├── research.md          # Phase 0 输出
├── data-model.md        # Phase 1 输出
├── quickstart.md        # Phase 1 输出
├── contracts/           # Phase 1 输出
│   ├── test-request-format.md
│   ├── completion-notification-format.md
│   ├── makefile-contract.md
│   └── test-report-format.md
├── checklists/
│   └── requirements.md  # 规格质量检查清单
└── tasks.md             # Phase 2 输出 (/speckit-tasks)
```

### Source Code (repository root)

```text
# Agent 3 核心代码（独立于 ./src/ 被测试代码）
agent3/
├── main.c               # 主循环入口：轮询调度、信号处理
├── main.h
├── scanner.c             # 变更检测：./src/ 和 ./docs/interact/ 监控
├── scanner.h
├── pipeline.c            # 测试流水线管理：创建目录→编译→运行→文档→提交
├── pipeline.h
├── makefile_gen.c        # Makefile 模板生成（all + clean 目标）
├── makefile_gen.h
├── process_mgr.c         # 进程管理：CreateProcess、超时控制、并发限制
├── process_mgr.h
├── git_mgr.c             # Git 操作封装：add + commit、锁竞争处理
├── git_mgr.h
├── reporter.c            # 文档生成：./docs/test/ 测试报告 + ./docs/interact/ 通知
├── reporter.h
├── interact.c            # 交互文档解析：Markdown 标题约定解析
├── interact.h
├── heartbeat.c           # 心跳文件管理：./docs/interact/.agent3_heartbeat
├── heartbeat.h
├── dedup.c               # 去重管理：基于 UID 的 60 秒哈希表去重
├── dedup.h
├── config.c              # 配置管理：轮询周期、并发上限、超时默认值
├── config.h
├── utils.c               # 通用工具：路径处理、日志、时间戳
├── utils.h
└── Makefile              # Agent 3 自身编译

# 被测试的源代码 (Agent 2 输出)
src/                       # BoolNet 框架模块
├── bool_router.c
├── conv1d_circular.c
├── upsampling.c
├── memory_layer.c
├── forward.c
└── tsetlin_train.c

# Agent 3 运行时创建的测试目录
test/
└── uid_NNN/               # 按 Layer UID 命名
    ├── module_info.txt    # 模块元数据
    ├── Makefile           # 自动生成
    └── test_*.c           # 测试代码

# 测试结果文档（运行时生成）
docs/test/
└── uid_NNN_report.md

# Agent 间交互（运行时读写）
docs/interact/
├── .agent3_heartbeat      # 心跳文件（每 60 秒更新）
├── test_request_uid_*.md  # Agent 2 → Agent 3 测试请求
├── test_done_uid_*.md     # Agent 3 → Agent 1/2 完成通知
└── error_uid_*.md         # Agent 3 → Agent 1 错误报告
```

**Structure Decision**: Agent 3 核心代码独立于 `agent3/` 目录，与 `./src/`（BoolNet 框架本身）隔离。
测试产出（`test/`、`docs/test/`）遵循宪章约定的目录结构。Agent 3 编译产物为 `agent3.exe`。

## Complexity Tracking

> 无宪章违规，无需记录。
