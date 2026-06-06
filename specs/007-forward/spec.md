# Feature Specification: Forward 函数

**Feature Branch**: `007-forward`

**Created**: 2026-06-06

**Input**: 宪章原则 IX — 完成整个网络的向前运算函数设计。

## User Scenarios

### User Story 1 — 完整网络 Forward (Priority: P1)

使用者定义层序列（布尔路由器 → 回环卷积 → 上采样 → Memory 层），
调用统一 forward 函数按顺序执行每层的 forward，获得最终输出。

**Why this priority**: Forward 是整个 BoolNet 的推理入口，串联所有其他组件。

**Independent Test**: 搭建一个最小网络（1 个路由器 + 1 个 Memory 层），
给定输入和路由器参数，forward 后验证 Memory 层触发状态符合预期。

**Acceptance Scenarios**:

1. **Given** 一个已配置的层序列 `[Router, Conv1D, Upsample, Memory]`，**When** forward 输入数据，**Then** 数据依次流经所有层，每层输出作为下一层输入。
2. **Given** 层序列为空，**When** forward，**Then** 输入=输出（直通）。
3. **Given** 中间层返回错误（如维度不匹配），**When** forward，**Then** 停止传播并返回错误。

---

### User Story 2 — 层拓扑管理 (Priority: P1)

使用者按 UID 递增顺序注册层到网络中，forward 按注册顺序执行。

**Why this priority**: 层顺序决定数据流图结构。

**Independent Test**: 注册 3 个层（UID 1,2,3），forward 验证执行顺序 = [1,2,3]。

---

### User Story 3 — 网络状态持久化 (Priority: P2)

保存/加载完整网络：所有层的 UID、参数、Memory 层状态。

**Why this priority**: 推理可在任意检查点暂停/恢复。

---

## Requirements

- **FR-001**: MUST 支持按 UID 顺序注册层（任意类型：Router/Conv/Upsample/Memory）
- **FR-002**: MUST forward 按注册顺序串行执行，每层输出 → 下一层输入
- **FR-003**: MUST 中间层错误时停止传播
- **FR-004**: MUST 支持空网络（直通）
- **FR-005**: MUST 支持保存/加载完整网络状态
- **FR-006**: 纯 C + Makefile

## Success Criteria

- **SC-001**: 4 层网络 forward < 5ms（10000 位输入）
- **SC-002**: 空网络 forward 开销 < 1μs（直通零开销）
- **SC-003**: 加载后网络产生与保存前相同输出

## Key Entities

- **BoolNet**: 层列表 + 网络配置 + forward/save/load
- **Layer**: 联合类型（Router | Conv | Upsample | Memory）+ UID + forward 函数指针
