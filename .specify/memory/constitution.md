<!--
Sync Impact Report
==================
Version change: 0.0.1 → 0.1.0 (MINOR bump)
Reason: Material restructuring — formalized meta-principles, separated technical architecture
        into its own section, added Governance section with amendment procedure and versioning policy.

Modified principles:
  - "输出语言原则" → "I. 双语文档与沟通 (Bilingual Documentation & Communication)"
  - "agent分工原则" → "II. 多Agent协作分工 (Multi-Agent Collaboration)"
  - "历史读取原则" → "III. 历史上下文参考 (Historical Context Reference)"

Added sections:
  - "Technical Architecture" — extracted 7 framework requirements as formal principles (IV–X)
  - "Development Workflow" — agent role definitions and file structure conventions
  - "Governance" — amendment procedure, versioning policy, compliance review

Removed sections: None (all original content preserved and restructured)

Templates requiring updates:
  - .specify/templates/plan-template.md — ✅ No changes needed (Constitution Check gate is generic)
  - .specify/templates/spec-template.md — ✅ No changes needed (requirements format compatible)
  - .specify/templates/tasks-template.md — ✅ No changes needed (task structure compatible)
  - .specify/templates/checklist-template.md — ✅ No changes needed

Follow-up TODOs: None
-->

# BoolNet Constitution

BoolNet 是一个基于布尔路由器与 Tsetlin 自动机的神经网络框架，实现完全布尔运算的神经网络。
使用纯 C 语言 + Makefile 编译，在 Windows 平台开发运行。

## Core Principles

### I. 双语文档与沟通 (Bilingual Documentation & Communication)

- 内部思考文档、Agent 间交互文档 MUST 可以使用中文或英文。
- 与最终使用者对接的文档 MUST 提供中文版本进行交互。
- 代码注释 SHOULD 使用英文以保证可移植性，但中文注释在内部模块中允许。

**Rationale**: 项目主要面向中文开发者社区，同时保持与国际开源生态的兼容性。

### II. 多Agent协作分工 (Multi-Agent Collaboration)

项目采用三 Agent 分工架构，每个 Agent 职责明确且 MUST 在其指定目录下工作：

**Agent 1 — 主控与设计 Agent (Master/Design)**
- 负责与用户直接沟通，整理设计方案。
- 生成指导性文档放置于 `./docs/interact/` 文件夹。
- 每个技术文档 MUST 标注目标 Agent。
- MUST 自动生成子 Agent 对文档进行审查。
- 读取其他 Agent 生成的文档向用户反馈。

**Agent 2 — 代码生成 Agent (Code Generation)**
- 主要在 `./src/` 文件夹下工作，负责代码生成。
- 在 `./docs/api/` 文件夹下生成代码 API 文档。
- MUST 自动生成子 Agent 做代码一次审查，审查结果放在 `./checkout/` 文件夹。
- 审查结果 MUST 标注审查者（哪个 Agent 或子 Agent）并清晰描述审查问题。
- MUST 自动读取 `./checkout/` 和 `./docs/test/` 中的审查结果及测试结果文档做出修改。
- 需要人工审核的部分 MUST 在 `./docs/interact/` 文件夹下上报。
- 完成部分代码后 MUST 在 `./docs/interact/` 生成文件告知测试 Agent。

**Agent 3 — 测试 Agent (Testing)**
- 主要完成代码的模块测试工作。
- MUST 在 `./test/` 文件夹下自动建立对应的测试目录结构。
- MUST 自动编译运行并调试运行结果。
- 测试结果 MUST 放在 `./docs/test/` 文件夹中。
- 检测到新代码文件完成后 MUST 自动读取 `./docs/interact/` 下文件并开始测试。
- 测试 MUST 自动生成 Makefile 文件进行编译。
- MUST 根据输出结果编写测试文档。
- 完成后 MUST 自动进行 Git 版本管理操作以便回滚。

**Rationale**: 明确的分工和目录约定确保多 Agent 协作不会产生文件冲突，每个 Agent 的输出都有明确的消费方。

### III. 历史上下文参考 (Historical Context Reference)

- `./docs/history/` 文件夹下的设计思路与 Agent 历史对话 MUST 作为设计参考。
- 新设计决策 SHOULD 查阅历史对话以避免重复讨论已解决的问题。
- 与历史结论冲突的新方案 MUST 明确说明变更理由。

**Rationale**: 保留设计决策的上下文信息，避免在迭代中丢失已确认的设计结论。

## Technical Architecture

以下为 BoolNet 框架 MUST 实现的核心技术组件：

### IV. 布尔路由器 (Boolean Router)

输入与路由器参数 bit 数量相等。若路由器 bit 为 1 则输入翻转，反之输入不变。
这是框架最基础的运算单元。

### V. 一维回环卷积 (1D Circular Convolution)

卷积核到下边界溢出后，多余的溢出部分与输入最前方运算，保证每一个数据处理次数相同。
因为是回环卷积，padding 是无效参数，但 MUST 保留步长 (stride) 与空洞卷积的间隔参数 (dilation)。

### VI. 多路由上采样 (Multi-Router Upsampling)

将输入分别用几个等大的布尔路由器进行处理，处理结果按顺序拼接，实现上采样功能。

### VII. 层 UID 与数据持久化 (Layer UID & Persistence)

每个实现层 MUST 分配唯一 UID 编号。MUST 完成数据参数的保存与读取功能。

### VIII. Memory 层设计 (Memory Layer)

一维定长整数 Memory，MUST 支持 uint8 / uint16 / uint32 / uint64 多个版本。
需要指定上限值：通过布尔路由器导向后记忆被触发，触发的记忆被还原到设定的上限值。
记忆按每个 forward 进行自减，自减值手动设定，自减前判断减去值与自身大小，保证不会小于 0（uint 不溢出）。
只要不为 0 即为记忆触发状态。

### IX. Forward 函数 (Forward Function)

MUST 完成整个网络的向前运算函数设计。

### X. Tsetlin 自动机训练 (Tsetlin Automaton Training)

网络 MUST 完成单个 train 的设计，使用 Tsetlin 自动机按 UID 递增顺序每一层进行权重翻转。
MUST 可以指定可接受的负奖励次数。每一次训练每一层仅触发一个翻转，每次翻转进行一次 forward 传播。
用 byte 上限减去输出 byte 与目标 byte 的差值作为奖励值。

## Development Workflow

### 目录结构约定

```text
./src/              — 源代码 (Agent 2)
./test/             — 测试代码 (Agent 3)
./docs/interact/    — Agent 间交互文档 (Agent 1 输出, Agent 2/3 读取)
./docs/api/         — API 文档 (Agent 2 输出)
./docs/test/        — 测试结果文档 (Agent 3 输出)
./docs/history/     — 历史对话存档 (所有 Agent 参考)
./checkout/         — 代码审查结果 (Agent 2 子 Agent 输出)
```

### 开发顺序

1. Agent 1 完成设计文档 → 放置于 `./docs/interact/`
2. Agent 2 读取设计文档 → 生成代码于 `./src/` → 子 Agent 审查 → 审查结果放 `./checkout/`
3. Agent 2 读取审查与测试反馈 → 修改代码 → 需要时在 `./docs/interact/` 上报
4. Agent 3 检测新代码 → 自动测试 → 结果放 `./docs/test/` → 自动 Git 版本管理
5. Agent 1 汇总各 Agent 输出 → 向用户反馈

## Governance

### 修订流程 (Amendment Procedure)

1. 任何人对宪章原则的修改 MUST 以文档形式提出，说明修改内容与理由。
2. 影响核心原则的修改 MUST 经过 Agent 1 审查确认。
3. 所有修订 MUST 记录在 Sync Impact Report 中（本文件顶部 HTML 注释）。

### 版本策略 (Versioning Policy)

版本号遵循 Semantic Versioning：

- **MAJOR**: 向后不兼容的原则删除或重新定义。
- **MINOR**: 新增原则/章节，或实质性扩展现有指导。
- **PATCH**: 澄清措辞、修复笔误、非语义性优化。

### 合规审查 (Compliance Review)

- 每次 Spec/Plan/Tasks 生成时 MUST 检查与宪章原则的一致性。
- Agent 1 在汇总反馈时 SHOULD 确认当前工作流符合本宪章要求。
- 复杂度偏离 MUST 在 Plan 的 Complexity Tracking 中记录并说明理由。

**Version**: 0.1.0 | **Ratified**: 2026-06-06 | **Last Amended**: 2026-06-06
