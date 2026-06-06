# Data Model: Agent 3 — 自动化测试

**Feature**: Agent 3 — 自动化测试
**Date**: 2026-06-06

## Entity Relationship

```
TestRequest ──→ TestModule ──→ TestPipeline ──→ TestReport
                    │                │
                    │                ├──→ CompileResult
                    │                └──→ RunResult
                    │
                    └──→ GitCommit
```

## Entities

### 1. TestModule (测试模块)

对应 `./src/` 中的一个待测试源代码模块。

| 字段 | 类型 | 说明 |
|------|------|------|
| `uid` | `uint32_t` | Layer UID，主键，全局唯一 |
| `module_name` | `char[64]` | 模块名，辅助标识（如 `bool_router`） |
| `src_path` | `char[256]` | 源文件绝对路径（如 `./src/bool_router.c`） |
| `test_dir` | `char[256]` | 测试目录路径（如 `./test/uid_001/`） |
| `last_modified` | `uint64_t` | 最近变更时间戳（`ftLastWriteTime` 转换） |
| `state` | `enum` | 测试状态（见状态机） |

**唯一性规则**: `uid` 全局唯一；`module_name` 在检测周期内唯一（同周期不重复测试）。

### 2. TestPipeline (测试流水线)

表示一个模块从检测到提交的完整测试流程。

| 字段 | 类型 | 说明 |
|------|------|------|
| `pipeline_id` | `uint32_t` | 流水线 ID（自增） |
| `module_uid` | `uint32_t` | 关联的 TestModule.uid |
| `phase` | `enum` | 当前阶段（见状态机） |
| `start_time` | `uint64_t` | 流水线启动时间戳 |
| `timeout` | `uint32_t` | 超时上限（秒），由 Agent 2 在 `## Test Params` 的 `timeout` 字段显式指定，无默认值。编译和运行阶段共用。缺失则拒绝指令 |
| `process_handle` | `HANDLE` | 当前活跃的子进程句柄 |
| `result` | `PipelineResult` | 流水线结果（完成后填写） |

### 3. PipelineResult (流水线结果)

| 字段 | 类型 | 说明 |
|------|------|------|
| `compile_ok` | `bool` | 编译是否成功 |
| `compile_output` | `char*` | 编译标准输出/错误（堆分配） |
| `compile_duration_ms` | `uint32_t` | 编译耗时（毫秒） |
| `run_ok` | `bool` | 运行是否成功 |
| `run_output` | `char*` | 运行标准输出/错误（堆分配） |
| `run_duration_ms` | `uint32_t` | 运行耗时（毫秒） |
| `exit_code` | `int` | 测试进程退出码 |
| `timed_out` | `bool` | 是否超时终止 |
| `pass_count` | `uint32_t` | 通过测试数（解析 `run_output` 提取） |
| `fail_count` | `uint32_t` | 失败测试数 |

### 4. TestReport (测试文档)

存放于 `./docs/test/uid_NNN_report.md` 的结构化文档。

| 字段 | 类型 | 说明 |
|------|------|------|
| `report_path` | `char[256]` | 文档文件路径 |
| `module_uid` | `uint32_t` | 关联的 TestModule.uid |
| `module_name` | `char[64]` | 模块名（辅助） |
| `test_time` | `char[32]` | 测试时间（YYYY-MM-DD HH:MM:SS） |
| `compile_status` | `bool` | 编译状态 |
| `run_status` | `bool` | 运行状态 |
| `pass_count` | `uint32_t` | 通过数 |
| `fail_count` | `uint32_t` | 失败数 |
| `error_details` | `char*` | 错误详情（Markdown 格式，堆分配） |

**文档格式约定**: 使用 Markdown，固定标题层级。参见 `contracts/test-report-format.md`。

### 5. InteractionDirective (交互指令)

存放于 `./docs/interact/` 的通信文档，分两种子类型。

#### 5a. TestRequest (Agent 2 → Agent 3)

文件名模式: `test_request_uid_NNN.md`

| 字段 | 来源标题 | 说明 |
|------|----------|------|
| `target_uid` | `## Target UID` | 必填，模块 Layer UID |
| `target_module` | `## Target Module` | 必填，模块名 |
| `test_priority` | `## Test Priority` | 必填，`high` / `medium` / `low` |
| `test_params.timeout` | `## Test Params` | 必填，`timeout: <秒>` |
| `test_params.extra_flags` | `## Test Params` | 可选，`cflags: <额外编译标志>` |

**校验规则**: 缺少必填字段 → 拒绝 + 记录解析错误（FR-015）。

#### 5b. CompletionNotification (Agent 3 → Agent 1/Agent 2)

文件名模式: `test_done_uid_NNN.md`

| 字段 | 标题 | 说明 |
|------|------|------|
| `status` | `## Status` | `completed` 或 `error` |
| `module` | `## Module` | 模块 UID 和名称 |
| `result` | `## Result` | `pass` / `fail` / `compile_error` / `timeout` |
| `summary` | `## Summary` | 通过/失败统计摘要 |

### 6. ErrorReport (错误报告)

文件名模式: `error_uid_NNN.md`，仅当发生不可恢复错误时由 Agent 3 生成。

| 字段 | 标题 | 说明 |
|------|------|------|
| `status` | `## Status` | `error` |
| `module` | `## Module` | 出错模块 UID 和名称 |
| `error_detail` | `## Error Detail` | 完整错误描述 |

### 7. GitCommit (Git 提交记录)

| 字段 | 类型 | 说明 |
|------|------|------|
| `commit_hash` | `char[41]` | SHA-1 哈希 |
| `message` | `char[256]` | 提交消息：`[Agent3] Test uid_NNN (module) - YYYY-MM-DD HH:MM` |
| `files` | `char**` | 涉及的文件路径列表 |
| `timestamp` | `uint64_t` | 提交时间戳 |

## State Machines

### TestModule State

```
                    ┌──────────┐
         ┌─────────→│  IDLE    │←─────────┐
         │          └────┬─────┘          │
         │               │ 检测到变更      │
         │          ┌────▼─────┐          │
         │          │  PENDING  │          │
         │          └────┬─────┘          │
         │               │ 流水线启动      │
         │          ┌────▼─────┐          │
         │          │ COMPILING│          │
         │          └────┬─────┘          │
         │               │               │
         │     ┌─────────┼─────────┐     │
         │     │         │         │     │
         │  ┌──▼──┐ ┌───▼───┐ ┌──▼──┐  │
         │  │FAIL │ │RUNNING│ │DONE │  │
         │  │(编译 │ └───┬───┘ │(完成)│  │
         │  │ 失败)│     │     └──────┘  │
         │  └──┬──┘     │               │
         │     │   ┌────┼────┐          │
         │     │   │    │    │          │
         │     │ ┌─▼─┐┌─▼─┐┌─▼──┐     │
         │     │ │OK ││FAIL││TIME│      │
         │     │ │通过││失败││超时│      │
         │     │ └──┬┘└─┬─┘└─┬──┘      │
         │     │    │   │    │          │
         │     └────┴───┴────┘          │
         │              │ 文档+提交完成   │
         └──────────────┘               │
                                         │
              重新检测（同模块再次修改）──┘
```

### Pipeline Phase

```
DETECT → CREATE_DIR → GENERATE_MAKEFILE → COMPILE → RUN → ANALYZE → REPORT → COMMIT
   │                      │                  │        │       │         │        │
   │                      │                  │        │       │         │        │
   └── 任何阶段失败 ──→ 记录错误 ──→ REPORT（错误报告）──→ COMMIT（可选）
```

## Concurrency Model

```
主进程（轮询循环）
  │
  ├── scan_src_dir()         扫描 ./src/ 变更
  ├── scan_interact_dir()    扫描 ./docs/interact/ 新请求
  │
  ├── 对每个待测试模块:
  │     pipeline = pipeline_create(module)
  │     pipeline_list.push(pipeline)
  │
  ├── 并发窗口管理:
  │     active = pipeline_list.filter(phase != IDLE && phase != DONE)
  │     while active.size < MAX_CONCURRENT && pending.size > 0:
  │         next = pending.pop()
  │         CreateProcess(next, phase=COMPILE)
  │         active.push(next)
  │
  ├── WaitForMultipleObjects(active.processes)
  │     完成者 → 推进 phase → 若 phase != DONE → CreateProcess 下一阶段
  │
  └── Git 操作:
        AcquireMutex(git_mutex)  # 全局互斥
        git_commit(completed_pipeline)
        ReleaseMutex(git_mutex)
```
