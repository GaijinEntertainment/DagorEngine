---
name: daFG Node Scheduling Rules
description: How daFG determines execution order -- dependency edges from resource lifecycles, explicit ordering, multiplexing fan-out, pruning from sinks, and Kahn's algorithm tie-breaking. Enough to predict the exact order of any node set.
type: reference
commit_reference: 542abf2623ae 2026-04-28
---

# daFG Node Scheduling -- Predicting Execution Order

Given nodes A, B, C, D with various resources and multiplexing modes, this document lets you determine their execution order.

## Step 1: Dependency Edges (built by IrGraphBuilder::addEdgesToIrGraph)

Every ordering constraint is an edge in the IR graph's `predecessors` set:
`node.predecessors.insert(other)` means "other must finish before node starts".

### 1a. Resource Lifecycle Edges

For each resource R with its `VirtualResourceLifetime`:

```
introducedBy  = I   (node that creates or renames-into R)
modifiers     = [M1, M2, ...]   (nodes that call modify(R))
readers       = [R1, R2, ...]   (nodes that call read(R))
consumedBy    = C   (node that renames R into something else)
```

The edges form a **bipartite fan**, NOT a chain:

```
Case: both modifiers and readers exist (full bipartite, not 1:1)

    I --+--> M1 --+--> R1 --+--> C
        |     \  / \  / |         |
        +--> M2 -X--> R2 --+-----+
        |     /  \ /  \ |
        +--> M3 --+--> R3 --+

Every modifier connects to EVERY reader (dense cross-product).

Edges:  I -> every M          (I is predecessor of each M)
        every M -> every R    (EACH M is predecessor of EACH R)
        every R -> C          (each R is predecessor of C)
```

**CRITICAL: Modifiers are NOT ordered relative to each other.** M1 and M2 can execute in any order. The scheduler picks an order based on tie-breaking heuristics (see Step 4). Same for readers -- R1 and R2 are unordered.

Degenerate cases:
- No modifiers, no readers: `I -> C` (direct edge)
- Only modifiers (no readers): `I -> every M`, `every M -> C`
- Only readers (no modifiers): `I -> every R`, `every R -> C`

### 1b. Explicit Ordering Edges

`registry.orderMeBefore("B")` called by node A:
- A's `followingNodeIds` contains B
- Edge: A is inserted into B.predecessors

`registry.orderMeAfter("B")` called by node A:
- A's `precedingNodeIds` contains B
- Edge: B is inserted into A.predecessors

These edges are added per multiplexing index (see Step 2).

### 1c. History Reads Create NO Edges

`historyFor(R)` reads the previous frame's version. No ordering edge is added within the current frame. History readers are only kept alive by the pruning reachability pass (see Step 3).

### 1d. SideEffects::None

Nodes with `executionHas(SideEffects::None)` (or void declaration callback) have an empty execution callback. Their resource requests ARE recorded in the registry and DO create dependency edges through the resource lifetime data (steps 1a). However, at the IR level, their requests are moved to `supressedRequests` -- this suppresses barrier events (no GPU barriers placed for these requests). The nodes are still scheduled and their empty callback is executed. They can be pruned if unreachable from sinks (step 3).

## Step 2: Multiplexing Fan-out

Each frontend node and resource is expanded into multiple IR instances based on their multiplexing mode and the current `Extents`.

Edge replication rule (line 1133 of irGraphBuilder.cpp):
```cpp
for (midx in all_multiplexing_indices):
    graph.nodes[mapNode(from, midx)].predecessors.insert(mapNode(to, midx))
```

**Key behavior**: undermultiplexed nodes map to the SAME IR index for all `midx`. This creates fan-in/fan-out patterns:

### Example: A creates R (FullMultiplex), B reads R (Mode::None, runs once), 2 viewports

IR nodes: A[vp0], A[vp1], B (single)

Edges built per midx:
- midx=0: B.predecessors.insert(A[vp0])
- midx=1: B.predecessors.insert(A[vp1])

Result: B waits for BOTH A[vp0] and A[vp1]. Correct: the non-multiplexed node must wait for all multiplexed instances.

### Example: A creates R (Mode::None), B reads R (FullMultiplex), 2 viewports

IR nodes: A (single), B[vp0], B[vp1]

Edges built per midx:
- midx=0: B[vp0].predecessors.insert(A)
- midx=1: B[vp1].predecessors.insert(A)

Result: Both B[vp0] and B[vp1] wait for A. Correct: A runs once, then all Bs can run.

### Example: Both FullMultiplex, 2 viewports

IR nodes: A[vp0], A[vp1], B[vp0], B[vp1]

Edges: B[vp0] waits for A[vp0], B[vp1] waits for A[vp1]. Each viewport runs independently.

## Step 3: Pruning (IrGraphBuilder::pruneGraph)

After edges are built, a DFS from **sink nodes** determines which nodes survive.

Sink nodes are:
1. All nodes with `SideEffects::External`
2. Nodes that produce/modify/consume resources in `sinkExternalResources`

Nodes NOT reachable from any sink (traversing predecessor edges from sinks toward their dependencies) are pruned. Exception: nodes whose resources have history readers get added as extra DFS roots (a second wave).

**Consequence**: If your node has `SideEffects::Internal` and no other node (directly or transitively) reads its output, the node is removed from the graph entirely.

## Step 4: Topological Sort (NodeScheduler::schedule)

The scheduler produces a total ordering of all surviving IR nodes using **reverse Kahn's algorithm**.

### Algorithm

Note: "in-degree" here is **reverse in-degree** -- for each node, it counts how many OTHER nodes list it in their `predecessors` set. In standard graph terms, this is OUT-degree (number of dependents). A node with reverse-in-degree 0 has no dependents = is a leaf/sink.

1. Compute reverse-in-degree for each node
2. Compute pass-level reverse-in-degree (cross-pass dependent count)
3. Initialize frontier with the graph's back node (a sentinel that has no dependents)
4. **While frontier is not empty**:
   a. Pick the "best" node from frontier via `max_element` with comparator (see below)
   b. Assign it position `timer--` (timer starts at N-1, counts down)
   c. First scheduled = LAST to execute; last scheduled = FIRST to execute
   d. For each predecessor of the scheduled node: decrement its reverse-in-degree; if it hits 0, add to frontier
   e. Decrement pass-level reverse-in-degree for cross-pass predecessors

### Tie-breaking comparator (orderComp)

When multiple nodes have reverse-in-degree 0 (no remaining dependents), the comparator picks which to schedule first (= execute last). Applied in order, first difference wins:

| Priority | Criterion | Effect |
|----------|-----------|--------|
| 1 | `multiplexingIndex` | Lower index executes earlier (higher index scheduled first) |
| 2 | `pass_coloring[n] == lastColor` | Nodes matching the last scheduled pass color are preferred (keeps pass contiguous) |
| 3 | `passInDegree[pass_coloring[n]]` | Passes with lower remaining cross-edge reverse-in-degree are scheduled first (= execute later). A pass at 0 means all its dependents are already scheduled, so its nodes can be placed contiguously |
| 4 | `node.priority` | Higher `priority_t` value = scheduled first = executes later. `PRIO_AS_LATE_AS_POSSIBLE` = INT32_MAX, `PRIO_AS_EARLY_AS_POSSIBLE` = INT32_MIN |
| 5 | Node index | Stability tiebreaker (higher index scheduled first) |

## Step 5: Worked Example

Given:
```
Node A: creates texture "depth" (FullMultiplex)
Node B: modifies "depth"        (FullMultiplex)
Node C: reads "depth"           (FullMultiplex)
Node D: reads "depth"           (Mode::None)
Extents: 2 viewports, rest = 1
```

### IR expansion
- A[0], A[1] (2 instances)
- B[0], B[1]
- C[0], C[1]
- D (1 instance, undermultiplexed)

### Resource lifetime for "depth"
```
introducedBy = A
modifiers = [B]
readers = [C, D]
consumedBy = (none)
```

### Edges (per midx)
Both modifiers and readers exist, so full bipartite:
- midx=0: A[0] -> B[0], B[0] -> C[0], B[0] -> D
- midx=1: A[1] -> B[1], B[1] -> C[1], B[1] -> D

D (undermultiplexed) gets edges from BOTH B[0] and B[1].

### Resulting valid orderings
Constraints: A[i] before B[i] before {C[i], D}. D must wait for BOTH B[0] and B[1].

Some valid schedules:
- `A[0], B[0], C[0], A[1], B[1], C[1], D` -- viewport-0 pipeline first, then viewport-1, D last
- `A[0], A[1], B[0], B[1], C[0], C[1], D` -- all creates, all modifies, all reads
- `A[0], B[0], A[1], B[1], C[0], C[1], D` -- interleaved

The actual schedule depends on tie-breaking heuristics (multiplexing index preference, pass coloring, priority). With default settings and same pass colors, the scheduler tends to group same-viewport nodes together (criterion 1: lower multiplexing index executes earlier).

## Rename Chains

When node X does `rename("depth", "prev_depth")`:
- "depth" lifetime gets `consumedBy = X`
- "prev_depth" lifetime gets `introducedBy = X`
- Both resource lifetimes contribute edges independently
- The rename creates a **scheduling barrier**: all readers of "depth" must finish before X, and X must finish before anything using "prev_depth"

## Summary: Quick Rules

| Situation | Ordering |
|-----------|----------|
| A creates R, B reads R | A before B |
| A creates R, B renames R | A before B (direct edge, no middle nodes) |
| A creates R, B modifies R | A before B |
| A modifies R, B reads R | A before B |
| A modifies R, B modifies R | **Unordered** (any order) |
| A reads R, B reads R | **Unordered** (any order) |
| A modifies R, B renames R | A before B |
| A reads R, B renames R | A before B |
| A reads historyFor(R) | No edge (previous frame) |
| A.orderMeBefore("B") | A before B |
| A.orderMeAfter("B") | B before A |
| A is FullMultiplex, B is None, same resource | B waits for ALL A instances |
| A is None, B is FullMultiplex, same resource | ALL B instances wait for A |
