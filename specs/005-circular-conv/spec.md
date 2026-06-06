# Feature Specification: 一维回环卷积

**Feature Branch**: `005-circular-conv`

**Created**: 2026-06-06

**Status**: Draft

**Input**: 宪章原则 V — 一维回环卷积：卷积核到下边界溢出后多余的溢出部分与输入最前方运算，保证每一个数据处理次数相同。padding 无效参数，保留步长 (stride) 与空洞卷积间隔 (dilation)。

## User Scenarios & Testing

### User Story 1 — 基本回环卷积 (Priority: P1)

使用者输入一维整数序列和卷积核，执行回环卷积：当卷积核滑出输入末尾时，溢出的部分
从输入开头继续取数。每个输入位置被处理的次数完全相同。

**Why this priority**: 回环卷积是 BoolNet 的核心特征提取层，保证每个数据点被平等对待。

**Independent Test**: 输入 `[1,2,3,4]`、核 `[1,1]`（kernel_size=2）、stride=1、
dilation=1，输出 `[1+2, 2+3, 3+4, 4+1] = [3,5,7,5]`。

**Acceptance Scenarios**:

1. **Given** 输入长度 4、核 [1,1]、stride=1、dilation=1，**When** 回环卷积，**Then** 输出长度 4（与输入等长），值为 `[3,5,7,5]`（最后一个 4+1=5 是回环）。
2. **Given** 输入长度 6、核 [1,0,1]、stride=1、dilation=2，**When** 回环卷积，**Then** 每个输入位置恰好被处理 kernel_size 次。
3. **Given** 步长 > 1（如 stride=2），**When** 回环卷积，**Then** 输出长度 = ceil(输入长度/stride)，回环仍然生效。

---

### User Story 2 — 空洞卷积 (Priority: P2)

使用者指定 dilation > 1，卷积核元素之间跳过 (dilation-1) 个输入位置取值。

**Why this priority**: 空洞卷积扩大感受野而不增加参数，是实现多尺度特征的基础。

**Independent Test**: 输入 `[1,2,3,4,5,6]`、核 `[1,1]`（size=2）、stride=1、
dilation=2，输出 `[1+3, 2+4, 3+5, 4+6, 5+1, 6+2] = [4,6,8,10,6,8]`。

---

### Edge Cases

- 输入长度小于卷积核有效长度（kernel_size + (kernel_size-1)*(dilation-1) > N）：返回错误
- 步长 = 1（无下采样）、步长 = 输入长度（单输出）
- 卷积核大小 = 1（逐元素乘法）、dilation = 1（标准卷积）

## Requirements

- **FR-001**: MUST 支持一维回环卷积：溢出时从输入开头继续取数
- **FR-002**: MUST 保证每个输入位置被精确处理 kernel_size 次
- **FR-003**: MUST 支持 stride ≥ 1、dilation ≥ 1 参数
- **FR-004**: padding MUST 为无效参数（回环天然消除边界效应）
- **FR-005**: MUST 支持任意整数输入类型（通过模板/宏支持 int8/16/32/64）
- **FR-006**: 纯 C + Makefile 编译

## Success Criteria

- **SC-001**: 10000 长度输入、kernel_size=3 卷积 < 2ms
- **SC-002**: 每个输出位置等于 kernel_size 个输入的加权和（可验证）
- **SC-003**: 任意位置出现次数 = kernel_size（遍历统计验证）
