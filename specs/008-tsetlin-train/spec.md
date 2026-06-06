# Feature Specification: Tsetlin 自动机训练

**Feature Branch**: `008-tsetlin-train`

**Created**: 2026-06-06

**Input**: 宪章原则 X — 使用 Tsetlin 自动机按 UID 递增顺序每层进行权重翻转训练。可指定负奖励次数。每次训练每层仅触发一个翻转，每次翻转进行一次 forward。用 byte 上限减去输出与目标的差值作为奖励。

## User Scenarios

### User Story 1 — 单层 Tsetlin 训练步 (Priority: P1)

使用者对网络执行单个训练步：按 UID 递增顺序，每层随机选择一个路由器 bit 翻转，
执行一次 forward 传播，比较输出与目标。用 `byte_max - |output - target|` 作为奖励值。
奖励 > 0 表示改进，保留翻转；否则回退并计入负奖励。

**Why this priority**: Tsetlin 训练是 BoolNet 的学习核心，没有训练就没有智能。

**Independent Test**: 单层路由器网络，随机初始参数，训练 100 步后输出逼近目标。
验证奖励函数正确计算。

**Acceptance Scenarios**:

1. **Given** 单层路由器、随机初始 bit、给定目标输出，**When** 执行一次训练步，**Then** 恰好 1 个 bit 翻转，计算并记录奖励值。
2. **Given** 翻转后奖励 > 0，**When** 步完成，**Then** 翻转被保留。
3. **Given** 翻转后奖励 ≤ 0，**When** 给定负奖励容忍次数，**Then** 翻转被回退并计入负奖励。累计负奖励超过容忍值 → 停止训练。

---

### User Story 2 — 多轮训练收敛 (Priority: P2)

使用者指定最大训练轮数和收敛条件，训练持续到收敛或达到最大轮数。

**Why this priority**: 收敛判断是实际训练的必要条件。

**Independent Test**: 简单 XOR 问题，2 个路由器网络，1000 轮内收敛至 95%+ 准确率。

---

### Edge Cases

- 负奖励次数容忍值 = 0：任何非正奖励立即回退
- 负奖励次数容忍值 = N：连续 N 次非正奖励后停止
- 训练中网络状态可随时保存/恢复
- 全 0 输出与目标比较的边界

## Requirements

- **FR-001**: MUST 按 UID 递增顺序逐层训练，每层每步仅翻转 1 个 bit
- **FR-002**: MUST 每次翻转后执行完整 forward 传播
- **FR-003**: MUST 计算奖励 = byte_max - |output_byte - target_byte|
- **FR-004**: MUST 支持可配置的负奖励容忍次数
- **FR-005**: MUST 奖励 > 0 时保留翻转，≤ 0 时回退翻转
- **FR-006**: MUST 连续负奖励超容忍值后停止训练
- **FR-007**: 纯 C + Makefile

## Success Criteria

- **SC-001**: 单步训练时间（1 翻转 + forward + 奖励）< 10ms（1000 位网络）
- **SC-002**: 简单 XOR 任务 1000 轮内收敛至 95% 准确率
- **SC-003**: 每步翻转 bit 数精确 = 1（可统计验证）

## Key Entities

- **TsetlinTrainer**: 网络引用 + 负奖励容忍值 + 负奖励计数器 + 训练统计
- **TrainingStep**: 层 UID + 翻转位置 + 翻转前后奖励值
