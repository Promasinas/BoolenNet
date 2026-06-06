# Feature Specification: Agent 2 - Memory & Activation Management

**Feature Branch**: `002-agent2-implementation`

**Created**: 2026-06-06

**Status**: Draft

**Input**: User description: "这里是agent2"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Write Activation to Memory Partition (Priority: P1)

Agent 2 receives partition addresses from Agent 1's Boolean cascade router and writes activation vectors into the correct fixed-size memory slots. Write must complete within the routing cycle.

**Why this priority**: Memory write is the fundamental operation enabling the entire architecture.

**Independent Test**: Send a partition address and activation vector; verify the activation persists and can be retrieved immediately.

**Acceptance Scenarios**:

1. **Given** valid partition address and activation vector, **When** Agent 2 receives write command, **Then** activation is stored at the exact memory slot within one cycle.
2. **Given** partition already occupied, **When** new activation written to same address, **Then** old value is overwritten (hard update).
3. **Given** invalid address (out of bounds), **When** write attempted, **Then** operation rejected with error, no corruption.

---

### User Story 2 - Read Activation from Memory Partition (Priority: P1)

Agent 2 retrieves stored activation vectors in O(1) constant time by direct partition address indexing.

**Why this priority**: Constant-time read is the architecture's core advantage over search-based retrieval.

**Independent Test**: Write known activations to specific slots, then read and verify exact match.

**Acceptance Scenarios**:

1. **Given** partition has stored activation, **When** read command received, **Then** activation returned in constant time.
2. **Given** partition is empty, **When** read that address, **Then** zero/default vector returned (silent default).
3. **Given** multiple consecutive reads for same address, **When** no intervening write, **Then** same value returned (idempotent read).

---

### User Story 3 - Trigger-Based Activation Release (Priority: P2)

Agent 2 releases weighted-sum of triggered partition activations, enabling soft distributed memory recall.

**Why this priority**: Trigger-based release differentiates this from a simple key-value store.

**Independent Test**: Pre-load multiple partitions, send trigger pattern with varying weights, verify weighted sum.

**Acceptance Scenarios**:

1. **Given** multiple occupied partitions, **When** trigger pattern arrives, **Then** correct weighted sum returned.
2. **Given** trigger pattern matching no partitions, **When** trigger fires, **Then** zero vector returned.
3. **Given** trigger exactly matching one partition (weight=1.0), **When** trigger fires, **Then** exactly that partition's activation returned.

---

### User Story 4 - Memory Partition Status Monitoring (Priority: P3)

Query partition occupancy, timestamps, and aggregate statistics (fill ratio).

**Why this priority**: Monitoring enables routing quality assessment by Agent 1 but is not required for core functionality.

**Independent Test**: Write to subset of partitions, query status, verify occupancy map matches.

**Acceptance Scenarios**:

1. **Given** activations written to some partitions, **When** status query issued, **Then** correct occupancy bitmask returned.
2. **Given** partition written at T1 and read at T2, **When** status queried, **Then** last-write=T1, last-access=T2 reported.

---

### Edge Cases

- All partitions full on new write → LRU eviction of least-recently-accessed partition
- Concurrent write and trigger-read to same partition → write takes precedence
- Activation vector dimension mismatch → reject with dimension error
- Negative trigger weights → supported, subtractive contributions
- Sparse address space bulk trigger → only occupied triggered partitions contribute

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST accept write commands (partition address + D-dim activation vector)
- **FR-002**: System MUST O(1) read partition activation vectors
- **FR-003**: System MUST accept trigger commands (address+weight pairs) and return weighted sum
- **FR-004**: System MUST initialize all partitions to zero at startup
- **FR-005**: System MUST validate partition addresses in [0, P-1]
- **FR-006**: System MUST validate activation vector dimensions match D
- **FR-007**: System MUST support LRU eviction when all P partitions occupied
- **FR-008**: System MUST provide occupancy bitmask query (P bits)
- **FR-009**: System MUST track per-partition last-write and last-access timestamps
- **FR-010**: System MUST ensure read operations are non-destructive
- **FR-011**: System MUST handle sparse triggers — only occupied partitions contribute O(K)
- **FR-012**: System MUST support negative trigger weights (subtractive contributions)
- **FR-013**: System MUST provide reset operation clearing all partitions and metadata
- **FR-014**: System MUST support configurable P and D at initialization

### Key Entities

- **Memory Partition**: Fixed-size slot (address 0..P-1), stores one D-dim activation vector + metadata
- **Activation Vector**: D-dim float vector representing stored state
- **Trigger Pattern**: Sparse (address, weight) pairs; weights in [-1.0, 1.0]
- **Partition Address**: Non-negative integer in [0, P-1], direct array index

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Read/write O(1) constant time — throughput unchanged from 10% to 100% fill
- **SC-002**: Trigger weighted-sum O(K) where K = occupied triggered partitions
- **SC-003**: Address validation rejects 100% out-of-bounds addresses with zero corruption
- **SC-004**: Sparse trigger cost scales with K (occupied), not P (total)
- **SC-005**: Fill ratio queried within single operation cycle
- **SC-006**: Operator can configure (P, D) and verify write/read/trigger cycle in under 5 minutes

## Assumptions

- Agent 1 produces valid partition addresses; Agent 2 validates but does not correct errors
- Activation dimension D fixed at init, matches Agent 1 output
- Single-threaded sequential execution model
- Memory partitions in RAM; persistent storage out of scope for v1
- Trigger weights provided externally (Agent 1/Forward)
- P ∈ [256, 65536]; no hard-coded upper bound
- LRU is default eviction policy
