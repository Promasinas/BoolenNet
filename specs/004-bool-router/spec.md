# Feature Specification: 布尔路由器 (Boolean Router)

**Feature Branch**: `004-bool-router`

**Created**: 2026-06-06

**Status**: Draft

**Input**: 宪章原则 IV — 布尔路由器：输入与路由器参数 bit 数量相等，bit 为 1 则输入翻转，反之不变。这是框架最基础的运算单元。

## User Scenarios & Testing *(mandatory)*

### User Story 1 — 单路由比特翻转 (Priority: P1)

框架使用者需要一个布尔路由器：输入任意长度的位序列，路由参数与输入等长，
路由 bit=1 的位置对应输入位翻转（XOR 1），bit=0 位置保持不变。

**Why this priority**: 布尔路由器是 BoolNet 框架的最基础运算单元，所有后续组件
（Memory 层触发、上采样、Forward 传播）都依赖它。

**Independent Test**: 给定输入 `[1,0,1,0]` 和路由参数 `[0,1,0,1]`，
输出 `[1,1,1,1]`（第 1、3 位翻转，第 0、2 位不变）。

**Acceptance Scenarios**:

1. **Given** 输入 `[1,1,0,0]`、路由 `[0,0,0,0]`（全不触发），**When** 执行路由，**Then** 输出 `[1,1,0,0]`（完全不翻转）。
2. **Given** 输入 `[1,1,0,0]`、路由 `[1,1,1,1]`（全触发），**When** 执行路由，**Then** 输出 `[0,0,1,1]`（完全翻转）。
3. **Given** 输入 `[0,0]`、路由 `[1,0]`，**When** 执行路由，**Then** 输出 `[1,0]`。
4. **Given** 输入和路由长度不匹配，**When** 执行路由，**Then** 返回错误。

---

### User Story 2 — 批量多路由器并行处理 (Priority: P2)

使用者可同时使用多个等大的布尔路由器对同一输入并行处理，
用于上采样（原则 VI）和 Multi-head 路由场景。

**Why this priority**: 多路由并行是上采样的基础，但依赖于单路由先实现。

**Independent Test**: 输入 `[1,0]`，同时用 2 个路由器 `[[0,0],[1,1]]` 处理，
输出 `[[1,0],[0,1]]`。

**Acceptance Scenarios**:

1. **Given** 输入长度 4、3 个路由器，**When** 批量路由，**Then** 输出 3 个等长结果。
2. **Given** 路由器数组为空（数量=0），**When** 批量路由，**Then** 返回空结果集。

---

### User Story 3 — 路由器参数持久化 (Priority: P2)

每个路由器实例有唯一 UID，支持保存参数位序列和从存储恢复（原则 VII）。

**Why this priority**: 持久化是实现训练中断/恢复的基础，非 MVP 阻塞项。

**Independent Test**: 创建路由器，保存参数，加载到新实例，验证两个实例对相同输入产生相同输出。

---

### Edge Cases

- 输入长度 0：返回错误（空路由无意义）
- 极大长度（10000+ 位）：验证性能在合理范围
- 所有路由器 bit 全 0：输入原样输出（恒等路由）
- 所有路由器 bit 全 1：输入完全翻转（NOT 路由）

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 系统 MUST 接收长度为 N 的输入位序列和等长路由参数，输出 N 位序列：`output[i] = input[i] XOR router_bit[i]`
- **FR-002**: 系统 MUST 校验输入与路由参数长度一致，不一致时返回错误
- **FR-003**: 系统 MUST 支持输入长度为 0 时返回错误
- **FR-004**: 系统 MUST 为每个路由器实例分配唯一 UID
- **FR-005**: 系统 MUST 支持保存路由器参数到二进制文件和从文件加载恢复
- **FR-006**: 系统 MUST 支持批量多路由器并行处理：M 个路由器 × 同一输入 → M 个输出
- **FR-007**: 系统 MUST 使用纯 C 语言 + Makefile 编译，在 Windows 平台上可编译为独立 .o

### Key Entities

- **BoolRouter**: 包含 uid、length（位数）、bits（路由参数位数组，uint8_t 数组）
- **RouterOutput**: 路由结果，与输入等长的位序列

## Success Criteria *(mandatory)*

- **SC-001**: 10000 位输入单次路由运算 < 0.5ms
- **SC-002**: M 个路由器的批量处理时间 < M × 单路由时间的 1.1 倍（无额外开销）
- **SC-003**: 保存→加载往返后路由器参数逐位完全一致
- **SC-004**: 100% 长度不匹配输入被拒绝

## Assumptions

- 输入和路由参数的位序列使用 uint8_t 数组存储（每字节 1 位或打包 8 位）
- 路由器参数在创建时确定，运行时不可变（训练时通过 Tsetlin 翻转更新）
- 位打包格式：每字节 8 位，bit 0 = LSB，与 Memory 层 forward 的 signal 格式一致
