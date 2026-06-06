# Feature Specification: Memory 层模块

**Feature Branch**: `001-agent1-design`

**Created**: 2026-06-06

**Status**: Draft

**Input**: User description: "Memory 层设计 — 一维定长整数记忆模块，支持多精度、布尔路由触发、自减机制与持久化"

## User Scenarios & Testing *(mandatory)*

### User Story 1 — 创建并初始化 Memory 层 (Priority: P1)

框架使用者需要一个可配置的 Memory 层：指定记忆长度、数据类型精度（uint8/16/32/64）、上限值（触发后恢复到的值）以及每次 forward 的自减值。

**Why this priority**: Memory 层是 BoolNet 框架的核心有状态组件，创建与初始化是所有后续操作的前提。

**Independent Test**: 创建一个 Memory 层实例，验证其长度、精度、上限值和自减值是否正确配置，且所有记忆单元的初始值可读取。

**Acceptance Scenarios**:

1. **Given** 使用者指定长度 10、uint8 精度、上限值 255、自减值 1，**When** 创建 Memory 层，**Then** 返回有效的 Memory 层实例，包含 10 个初始值 0 的记忆单元。
2. **Given** 使用者指定 uint16 精度，**When** 创建 Memory 层，**Then** 每个记忆单元使用 16 位无符号整数存储。
3. **Given** 使用者指定长度为 0，**When** 创建 Memory 层，**Then** 返回明确的错误指示，不允许创建空 Memory 层。

---

### User Story 2 — Forward 传播与触发恢复 (Priority: P1)

在每次 forward 运算中，Memory 层接收布尔路由器的导向信号。被导向命中的记忆单元立即恢复至设定的上限值；未被命中的记忆单元执行自减操作。

**Why this priority**: Forward 是 Memory 层的核心行为——触发恢复与自减共同构成记忆的生命周期，是网络前向传播的关键环节。

**Independent Test**: 创建一个已初始化的 Memory 层（部分单元设为非零值），传入路由信号（指定哪些位置被触发），执行 forward 后验证：触发位置恢复为上限值，未触发位置减去自减值。

**Acceptance Scenarios**:

1. **Given** Memory 层长度 5、上限值 10、自减值 1，其中位置 1 和 3 当前值为 5，**When** forward 传入路由信号触发位置 1 和 3，**Then** 位置 1 和 3 的值恢复为 10，位置 0、2、4 的值从当前值减 1。
2. **Given** 某未触发记忆单元当前值为 0，自减值 1，**When** forward 执行，**Then** 该单元保持为 0（不发生下溢）。
3. **Given** 某未触发记忆单元当前值为 1，自减值 3，**When** forward 执行，**Then** 该单元变为 0（自减值大于当前值时，结果置为 0，不溢出）。

---

### User Story 3 — 触发状态查询 (Priority: P2)

使用者需要查询整个 Memory 层各单元的触发状态：值不为 0 即表示处于触发状态，值为 0 表示未触发。

**Why this priority**: 触发状态是 Memory 层对外输出的核心语义——上层网络根据触发状态决定后续行为。

**Independent Test**: 创建 Memory 层，手动设置部分单元为非零值，调用状态查询，验证返回的触发掩码与预期一致。

**Acceptance Scenarios**:

1. **Given** Memory 层长度 4，其中位置 0=0, 位置 1=5, 位置 2=0, 位置 3=255，**When** 查询触发状态，**Then** 返回 [false, true, false, true]。
2. **Given** 所有记忆单元值均为 0，**When** 查询触发状态，**Then** 返回全部为 false。

---

### User Story 4 — UID 标识与数据持久化 (Priority: P2)

每个 Memory 层实例必须有唯一 UID 编号，且支持将全部参数及当前状态序列化保存和完整恢复。

**Why this Priority**: 持久化是网络训练中断/恢复、模型部署的必要能力。

**Independent Test**: 创建 Memory 层，执行若干 forward 操作改变内部状态后保存，从保存数据加载，验证加载后的参数和所有记忆单元值与保存前完全一致。

**Acceptance Scenarios**:

1. **Given** 一个已初始化和运行过 forward 的 Memory 层，**When** 调用保存函数，**Then** 生成包含 UID、长度、精度类型、上限值、自减值、所有记忆单元值的数据块。
2. **Given** 一份之前保存的有效数据块，**When** 调用加载函数，**Then** 重建 Memory 层，所有参数与保存前一致。
3. **Given** 一份损坏的数据块，**When** 调用加载函数，**Then** 返回错误指示，不创建无效的 Memory 层。

---

### Edge Cases

- 上限值设为 0 时，"触发即遗忘"——需确认是否为合法配置
- Memory 长度为 1 的最小情况
- 所有记忆单元同时触发 / 同时耗尽（全为 0）
- 极大长度（如 10000 单元）
- uint64 精度下最大上限值（2^64-1）
- 自减值设为 0 的特殊情况

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 系统 MUST 支持创建一维定长 Memory 层，指定长度、精度类型（uint8/16/32/64）、上限值和自减值。
- **FR-002**: 系统 MUST 为每个 Memory 层实例分配全局唯一的 UID 编号。
- **FR-003**: 系统 MUST 在 forward 操作中接收布尔路由信号（位掩码，长度与 Memory 层一致）。
- **FR-004**: 被触发的记忆单元 MUST 立即将其值恢复为设定的上限值。
- **FR-005**: 未被触发的记忆单元 MUST 将其当前值减去自减值，防下溢（无符号安全）。
- **FR-006**: 系统 MUST 提供触发状态查询接口：值 > 0 为触发，值 == 0 为未触发。
- **FR-007**: 系统 MUST 支持 Memory 层全部参数和状态的序列化保存。
- **FR-008**: 系统 MUST 支持从序列化数据加载重建，格式无效 MUST 返回错误。
- **FR-009**: 系统 MUST 使用纯 C 语言 + Makefile 编译，Windows 平台可编译为独立 .o。
- **FR-010**: 每种精度类型 MUST 有对应的实现变体，接口行为保持一致。

### Key Entities

- **MemoryLayer**: uid, length, precision, max_value, decay, cells[]
- **BoolRouter**: 外部模块，提供触发信号位掩码
- **SerializedData**: 紧凑二进制格式

## Success Criteria *(mandatory)*

- **SC-001**: 独立编译，无外部依赖
- **SC-002**: 四种精度变体行为一致
- **SC-003**: 1000 次完整生命周期无内存泄漏
- **SC-004**: 加载后逐位 100% 精确恢复
- **SC-005**: 10000 单元单次 forward < 1ms
- **SC-006**: 100% 非法输入被拒绝并返回错误码

## Assumptions

- 布尔路由器输出由框架其他模块提供
- UID 由框架统一分配
- 序列化格式为紧凑二进制
- Windows 开发环境已配置 GCC/MinGW 和 GNU Make
- Memory 层的长度在创建时确定，运行时不可变
