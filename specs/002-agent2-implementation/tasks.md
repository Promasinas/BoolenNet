# Tasks: Agent 2 — Memory & Activation Management

**Input**: Design documents from `/specs/002-agent2-implementation/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Agent 3 在后续阶段负责自动化测试。

**Organization**: 任务按用户故事分组。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 可并行执行
- **[Story]**: 所属用户故事（US1–US4）

---

## Phase 1: Setup

**Purpose**: 验证已有模块完整性，准备集成

- [x] T001 验证已有底层模块编译通过：`src/boolnet_common.h`、`src/activation.h/c`、`src/partition.h/c`、`src/trigger.h/c`（在 `src/` 目录执行 `make` 确认各 .o 文件生成无错误）
- [x] T002 [P] 创建类型聚合头文件 `src/activation_mgr_types.h`（聚合 `memory_types.h` 中的 `memory_config_t`、`partition_status_t`、`trigger_entry_t`、`trigger_pattern_t`、错误码常量；若文件不存在则从零创建，参见 `contracts/activation-mgr-api.md`）
- [x] T003 [P] 更新构建文件 `src/Makefile`（增加 `activation_mgr.o` 目标，依赖 `activation.o`、`partition.o`、`trigger.o`）

**Checkpoint**: 所有已有模块可编译，Makefile 包含新目标

---

## Phase 2: Foundational

**Purpose**: 统一接口骨架——activation_mgr 初始化/重置/校验

- [x] T004 创建统一接口头文件 `src/activation_mgr.h`（声明 `activation_mgr_init`/`reset`/`write`/`read`/`trigger`/`get_status`/`get_bitmask`/`occupied_count`/`fill_ratio`；include `activation_mgr_types.h`；FR-004、FR-013、FR-014）
- [x] T005 实现初始化与重置 `src/activation_mgr.c`（`activation_mgr_init(P, D)` 调用 `partition_store_alloc` 分配 P×D 数组 + 元数据；`activation_mgr_reset()` 调用 `partition_store_clear`；维护 `g_initialized` 全局标志；FR-004、FR-013、FR-014）
- [x] T006 实现入口校验 `src/activation_mgr.c`（`_check_ready()` 检查 `g_initialized` → 否则返回 ERR_NOT_INIT；`_check_address(addr)` 检查 `addr < P` → 否则返回 ERR_BAD_ADDRESS；供所有后续函数共用）

**Checkpoint**: `activation_mgr_init(256, 64)` 成功，`activation_mgr_reset()` 清零，未初始化调用返回错误

---

## Phase 3: User Story 1 — Write Activation (Priority: P1) 🎯 MVP

**Goal**: `activation_mgr_write(address, activation)` 将 D 维浮点向量写入分区

**Independent Test**: 写入后 `activation_mgr_read` 返回相同向量

### Implementation

- [x] T007 [US1] 实现 `activation_mgr_write()` `src/activation_mgr.c`（(1) `_check_ready`；(2) `_check_address`；(3) `activation != NULL` → 否则 ERR_NULL_PARAM；(4) 若 `!partition_is_occupied(addr)` 且无空闲槽 → 调用 `partition_evict_lru()`；(5) `activation_copy` 写入 `partition_get_ptr(addr)`；(6) `partition_touch_write(addr)`；FR-001、FR-005、FR-006、FR-007）
- [x] T008 [US1] 实现写入维度校验 `src/activation_mgr.c`（传入向量维度必须等于 D，否则 ERR_DIM_MISMATCH；FR-006）

**Checkpoint**: `activation_mgr_write(42, vec)` → 数据写入分区 42，地址越界被拒绝

---

## Phase 4: User Story 2 — Read Activation (Priority: P1)

**Goal**: `activation_mgr_read(address, out)` O(1) 读取激活向量

**Independent Test**: 写入后读取精确匹配；空分区返回零

### Implementation

- [x] T009 [US2] 实现 `activation_mgr_read()` `src/activation_mgr.c`（(1) `_check_ready`；(2) `_check_address`；(3) `out != NULL`；(4) 若 `partition_is_occupied(addr)` → `activation_copy`；否则 `activation_zero(out)`；(5) `partition_touch_read(addr)` 更新 last_access 但不修改 occupancy；FR-002、FR-010）
- [x] T010 [US2] 实现 `activation_mgr_occupied_count()` 和 `activation_mgr_fill_ratio()` `src/activation_mgr.c`（遍历 occupancy 位图计数 → count；fill_ratio = (float)count / P；SC-005）

**Checkpoint**: 完整读写闭环；重复读取幂等；空分区返回零

---

## Phase 5: User Story 3 — Trigger Weighted-Sum (Priority: P2)

**Goal**: `activation_mgr_trigger(pattern, out)` 稀疏加权和

**Independent Test**: 写入 3 个分区后触发不同权重，验证加权和精确计算

### Implementation

- [x] T011 [P] [US3] 实现 `activation_mgr_trigger()` `src/activation_mgr.c`（(1) `_check_ready`；(2) `pattern != NULL && pattern.output_dim == D` → 否则 ERR_DIM_MISMATCH；(3) `pattern.num_entries == 0` → 返回零向量；(4) 调用 `trigger_compute()` 执行加权和；(5) 更新所有被触发分区的 last_access；FR-003、FR-011、FR-012）
- [x] T012 [US3] 验证触发稀疏优化 `src/activation_mgr.c`（确认仅遍历被触发且占用的分区，不扫描全部 P；SC-002、SC-004）

**Checkpoint**: 加权和正确（含负权重减法）；空分区不参与；维度不匹配拒绝

---

## Phase 6: User Story 4 — Status Monitoring (Priority: P3)

**Goal**: 分区状态查询——单分区详情、占用位图、聚合统计

**Independent Test**: 写入 N 个分区后查询状态，验证位图和统计准确

### Implementation

- [x] T013 [P] [US4] 实现 `activation_mgr_get_status()` `src/activation_mgr.c`（填充 `partition_status_t`：address、occupied、last_write_time、last_access_time、activation_preview[8]；FR-008、FR-009）
- [x] T014 [P] [US4] 实现 `activation_mgr_get_bitmask()` `src/activation_mgr.c`（遍历 P 分区逐字节填充位图；输出 ceil(P/8) 字节；FR-008）

**Checkpoint**: status/bitmask/occupied_count/fill_ratio 全部可用

---

## Phase 7: Polish

**Purpose**: 诊断程序、API 文档、最终验证

- [x] T015 [P] 编写诊断程序 `src/activation_mgr_diag.c`（独立 main：init(256, 64) → write 3 partitions → read 验证 → trigger 加权和 → status 查询 → 输出 PASS/FAIL 结果 → reset → 输出 summary；对应 SC-006 5 分钟配置验证）
- [x] T016 [P] 编写 API 文档 `docs/api/activation_mgr_api.md`（按 `contracts/activation-mgr-api.md` 函数签名；包含初始化示例、读写触发完整代码片段；中英双语）
- [x] T017 端到端编译验证（`make activation_mgr.o` + 编译 `activation_mgr_diag.exe`；运行诊断程序确认所有测试 PASS）
- [x] T018 [P] 代码审查：检查 `malloc`/`calloc` 与 `free` 配对；验证边界检查完整性；确认 SoA 布局正确

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: 无依赖——立即开始
- **Foundational (Phase 2)**: 依赖 Phase 1
- **US1 Write (Phase 3)**: 依赖 Phase 2
- **US2 Read (Phase 4)**: 依赖 Phase 3（需要 write 验证 read）
- **US3 Trigger (Phase 5)**: 依赖 Phase 3（需要 write 设置测试数据）
- **US4 Status (Phase 6)**: 依赖 Phase 3（需要 write 产生元数据）
- **Polish (Phase 7)**: 依赖 Phase 3–6

### Parallel Opportunities

- **Phase 1**: T002、T003 可并行
- **Phase 5**: T011 独立
- **Phase 6**: T013、T014 可并行
- **Phase 7**: T015、T016、T018 可并行

---

## Implementation Strategy

### MVP (US1 + US2)

1. Phase 1: Setup → 验证已有模块
2. Phase 2: Foundational → activation_mgr 骨架
3. Phase 3: US1 Write → 写入可用
4. Phase 4: US2 Read → 读写闭环
5. **STOP**: 编译诊断程序验证读写

### Full Delivery

MVP + US3 (Trigger) + US4 (Status) + Polish → 完整 Memory 层

---

## Notes

- 已有底层模块（activation/partition/trigger）为内部实现，activation_mgr 为对外统一接口
- 所有内存分配一次性完成（init），无动态增长
- LRU 淘汰仅在所有分区占满时触发
- 未初始化调用返回 ERR_NOT_INIT
