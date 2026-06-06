# Feature Specification: 多路由上采样

**Feature Branch**: `006-upsampling`

**Created**: 2026-06-06

**Input**: 宪章原则 VI — 将输入分别用几个等大的布尔路由器处理，结果按顺序拼接实现上采样。

## User Scenarios

### User Story 1 — 基本多路由上采样 (Priority: P1)

使用者将输入同时送入 M 个等大的布尔路由器，每个路由器独立产生输出，
所有输出按路由器顺序拼接为一个长向量，实现 M 倍上采样。

**Why this priority**: 上采样是 BoolNet 空间扩展的核心机制。

**Independent Test**: 输入 [1,0,1]、2 个路由器（r1=全0恒等，r2=全1翻转），
输出 = [r1输出] + [r2输出] = [1,0,1,0,1,0]（2 倍上采样）。

**Acceptance Scenarios**:

1. **Given** 输入长度 N、M 个路由器，**When** 上采样，**Then** 输出长度 = N × M。
2. **Given** 路由器长度不等于输入长度，**When** 上采样，**Then** 返回错误。
3. **Given** M=1，**When** 上采样，**Then** 退化为单路由处理（1 倍"上采样"）。

---

### User Story 2 — 级联上采样 (Priority: P2)

支持多次上采样级联：第一级输出作为第二级输入。

**Why this priority**: 深层网络需要逐级扩展维度空间。

**Independent Test**: 输入长度 2，第一级 M=2 → 输出 4，第二级 M=2 → 输出 8。

---

### Edge Cases

- M=0：返回错误
- 输入为空（N=0）：返回错误
- 单个路由器全 0：对应输出段 = 输入原样
- 极大 M（如 256 路由器）：并行性能验证

## Requirements

- **FR-001**: MUST 使用 M 个等大布尔路由器处理同一输入
- **FR-002**: MUST 按路由器顺序拼接结果为 M×N 输出
- **FR-003**: MUST 校验所有路由器长度 = 输入长度
- **FR-004**: MUST 支持级联上采样叠加
- **FR-005**: 纯 C + Makefile，每个路由器运算可独立并行

## Success Criteria

- **SC-001**: 10000 位输入 × 4 路由器上采样 < 2ms
- **SC-002**: 输出第 k 段 = 第 k 个路由器对原始输入的独立路由结果（可逐段验证）
- **SC-003**: 级联两级后数据完整性 100%

## Key Entities

- **UpsampleLayer**: M 个 BoolRouter + uid
- **输出**: M × N 位序列，由 M 个 N 位段拼接
