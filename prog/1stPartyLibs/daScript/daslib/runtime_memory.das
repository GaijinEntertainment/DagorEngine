options gen2
module runtime_memory shared public

struct ContextMemory {
    //! Memory sizes in bytes, sampled on the owning context.
    heap_used : uint64 //! Live context heap bytes.
    heap_reserved : uint64 //! Reserved context heap bytes.
    strings_used : uint64 //! Live string heap bytes.
    strings_reserved : uint64 //! Reserved string heap bytes.
    stack : uint64 //! Context stack capacity.
    globals : uint64 //! Context global storage.
    native_stack : uint64 //! Current Emscripten thread stack capacity, zero elsewhere.
}
def context_memory() : ContextMemory {
    //! Samples the calling context. Publish the returned value copy across threads.
    let overhead = context_memory_sizes()
    return ContextMemory(heap_used = heap_bytes_allocated(), heap_reserved = heap_total_allocated(),
        strings_used = string_heap_bytes_allocated(), strings_reserved = string_heap_total_allocated(),
        stack = overhead.x, globals = overhead.y, native_stack = native_thread_stack_size())
}

struct IdleForkMemory {
    //! Memory retained by idle forks; active jobs are excluded.
    count : uint64 //! Retained idle context count.
    memory : ContextMemory //! Aggregate context sizes; native_stack is zero.
}
[arch(at="ARCHITECTURE_MEMORY.md#runtime-memory-snapshots")]
def context_idle_forks() : IdleForkMemory {
    //! Idle job contexts retained by this owner. Join jobs before sampling to include
    //! the complete pool. Active forks are excluded rather than reading changing heaps.
    let heap = context_idle_fork_memory(0); let strings = context_idle_fork_memory(1)
    let overhead = context_idle_fork_memory(2); let count = context_idle_fork_memory(3)
    return IdleForkMemory(count = count.x, memory = ContextMemory(heap_used = heap.x, heap_reserved = heap.y,
        strings_used = strings.x, strings_reserved = strings.y, stack = overhead.x, globals = overhead.y))
}
