# Feature Specification: Agent 3 — 自动化测试 (Automated Testing)

**Feature Branch**: `003-agent3-testing`

**Created**: 2026-06-06

**Status**: Draft

**Input**: User description: "这是agent3"

## Clarifications

### Session 2026-06-06

- Q: Agent 2 应该用什么具体格式向 Agent 3 发送测试请求？ → A: 约定式 Markdown 标题——固定标题层级（如 `## Target Module`、`## Test Priority`），靠标题命名约定解析。
- Q: 多个模块需要同时测试时，Agent 3 应该串行还是并行处理？ → A: 完全并行——每个模块拥有独立的端到端流水线（检测→创建目录→编译→运行→文档→提交），各模块并发执行，互不等待。
- Q: 测试完成后 Agent 3 如何通知其他 Agent？ → A: 双向通知——Agent 3 在 `./docs/interact/` 放置完成通知文档（含状态摘要），同时在 `./docs/test/` 写入详细报告。Agent 2 轮询 `./docs/interact/` 获取完成状态，Agent 1 查阅 `./docs/test/` 获取完整结果。
- Q: Agent 3 应以什么作为模块的唯一标识？ → A: Layer UID 为主标识——测试目录以 UID 命名（如 `./test/uid_001/`），测试文档按 UID 索引。模块名作为元数据辅助记录。
- Q: 并行编译的默认最大并发进程数是多少？ → A: CPU 核心数——自动检测系统 CPU 核心数，设为编译并发上限，确保不超过系统承载能力。
- Q: Agent 3 对同一模块的多次变更检测应使用多长的去重时间窗口？ → A: 60 秒——与轮询周期对齐，每轮轮询中同一模块只触发一次测试，超出窗口的新变更触发新一轮。
- Q: 测试运行失败时，Agent 3 是否应该自动重试？ → A: 不重试——测试失败直接记录并报告，不自动重试。由 Agent 2 修复代码后重新触发测试流程。
- Q: Agent 3 是否需要定期输出心跳/健康状态信号？ → A: 心跳文件——Agent 3 每分钟更新 `./docs/interact/.agent3_heartbeat` 的时间戳，其他 Agent 通过检查该文件判断 Agent 3 是否存活（超过 180 秒未更新即判定异常）。
- Q: 测试执行超时的默认值应为多少？ → A: 无默认值——超时上限 MUST 由 Agent 2 在测试指令的 `## Test Params` 中通过 `timeout` 字段显式指定（单位：秒）。Agent 3 MUST 拒绝缺少 `timeout` 参数的测试指令并记录解析错误。

## User Scenarios & Testing *(mandatory)*

### User Story 1 — 自动检测代码变更并触发测试 (Priority: P1)

Agent 3 作为测试 Agent，需要自动监测 `./src/` 目录下的代码变更以及 `./docs/interact/` 目录中的交互文档，并在检测到新代码或测试指令后自动启动测试流程。

**Why this priority**: 这是 Agent 3 的入口功能。没有自动检测能力，Agent 3 无法获知何时需要执行测试，整个自动化测试流水线无法启动。

**Independent Test**: 在 `./src/` 目录中放置一个新的 .c 源文件或在 `./docs/interact/` 中放入测试指令文档后，Agent 3 能在合理时间内（不超过 1 分钟轮询周期）自动识别变更并触发后续测试流程。

**Acceptance Scenarios**:

1. **Given** `./src/` 目录中新增了一个 .c 源文件，**When** Agent 3 执行检测逻辑，**Then** Agent 3 识别到新文件并记录文件路径和变更时间戳。
2. **Given** `./docs/interact/` 目录中出现了来自 Agent 2 的测试通知文档，**When** Agent 3 读取该文档，**Then** Agent 3 解析出需要测试的模块列表并启动对应测试流程。
3. **Given** `./src/` 目录中没有新文件且 `./docs/interact/` 中没有新的测试指令，**When** Agent 3 执行检测逻辑，**Then** Agent 3 进入等待状态而不执行任何测试操作。

---

### User Story 2 — 自动建立测试目录结构 (Priority: P2)

Agent 3 在识别到需要测试的代码模块后，MUST 在 `./test/` 目录下自动创建与源代码模块对应的测试目录结构。

**Why this priority**: 测试目录结构是测试代码和测试结果的组织基础，必须在编译和运行测试之前完成。没有规范的目录结构，后续的测试文件、Makefile 和文档将无法正确存放。

**Independent Test**: 当 Agent 3 检测到 `./src/` 中的新模块后，在 `./test/` 下以 Layer UID 自动创建对应的测试子目录（如 `./test/uid_001/`），目录结构包含测试代码文件占位和 Makefile。

**Acceptance Scenarios**:

1. **Given** `./src/` 中有一个新模块（UID=1，模块名=bool_router），**When** Agent 3 为该模块创建测试目录，**Then** `./test/uid_001/` 目录被创建，且包含初始的测试文件框架和 Makefile。
2. **Given** `./test/` 根目录尚不存在，**When** Agent 3 首次需要创建测试目录，**Then** Agent 3 先创建 `./test/` 根目录再创建子目录。
3. **Given** 某个模块的测试目录已存在，**When** Agent 3 再次检测到同一模块的测试请求，**Then** Agent 3 复用已有目录结构，仅更新变化的测试文件。

---

### User Story 3 — 自动编译与运行测试 (Priority: P2)

Agent 3 MUST 为每个测试模块自动生成或更新 Makefile，调用 Makefile 编译测试代码，运行编译产物，并捕获运行输出结果。

**Why this priority**: 编译和运行是测试的核心环节。没有自动编译和运行能力，Agent 3 无法验证代码正确性，测试流程无法闭环。

**Independent Test**: 对一个已创建测试目录的模块，Agent 3 能自动生成 Makefile、执行编译、运行测试二进制文件，并捕获标准输出和标准错误。

**Acceptance Scenarios**:

1. **Given** 测试目录 `./test/uid_001/` 已创建且包含测试源代码，**When** Agent 3 执行编译流程，**Then** 生成正确的 Makefile，使用 `make` 命令成功编译出测试可执行文件。
2. **Given** 测试可执行文件已编译完成，**When** Agent 3 执行运行流程，**Then** 测试可执行文件被运行，标准输出和标准错误被完整捕获。
3. **Given** 编译过程出现错误（如语法错误），**When** Agent 3 检测到编译失败，**Then** Agent 3 记录完整的编译错误信息到测试文档中，并标记测试状态为"编译失败"。

---

### User Story 4 — 测试结果分析与文档生成 (Priority: P3)

Agent 3 MUST 分析测试运行结果，编写结构化的测试文档，并将文档输出到 `./docs/test/` 目录。文档应包含测试模块名称、测试时间、测试结果摘要、通过/失败详情和错误信息。

**Why this priority**: 测试文档是 Agent 3 向其他 Agent（特别是 Agent 2 代码生成和 Agent 1 主控）反馈测试结果的主要渠道。虽然它依赖编译和运行完成，但其自身的实现可以独立验证（直接读取已有运行输出来生成文档）。

**Independent Test**: 给定一份模拟的测试运行输出，Agent 3 能够生成一份包含模块名、时间戳、通过/失败统计和详细错误日志的结构化测试文档。

**Acceptance Scenarios**:

1. **Given** 测试运行已完成且产生了输出结果，**When** Agent 3 分析运行输出，**Then** 生成测试文档包含：测试模块名称、测试执行时间、通过用例数/失败用例数、失败用例的详细错误信息。
2. **Given** 所有测试用例均通过，**When** Agent 3 写入测试文档，**Then** 文档明确标记为"全部通过"且不包含错误详情段落。
3. **Given** 存在测试失败，**When** Agent 3 写入测试文档，**Then** 文档中清晰列出每个失败用例的输入、期望输出、实际输出和差异描述。

---

### User Story 5 — Git 版本管理 (Priority: P3)

Agent 3 在完成测试后 MUST 自动执行 Git 提交操作，将测试结果文档和测试目录变更纳入版本管理，以便在需要时进行回滚。

**Why this priority**: Git 版本管理确保测试结果和测试代码的可追溯性。这是测试流程的最后一步，不影响核心测试功能，但对项目治理和回滚能力至关重要。

**Independent Test**: 在一次完整的测试流程完成后，检查 Git 日志确认 Agent 3 执行了提交操作，提交消息包含测试模块信息和时间戳。

**Acceptance Scenarios**:

1. **Given** 测试文档已写入 `./docs/test/`，**When** Agent 3 完成测试流程，**Then** 自动执行 `git add` 和 `git commit`，提交消息包含模块名称和测试日期。
2. **Given** 测试流程中无任何文件变更（所有测试结果与之前一致），**When** Agent 3 尝试提交，**Then** Agent 3 检测到无变更，跳过提交并记录"无变更可提交"。
3. **Given** Git 提交过程中发生错误（如合并冲突），**When** Agent 3 检测到错误，**Then** Agent 3 记录错误信息到日志并通知 Agent 1（通过 `./docs/interact/` 目录）。

---

### Edge Cases

- 当 `./src/` 目录为空（项目初始化阶段）时，Agent 3 不应报错，而是等待直到有源代码文件出现。
- 当同一个模块在 60 秒去重窗口内被多次修改，Agent 3 MUST 合并为一次测试请求，避免重复编译和测试竞态；超出窗口的新变更触发新一轮测试。
- 当编译产生的可执行文件过大或运行时间超过 Agent 2 指定的 timeout 超时上限时，Agent 3 应终止进程并记录超时信息。
- 当 `./docs/interact/` 中的交互文档格式不完整或无法解析时，Agent 3 应记录解析错误而不是静默失败。
- 当测试过程中磁盘空间不足时，Agent 3 应检测写入失败并及时报错。
- 当 Windows 平台路径分隔符与 Unix 风格 Makefile 冲突时，Agent 3 需正确处理路径转换（项目运行在 Windows 平台）。
- 当多个模块并行编译时，每个模块的 Makefile 在独立子目录中运行，互不干扰；并发编译进程数由自动检测的 CPU 核心数限制，防止系统过载。
- 当多个模块并行完成测试后同时执行 Git 提交时，Agent 3 需处理 Git 锁竞争，确保每次提交原子化且不丢失结果。
- 当同一模块被多次手动触发测试（非代码变更触发）时，Agent 3 应每次独立执行完整测试流程，不适用去重窗口。
- 当 Agent 1/Agent 2 检测到心跳文件超过 3 分钟未更新时，应认为 Agent 3 可能已停止运行或卡死。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Agent 3 MUST 周期性检测 `./src/` 目录下的 .c/.h 文件新增与变更（轮询周期不超过 1 分钟）。
- **FR-002**: Agent 3 MUST 读取并解析 `./docs/interact/` 目录中由 Agent 2 生成的测试指令文档。文档 MUST 使用约定式 Markdown 标题（固定标题层级：`## Target UID` 为模块 Layer UID、`## Target Module` 为模块名称、`## Test Priority`、`## Test Params`），Agent 3 按标题命名约定提取字段，以 UID 为主键定位模块。
- **FR-003**: Agent 3 MUST 在 `./test/` 目录下自动创建测试子目录，目录以模块的 Layer UID 命名（如 `./test/uid_001/`），模块名作为元数据记录。每个测试子目录 MUST 包含对应模块的测试代码框架和 Makefile。
- **FR-004**: Agent 3 MUST 为每个测试模块自动生成 Makefile，Makefile 需包含编译目标 (`all`) 和清理目标 (`clean`)。
- **FR-005**: Agent 3 MUST 自动调用 Makefile 编译测试代码并捕获编译输出（含错误信息）。
- **FR-006**: Agent 3 MUST 在编译成功后自动运行测试可执行文件并捕获运行输出。
- **FR-007**: Agent 3 MUST 将测试结果以结构化文档形式写入 `./docs/test/` 目录，文档包含：Layer UID、模块名称（辅助）、时间戳、通过/失败统计、错误详情。文件名以 UID 为前缀（如 `uid_001_report.md`）。
- **FR-008**: Agent 3 MUST 在测试完成后自动执行 Git 版本管理操作（add + commit），提交消息需标注模块的 Layer UID、模块名称和测试日期。
- **FR-009**: Agent 3 MUST 在检测到无文件变更时跳过 Git 提交并记录日志。
- **FR-010**: Agent 3 MUST 处理编译失败场景：记录完整错误信息，标记测试状态，并反馈到测试文档。
- **FR-011**: Agent 3 MUST 处理运行时超时场景：测试执行超时上限由 Agent 2 在 `## Test Params` 的 `timeout` 字段中显式指定（单位：秒），Agent 3 MUST 拒绝缺少此参数的指令。超时后 Agent 3 MUST 终止测试进程并记录超时事件。
- **FR-015**: Agent 3 MUST 校验测试指令的完整性——缺少必填字段（`## Target UID`、`## Target Module`、`## Test Priority`、`## Test Params` 含 `timeout`）的指令 MUST 被拒绝并在日志中记录解析错误。
- **FR-012**: Agent 3 MUST 在发生不可恢复错误时通过 `./docs/interact/` 目录向 Agent 1 报告异常。报告文档 MUST 使用约定式 Markdown 标题格式（`## Status` 标记为 `error`、`## Module` 标注出错模块、`## Error Detail` 记录错误详情）。
- **FR-014**: Agent 3 MUST 在每个模块测试流程完成后，在 `./docs/interact/` 中放置完成通知文档，采用约定式标题格式（`## Status` 标记为 `completed`、`## Module` 标注模块名、`## Result` 标记 `pass`/`fail`、`## Summary` 包含通过/失败统计摘要）。
- **FR-016**: Agent 3 MUST 自动检测系统 CPU 核心数，将并行编译的并发进程数限制在 CPU 核心数以内，防止系统过载。
- **FR-017**: Agent 3 MUST 在测试用例运行失败时不执行自动重试——失败结果直接记录到测试文档并通知 Agent 2。测试仅在 Agent 2 修复代码并重新触发后才再次运行。
- **FR-018**: Agent 3 MUST 每分钟更新 `./docs/interact/.agent3_heartbeat` 文件（写入当前时间戳），供 Agent 1 和 Agent 2 检测 Agent 3 的运行状态。
- **FR-013**: Agent 3 MUST 支持完全并行测试流水线——每个模块拥有独立的测试目录、独立的 Makefile 编译进程、独立的文档输出，各模块端到端流水线并发执行，互不阻塞。

### Key Entities

- **测试模块 (Test Module)**: 对应 `./src/` 中的一个源代码模块。以 **Layer UID** 为主键唯一标识。包含：UID、模块名称（辅助标识）、源文件路径、测试目录路径（`./test/uid_NNN/`）、最近变更时间戳、测试状态（待测试/编译中/运行中/已完成/失败）。
- **测试文档 (Test Report)**: 存放于 `./docs/test/` 的结构化文档。包含模块名称、测试时间、编译状态、运行结果摘要、通过/失败用例详情、错误日志。
- **交互指令 (Interaction Directive)**: 存放于 `./docs/interact/` 的 Markdown 文档，采用约定式标题格式。分为两类：(1) **测试请求**（Agent 2 → Agent 3）：必填标题包括 `## Target UID`（模块 Layer UID）、`## Target Module`（模块名称）、`## Test Priority`（`high`/`medium`/`low`）、`## Test Params`（必含 `timeout` 字段，单位秒）；(2) **完成通知**（Agent 3 → Agent 1/Agent 2）：固定标题包括 `## Status`（`completed`/`error`）、`## Module`、`## Result`（`pass`/`fail`）、`## Summary`。
- **Git 提交记录 (Git Commit)**: 每次测试完成后的版本管理记录。包含提交消息、涉及文件列表、时间戳。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Agent 3 在检测到新源代码文件后 2 分钟内启动测试流程。
- **SC-002**: Agent 3 对单个模块的完整测试流程（检测→创建目录→编译→运行→文档→提交）在 5 分钟内完成（不含测试代码自身的运行时间）。
- **SC-003**: 100% 的测试运行结果有对应的结构化测试文档归档在 `./docs/test/` 目录中。
- **SC-004**: Agent 3 生成的 Makefile 首次编译成功率达到 95% 以上（在测试代码语法正确的前提下）。
- **SC-005**: 测试流程中出现的异常（编译失败、超时、Git 错误）100% 被记录并反馈。
- **SC-006**: Agent 3 在无人干预的情况下可持续轮询运行，72 小时内无内存泄漏或进程僵死。
- **SC-007**: 当 3 个模块同时需要测试时，Agent 3 能并发启动 3 条独立流水线，总完成时间不超过单模块串行执行时间的 1.5 倍（即并行效率不低于 67%）。

## Assumptions

- Agent 2 会在 `./docs/interact/` 中生成测试指令文档，采用约定式 Markdown 标题格式（`## Target Module`、`## Test Priority`、`## Test Params`），Agent 3 按标题命名约定解析。
- 项目编译环境（GCC/MinGW、Make）已在 Windows 平台正确安装并配置在 PATH 中。
- `./src/` 目录下的代码遵循项目既定的代码结构和命名规范（纯 C + Makefile）。
- Agent 3 的轮询周期（1 分钟）在项目初期是合理的，后续可根据需要调整为事件驱动机制。
- Git 已正确初始化（由 speckit-git-initialize 完成）且 Agent 3 有权限执行 commit 操作。
- 测试框架和断言库的选择不属于本规格范围——Agent 3 负责测试流程自动化，不负责定义具体的测试断言 API。
