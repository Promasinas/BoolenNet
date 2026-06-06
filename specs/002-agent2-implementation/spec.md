# Feature Specification: Agent 2 - Memory & Activation Management

**Feature Branch**: `002-agent2-implementation`

**Created**: 2026-06-06

**Status**: Draft

**Input**: User description: "这是agent2"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Write Activation to Memory Partition (Priority: P1)

As a BoolNet system operator, when Agent 1's Boolean cascade router determines a target partition address, Agent 2 receives the activation data and writes it into the correct fixed-size memory slot. The write operation must complete within the routing cycle so that subsequent reads can retrieve the stored activation without delay.

**Why this priority**: Memory write is the fundamental operation that enables the entire architecture. Without reliable writes, the system cannot store any activation state, making all other functionality impossible.

**Independent Test**: Can be fully tested by sending a partition address and activation vector to Agent 2 and verifying the activation persists in the specified slot and can be retrieved immediately.

**Acceptance Scenarios**:

1. **Given** Agent 1 has produced a valid partition address and activation vector, **When** Agent 2 receives the write command, **Then** the activation is stored in the exact memory slot corresponding to the address within one cycle.
2. **Given** a memory partition already contains an activation, **When** a new activation is written to the same partition address, **Then** the old activation is overwritten (hard update) with the new one.
3. **Given** an invalid partition address (out of bounds), **When** a write is attempted, **Then** the operation is rejected with a clear error indicator and no memory corruption occurs.

---

### User Story 2 - Read Activation from Memory Partition (Priority: P1)

As a BoolNet system operator, when a trigger signal arrives with a partition address, Agent 2 retrieves the stored activation from the corresponding fixed-size memory slot and returns it as the output. The read operation must be constant-time (O(1)) regardless of which partition is addressed.

**Why this priority**: Reading stored activations is the co-equal fundamental operation. The architecture's key advantage — hard-routed constant-time memory access — depends on reads being O(1) direct access, not a search.

**Independent Test**: Can be fully tested by writing known activations to specific slots, then issuing read commands for those addresses and verifying the exact activation is returned.

**Acceptance Scenarios**:

1. **Given** a memory partition has a stored activation, **When** Agent 2 receives a read command with that partition's address, **Then** the activation is returned within constant time.
2. **Given** a memory partition is empty (no activation ever written), **When** Agent 2 receives a read command for that address, **Then** a zero/default activation vector is returned (silent default).
3. **Given** multiple consecutive reads for the same address, **When** no intervening write occurs, **Then** the same activation is returned each time (idempotent read).

---

### User Story 3 - Trigger-Based Activation Release (Priority: P2)

As a BoolNet system operator, when a Boolean trigger condition is met (e.g., a routing path matches a stored pattern), Agent 2 releases the weighted sum of activations from the triggered memory partitions. The released activation is a linear combination of multiple partition contents weighted by their trigger strength.

**Why this priority**: Trigger-based release is the key mechanism that differentiates this architecture from a simple key-value store — it enables soft, distributed memory recall rather than hard single-slot lookup.

**Independent Test**: Can be fully tested by pre-loading several memory partitions with known activations, sending a trigger pattern that partially matches multiple partitions, and verifying the output is the correct weighted sum.

**Acceptance Scenarios**:

1. **Given** multiple memory partitions contain activations, **When** a trigger pattern arrives with per-partition weights, **Then** Agent 2 returns the weighted sum of all triggered partition activations.
2. **Given** a trigger pattern that matches no partitions (all weights zero), **When** the trigger fires, **Then** Agent 2 returns a zero activation vector.
3. **Given** a trigger pattern that exactly matches one partition (weight=1.0 for one, 0 for others), **When** the trigger fires, **Then** Agent 2 returns exactly that partition's activation (sharp recall mode).

---

### User Story 4 - Memory Partition Status Monitoring (Priority: P3)

As a BoolNet system operator, I can query which memory partitions are currently occupied, their write timestamps, and aggregate statistics (fill ratio, write frequency) to understand memory utilization and detect potential saturation.

**Why this priority**: Monitoring enables debugging and tuning of the routing cascade (Agent 1) by revealing how evenly the Boolean routing distributes activations across partitions. However, the core read/write/trigger functionality works without it.

**Independent Test**: Can be fully tested by writing to a subset of partitions, then querying the status and verifying the occupancy map matches exactly which partitions were written.

**Acceptance Scenarios**:

1. **Given** activations have been written to some partitions, **When** a status query is issued, **Then** Agent 2 returns a bitmask indicating which partitions are occupied.
2. **Given** a partition was written at time T1 and read at time T2, **When** the status is queried, **Then** the partition reports last-write-time = T1 and last-access-time = T2.

---

### Edge Cases

- What happens when all memory partitions are full and a new write arrives for a new address? (The oldest or least-recently-used partition should be evicted.)
- How does the system handle concurrent write and trigger-read to the same partition? (Write takes precedence; trigger-read sees the new value.)
- What happens when the activation vector size does not match the fixed memory slot size? (Reject with dimension mismatch error.)
- How does the system handle a trigger pattern with negative weights? (Negative weights should be supported and produce subtractive contributions to the weighted sum.)
- What happens when the partition address space is sparse (many empty slots) and a bulk trigger fires? (Empty slots contribute zero activation; weighted sum is computed only over occupied triggered partitions for efficiency.)

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST accept write commands containing a partition address (non-negative integer) and an activation vector of fixed dimension D.
- **FR-002**: System MUST accept read commands containing a partition address and return the stored activation vector for that partition in constant O(1) time.
- **FR-003**: System MUST accept trigger commands containing a set of (partition address, weight) pairs and return the weighted sum of the corresponding partition activations.
- **FR-004**: System MUST initialize all memory partitions to the zero activation vector at system start.
- **FR-005**: System MUST validate partition addresses against the total number of partitions P and reject out-of-bounds addresses.
- **FR-006**: System MUST validate activation vector dimensions against the fixed memory slot dimension D and reject mismatched vectors.
- **FR-007**: System MUST support an eviction policy when all P partitions are occupied and a write to a new partition address is requested. The default policy SHALL be overwrite of the least-recently-accessed partition.
- **FR-008**: System MUST provide a status query that returns the occupancy bitmask (P bits) indicating which partitions contain non-zero activations.
- **FR-009**: System MUST track and report per-partition metadata: last-write timestamp and last-access timestamp.
- **FR-010**: System MUST ensure read operations are non-destructive (the stored activation persists after being read).
- **FR-011**: System MUST handle trigger operations with sparse weight sets efficiently — only occupied, triggered partitions contribute to the weighted sum computation.
- **FR-012**: System MUST support negative trigger weights, producing subtractive contributions in the weighted sum output.
- **FR-013**: System MUST provide a reset operation that clears all memory partitions to zero and resets all metadata.
- **FR-014**: System MUST support configurable memory parameters: total number of partitions P and activation vector dimension D, set at initialization time.

### Key Entities

- **Memory Partition**: A fixed-size slot identified by an integer address (0 to P-1). Stores one activation vector of dimension D plus metadata (write timestamp, access timestamp, occupancy flag). Represents the atomic unit of memory in the BoolNet architecture.
- **Activation Vector**: A fixed-length (dimension D) numeric vector representing the stored state within a partition. All partitions share the same vector dimension. The values are real numbers that result from Agent 1's Boolean routing decisions.
- **Trigger Pattern**: A sparse set of (partition address, weight) pairs. Weights are real numbers in range [-1, 1] representing the trigger strength for each addressed partition. Drives the weighted-sum readout operation.
- **Partition Address**: A non-negative integer in range [0, P-1] that directly indexes a memory partition. Produced by Agent 1's Boolean cascade router as the routing destination.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Memory read and write operations complete in constant time — throughput does not degrade as the number of occupied partitions increases from 10% to 100% fill.
- **SC-002**: Trigger-based weighted-sum readout across all P partitions completes within the same time budget as a single-partition read (hardware-parallel accumulation).
- **SC-003**: Partition address validation rejects 100% of out-of-bounds addresses with zero memory corruption.
- **SC-004**: The system correctly handles sparse activation patterns — when only K out of P partitions are occupied (K << P), trigger readout computational cost scales with K, not P.
- **SC-005**: Memory fill ratio can be queried and reported within a single operation cycle, enabling real-time routing quality monitoring by Agent 1.
- **SC-006**: An operator can configure the memory system (P, D) and verify correct operation with a small diagnostic test (write/read/trigger cycle) in under 5 minutes.

## Assumptions

- Agent 1 (Boolean Cascade Router) produces valid partition addresses through its routing mechanism; Agent 2 is responsible for validating addresses but not for correcting routing errors.
- The activation vector dimension D is fixed at system initialization and matches the output dimension of Agent 1's routing output.
- The memory system operates in a single-threaded execution model: commands (write, read, trigger, status) are processed sequentially, not concurrently.
- Memory partitions reside in main memory (RAM) for fast access; persistent storage is out of scope for this version.
- The trigger mechanism's weight values are provided externally (by a controller or higher-level agent) — Agent 2 computes the weighted sum but does not generate trigger patterns.
- Partition count P is expected to be in the range [256, 65536] for typical configurations, but the design should not hard-code an upper bound.
- The system is designed as a component within the larger BoolNet architecture and communicates with Agent 1 via a well-defined interface (address + activation vector).
- Eviction policy (LRU) is a reasonable default; more sophisticated policies (LFU, FIFO, score-based) may be added in future versions.
