# Runtime memory architecture

## 43. Runtime memory snapshots {#runtime-memory-snapshots}

`runtime_memory.context_idle_forks` and `Context::getIdleForkMemory` share the selector
contract:0 returns heap used/reserved bytes,1 returns string-heap used/reserved bytes,
2 returns stack/global bytes, and3 returns the idle-context count. The C++ owner
locks its pool and visits only returned forks. A complete pool snapshot follows the
join of its jobs; active forks are excluded. The script wrapper publishes value
copies, not pointers into another context. Native thread stacks belong to workers,
not pooled contexts, and are not summed into idle-fork totals.
