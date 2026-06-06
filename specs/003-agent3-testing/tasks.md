# Tasks: Agent 3 — 自动化测试

**Input**: Design documents from `/specs/003-agent3-testing/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: 无——规格未要求独立测试框架，Agent 3 自身即为测试 Agent，正确性通过 `quickstart.md` 端到端验证确认。

**Organization**: 任务按用户故事分组，支持独立实现与测试。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 可并行执行（不同文件、无依赖）
- **[Story]**: 所属用户故事（US1–US5）
- 描述中包含精确文件路径

## Path Conventions

Agent 3 核心代码位于 `agent3/` 目录，运行时操作 `./src/`、`./test/`、`./docs/`。

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 创建项目骨架、构建系统和共享基础设施

- [x] T001 创建 Agent 3 目录结构 `agent3/` 及所有子模块占位文件，参见 plan.md 项目结构
- [x] T002 [P] 实现通用工具模块 `agent3/utils.h` 和 `agent3/utils.c`（路径处理 `path_join`/`path_to_windows`/`path_to_unix`、日志 `log_info`/`log_error`/`log_warn`/`log_debug`、时间戳 `get_timestamp_ms`/`format_time_iso8601`、磁盘空间检查 `check_disk_space(min_free_mb=100)`）
- [x] T003 [P] 实现配置管理模块 `agent3/config.h` 和 `agent3/config.c`（常量：`POLL_INTERVAL_SEC=60`、`MAX_CONCURRENT`=CPU 核心数、`DEDUP_WINDOW_SEC=60`、`HEARTBEAT_TIMEOUT_SEC=180`、`SRC_DIR="./src/"`、`INTERACT_DIR="./docs/interact/"`、`TEST_DIR="./test/"`、`DOCS_TEST_DIR="./docs/test/"`；支持命令行参数 `--poll-interval`/`--max-concurrent`/`--once`/`--verbose` 覆盖；注意：超时无默认常量，由 Agent 2 指令中的 `timeout` 字段指定）
- [x] T004 实现 Agent 3 主入口 `agent3/agent3.c` 和 `agent3/agent3.h`（初始化日志/配置/模块注册、命令行参数解析、主循环骨架：轮询→扫描→调度→休眠）
- [x] T005 创建 Agent 3 构建文件 `agent3/Makefile`（`all` 目标编译 `agent3.exe`、`clean` 目标清理产物、依赖 MinGW/GCC 工具链、`CFLAGS=-Wall -Wextra -std=c11`）

**Checkpoint**: 可执行 `make` 编译出 `agent3.exe`（空主循环，仅输出启动日志和配置摘要）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: 所有用户故事依赖的核心基础设施——模块状态管理、去重、心跳

**⚠️ CRITICAL**: 所有用户故事实现必须等待本阶段完成

- [x] T006 [P] 实现模块状态跟踪器 `agent3/module_tracker.h` 和 `agent3/module_tracker.c`
- [x] T007 [P] 实现心跳模块 `agent3/heartbeat.h` 和 `agent3/heartbeat.c`
- [x] T008 将模块跟踪器和心跳集成到主循环 `agent3/agent3.c`

**Checkpoint**: 主循环可运行，模块跟踪器可注册/查询/去重模块，心跳文件每分钟更新

---

## Phase 3: User Story 1 — 自动检测代码变更并触发测试 (Priority: P1) 🎯 MVP

**Goal**: Agent 3 周期性扫描 `./src/` 和 `./docs/interact/`，检测新代码/变更和新测试指令，将模块注册到跟踪器并标记为 PENDING

**Independent Test**: 在 `./src/` 放置 .c 文件或在 `./docs/interact/` 放入测试请求文档后，Agent 3 日志输出检测到的模块列表，模块状态变为 PENDING

### Implementation for User Story 1

- [x] T009 [P] [US1] 实现文件系统扫描器 `agent3/detector.h` 和 `agent3/detector.c`（使用 Windows `FindFirstFile`/`FindNextFile` API 扫描 `./src/` 中的 `.c`/`.h` 文件；比对 `ftLastWriteTime` 与 module_tracker 记录的时间戳检测新增/变更；返回变更模块列表；处理空目录和无效路径的容错）
- [x] T010 [P] [US1] 实现交互指令解析器 `agent3/interact.h` 和 `agent3/interact.c`——请求解析部分（扫描 `./docs/interact/test_request_*.md`；按约定式 Markdown 标题 `## Target UID`/`## Target Module`/`## Test Priority`/`## Test Params` 解析字段；提取 `timeout` 参数（必填，无默认值）；缺少必填字段（UID、Module、Priority、timeout）时记录解析错误并拒绝该指令；FR-002、FR-011、FR-015）
- [x] T011 [US1] 实现去重逻辑 `agent3/detector.c`（60 秒去重窗口：同一 UID 在 `last_tested_at + 60s` 内由文件变更触发的重复检测合并为一次；手动交互指令（来自 `test_request_*.md`）触发的测试不受去重窗口限制——每次独立执行；FR-017 不重试策略：仅适用于文件变更，手动触发不合并）
- [x] T012 [US1] 将检测结果集成到主循环 `agent3/agent3.c`（每轮轮询调用 `scan_src_dir()` → `scan_interact_dir()` → 去重检查 → 写入 `module_tracker` 标记 PENDING → 日志输出检测摘要 `[检测] 发现 N 个新模块，M 个重复(已合并)`）

**Checkpoint**: Agent 3 可检测新文件和交互请求，模块正确注册到跟踪器，去重逻辑和 60 秒窗口生效

---

## Phase 4: User Story 2 — 自动建立测试目录结构 (Priority: P2)

**Goal**: Agent 3 为每个 PENDING 模块在 `./test/uid_NNN/` 下自动创建测试目录、初始测试代码框架和 Makefile

**Independent Test**: 给定一个注册为 PENDING 的模块（uid=1, module=bool_router），Agent 3 在 `./test/uid_001/` 下创建目录、`test_main.c` 框架、`module_info.txt` 和 `Makefile`

### Implementation for User Story 2

- [x] T013 [P] [US2] 实现测试目录管理器 `agent3/testdir.h` 和 `agent3/testdir.c`（按 Layer UID 创建 `./test/uid_NNN/` 目录结构；首次创建时若 `./test/` 不存在则先创建根目录；若目录已存在则复用仅更新变化的文件；生成 `module_info.txt` 记录 `UID=N` 和 `Module=name` 元数据；磁盘空间不足时检测并报错；FR-003）
- [x] T014 [P] [US2] 实现 Makefile 生成器 `agent3/makefile_gen.h` 和 `agent3/makefile_gen.c`（从内嵌模板生成 `./test/uid_NNN/Makefile`：填充 `TARGET=test_uid_NNN.exe`、`SRCS=test_main.c ../../src/module.c`、`CFLAGS`；包含 `all` 和 `clean` 目标；Windows 路径 `\` → Makefile Unix `/` 转换；支持 `cflags` 额外编译标志（来自 `## Test Params` 可选字段）；FR-004）
- [x] T015 [US2] 实现测试代码框架生成 `agent3/testdir.c`（为每个测试目录生成初始 `test_main.c`：包含 `#include` 对应源模块头文件、`main()` 入口、占位 `printf("PASS: test_name\n")` / `printf("FAIL: test_name | Input: ... | Expected: ... | Actual: ...\n")` 输出格式的注释示例；框架不包含具体测试断言逻辑——由 Agent 2 后续填充）
- [x] T016 [US2] 将测试目录创建集成到流水线 `agent3/agent3.c`（PENDING 模块 → 调用 `create_test_dir()` → `generate_makefile()` → `generate_test_framework()` → 状态更新为 COMPILING）

**Checkpoint**: 对任意 PENDING 模块，Agent 3 可自动创建完整测试目录结构并生成可用 Makefile

---

## Phase 5: User Story 3 — 自动编译与运行测试 (Priority: P2)

**Goal**: Agent 3 调用 Makefile 编译测试代码，运行编译产物，捕获输出，处理超时

**Independent Test**: 对一个已有测试目录和 Makefile 的模块，Agent 3 能执行 `make` 编译、运行可执行文件、捕获 stdout/stderr、超时终止

### Implementation for User Story 3

- [x] T017 [P] [US3] 实现进程管理器 `agent3/compiler.h` 和 `agent3/compiler.c`（使用 Windows `CreateProcess` 调用 `make all`，工作目录设为 `./test/uid_NNN/`；通过匿名管道捕获 stdout/stderr；解析编译退出码判断成功/失败；编译失败时记录完整错误输出到 PipelineResult；FR-005、FR-010）
- [x] T018 [P] [US3] 实现测试运行器 `agent3/runner.h` 和 `agent3/runner.c`（使用 `CreateProcess` 运行编译产物 `test_uid_NNN.exe`；通过管道捕获 stdout/stderr；超时控制：使用 `WaitForSingleObject(process_handle, timeout_ms)`，超时后 `TerminateProcess` 强制终止；记录运行耗时、退出码、超时事件到 PipelineResult；FR-006、FR-011）
- [x] T019 [P] [US3] 实现并行调度器 `agent3/parallel.h` 和 `agent3/parallel.c`（为每个模块维护独立 Pipeline 结构体：phase 枚举、process_handle、PipelineResult 缓冲区；并发窗口管理：活跃进程数 ≤ CPU 核心数（`GetSystemInfo`/`sysconf` 获取）；使用 `WaitForMultipleObjects` 等待进程组；完成者推进 phase，释放槽位后启动下一待处理模块；FR-013、FR-016）
- [x] T020 [US3] 将编译运行集成到流水线 `agent3/agent3.c`（COMPILING 状态模块 → `parallel_scheduler` 调度 `compile()` → 编译成功 → `run()` → 收集结果到 PipelineResult；编译失败 → 记录错误 → 进入 REPORT 阶段；超时终止后清理进程资源；**不重试**——失败直接记录 FR-017）

**Checkpoint**: Agent 3 可编译并运行测试模块，支持多模块并行，600 秒默认超时可按模块覆盖

---

## Phase 6: User Story 4 — 测试结果分析与文档生成 (Priority: P3)

**Goal**: Agent 3 分析测试运行输出，生成结构化测试报告（`./docs/test/uid_NNN_report.md`）和完成通知（`./docs/interact/test_done_uid_NNN.md`）

**Independent Test**: 给定一份模拟测试运行输出（stdout 包含 PASS/FAIL 行），Agent 3 生成格式正确的报告和通知文档

### Implementation for User Story 4

- [x] T021 [P] [US4] 实现输出解析器 `agent3/reporter.h` 和 `agent3/reporter.c`——解析部分（从 `run_output` 中提取 `PASS:` / `FAIL:` 行；统计 pass_count 和 fail_count；识别 `[TEST]` 行提取测试用例名；提取失败用例的 Input/Expected/Actual/Difference 字段；FR-007）
- [x] T022 [P] [US4] 实现测试报告生成器 `agent3/reporter.c`——报告写入（按 `contracts/test-report-format.md` schema 生成 `./docs/test/uid_NNN_report.md`：Module Info、Test Time、Compile Result（PASS/FAIL/TIMEOUT + 耗时）、Run Result（PASS/FAIL/TIMEOUT + 退出码 + 耗时）、Summary（Total/Passed/Failed/Pass Rate）、Failed Tests 表格、Errors 段落；编译失败时不生成 Run Result 节、Total Tests=0；文件名以 UID 为前缀；FR-007）
- [x] T023 [P] [US4] 实现完成通知生成器 `agent3/interact.c`——通知写入（按 `contracts/completion-notification-format.md` schema 生成 `./docs/interact/test_done_uid_NNN.md`：`## Status`=completed/error、`## Module`=uid_NNN (name)、`## Result`=pass/fail/compile_error/timeout、`## Summary` 含统计和报告路径链接；FR-014）
- [x] T024 [US4] 实现错误报告生成 `agent3/interact.c`——错误通知（发生不可恢复错误时生成 `./docs/interact/error_uid_NNN.md`：`## Status`=error、`## Module`、`## Error Detail`=完整错误描述；FR-012）
- [x] T025 [US4] 将文档生成集成到流水线 `agent3/agent3.c`（RUN 阶段完成 → 解析输出 → 生成测试报告 `./docs/test/uid_NNN_report.md` → 生成完成通知 `./docs/interact/test_done_uid_NNN.md` → 状态推进至 COMMIT；编译失败 → 生成含错误详情的报告和通知；**不重试**——失败直接记录到文档 FR-017）

**Checkpoint**: Agent 3 可生成完整测试报告和双向通知文档，格式符合合约定义

---

## Phase 7: User Story 5 — Git 版本管理 (Priority: P3)

**Goal**: Agent 3 在测试完成后自动执行 Git 提交（add + commit），处理锁竞争和无变更场景

**Independent Test**: 一次完整测试流程结束后，检查 Git 日志确认存在提交记录，消息包含模块 UID 和时间戳

### Implementation for User Story 5

- [x] T026 [P] [US5] 实现 Git 操作模块 `agent3/gitmgr.h` 和 `agent3/gitmgr.c`（封装 `system("git add <files>")` 和 `system("git commit -m \"...\"")` 调用；提交消息格式：`[Agent3] Test uid_NNN (module_name) - YYYY-MM-DD HH:MM`；提交前 `git status --porcelain` 检查是否有变更，无变更跳过并记录 "无变更可提交"；Git 错误（合并冲突等）捕获并记录到日志；FR-008、FR-009）
- [x] T027 [US5] 实现 Git 锁管理 `agent3/gitmgr.c`（全局互斥锁 `git_mutex`：多个并行流水线完成测试后串行化所有 Git 操作；使用 Windows `CreateMutex` 或文件锁 `./.git/.agent3_lock`；确保每次提交原子化，不会因并行提交导致仓库损坏）
- [x] T028 [US5] 将 Git 提交流水线集成到主循环 `agent3/agent3.c`（REPORT 阶段完成 → `AcquireGitLock()` → `git_commit(test_report_path, test_dir)` → `ReleaseGitLock()` → 模块状态归位 IDLE；提交失败时通过 `./docs/interact/` 生成错误通知 FR-012）

**Checkpoint**: 完整测试流水线端到端可运行：检测→创建目录→生成 Makefile→编译→运行→报告→通知→提交

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: 完善监控、文档和健壮性

- [x] T029 [P] 实现 Agent 3 自身日志系统增强 `agent3/utils.c`（每日轮转日志 `./docs/test/.agent3_log_YYYYMMDD.txt`；记录轮询周期事件、模块状态转换、错误详情、吞吐统计、心跳写入确认）
- [x] T030 [P] 验证快速入门流程 `quickstart.md`（按文档步骤编译 `agent3/Makefile` → 启动 `agent3.exe` → 手动创建 `test_request_uid_001.md` → 验证心跳文件 → 观察完整流水线执行 → 检查报告和通知文档 → 确认 Git 提交记录）
- [x] T031 处理 Windows 路径兼容性全局检查（确保所有文件路径操作在 `./test/uid_NNN/Makefile` 中使用 Unix 风格 `/`，Windows API 调用使用 `\`，跨环境正确转换；FR Edge Case: 路径分隔符冲突）
- [x] T032 [P] 实现磁盘空间检查 `agent3/utils.c`（测试前检查 `./test/` 所在磁盘可用空间 ≥ 100MB，不足时拒绝启动新测试并记录警告；FR Edge Case: 磁盘空间不足）
- [x] T033 代码审查与内存泄漏检查（检查所有 `malloc`/`HeapAlloc` 对应的 `free`/`HeapFree`；使用 Dr. Memory 或手动压力测试验证 SC-006 72 小时运行无泄漏；确保心跳线程和轮询循环资源正确释放）

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: 无依赖——立即开始
- **Foundational (Phase 2)**: 依赖 Phase 1 完成——**阻塞所有用户故事**
- **User Story 1 (Phase 3)**: 依赖 Phase 2 完成（需要 module_tracker）
- **User Story 2 (Phase 4)**: 依赖 Phase 3 完成（需要 detector 输出 PENDING 模块）；但 T013–T014 可**与 US1 并行开发**（独立模块，接收模拟输入验证）
- **User Story 3 (Phase 5)**: 依赖 Phase 4 完成（需要 testdir 和 makefile_gen）；但 T017–T019 可**提前并行开发**（独立模块）
- **User Story 4 (Phase 6)**: 依赖 Phase 3+5；但 reporter/interact 模块（T021–T024）可**独立于 US2/US3 开发**——仅依赖合约定义和模拟数据
- **User Story 5 (Phase 7)**: 依赖 Phase 4+6；但 gitmgr（T026–T027）可**提前并行开发**
- **Polish (Phase 8)**: 依赖所有用户故事完成

### User Story Dependencies

- **US1 (P1)**: 可在 Foundational 完成后开始——不依赖其他故事
- **US2 (P2)**: 依赖 US1（需检测结果触发目录创建）
- **US3 (P2)**: 依赖 US2（需测试目录和 Makefile）
- **US4 (P3)**: 依赖 US3（需编译/运行输出）
- **US5 (P3)**: 依赖 US4（需报告文件才能提交）

### Within Each User Story

- 头文件 + 模块实现 → 集成到流水线
- 每个故事以 checkpoint 为里程碑

### Parallel Opportunities

- **Phase 1**: T002、T003 可并行（不同文件）
- **Phase 2**: T006、T007 可并行（不同文件）
- **Phase 3**: T009、T010 可并行（不同文件）
- **Phase 4**: T013、T014 可并行（不同文件）
- **Phase 5**: T017、T018、T019 可并行（三个独立模块，不同文件）
- **Phase 6**: T021、T022、T023 可并行（不同功能但 T022 依赖 T021 解析结果；T023 独立）
- **Phase 7**: T026 独立可并行开发
- **Phase 8**: T029、T030、T032 可并行（不同文件/独立任务）
- **跨 Phase**: US4（reporter）可与 US2/US3 并行开发；US5（gitmgr）可与 US3/US4 并行开发

---

## Parallel Example: User Story 5

```bash
# 可并行启动：
Task: "实现 Git 操作模块 agent3/gitmgr.c (T026)"
Task: "实现 Git 锁管理 agent3/gitmgr.c (T027)"  # 同文件但可先后在同一文件中实现
```

---

## Parallel Example: User Story 3

```bash
# 可并行启动（三个独立模块，不同文件）：
Task: "实现进程管理器 agent3/compiler.c (T017)"
Task: "实现测试运行器 agent3/runner.c (T018)"
Task: "实现并行调度器 agent3/parallel.c (T019)"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. 完成 Phase 1: Setup
2. 完成 Phase 2: Foundational
3. 完成 Phase 3: User Story 1
4. **停止验证**: Agent 3 运行主循环，日志输出检测到的模块，心跳文件正常更新
5. MVP 验证通过后可演示基础检测能力

### Incremental Delivery

1. Setup + Foundational → 基础框架就绪（心跳+模块跟踪+去重）
2. +US1 → 可检测代码变更和交互指令（MVP！）
3. +US2 → 可自动创建测试目录结构和 Makefile
4. +US3 → 可编译并运行测试，支持并行（核心闭环！）
5. +US4 → 可生成报告和双向通知（与其他 Agent 联通！）
6. +US5 → 可自动 Git 版本管理（完整流水线！）
7. +Polish → 生产就绪、通过 72 小时运行验证

### Suggested MVP Scope

**推荐首个交付版本 = Phase 1–7 全部完成**。虽然 US1 单独可验证基础架构，但 US1 的实用价值有限——仅检测不执行测试。US3（编译运行）才是核心闭环，US4（报告通知）实现与其他 Agent 的互联。建议以完整流水线作为首个可用版本。

---

## Notes

- [P] 标记 = 不同文件，无依赖，可并行
- [US?] 标签 = 映射到特定用户故事以追踪
- 每个用户故事应独立可完成和测试
- 每个任务或逻辑组完成后提交 git
- 在任何 checkpoint 停止验证故事独立性
- Agent 3 自身不包含测试框架——正确性通过 `quickstart.md` 端到端验证
- 超时无默认值，Agent 2 MUST 在 `## Test Params` 的 `timeout` 字段显式指定
- 不重试失败测试（FR-017）——Agent 2 修复代码后重新触发
- 心跳文件每 60 秒更新到 `./docs/interact/.agent3_heartbeat`（FR-018）
- 去重窗口 60 秒，手动触发不受限制
