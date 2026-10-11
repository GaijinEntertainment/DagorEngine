options gen2
require daslib/runtime_memory
require dastest/testing_boost public

[test]
def test_context_memory_tracks_owned_allocations(t : T?) {
    let before = context_memory()
    var data : array<uint8>; resize(data, 1048576)
    let during = context_memory()
    t |> success(during.heap_used >= before.heap_used + 1048576ul)
    t |> success(during.heap_reserved >= during.heap_used)
    t |> success(during.strings_reserved >= during.strings_used)
    t |> success(during.stack > 0ul)
    delete data
    t |> success(context_memory().heap_used < during.heap_used)
    if (get_running_platform_name() == "emscripten") {
        let allocator = native_allocator_stats(); let extent = native_allocator_extent()
        t |> success(allocator.x > 0ul && extent.y > extent.x)
        t |> success(allocator.x + allocator.y <= extent.y - extent.x)
        t |> success(during.native_stack > 0ul)
    } else {
        t |> equal(native_allocator_stats(), urange64(0ul, 0ul))
    }
}

require daslib/jobque_boost
var owned_global_memory : int[128]
[test]
def test_idle_fork_memory_is_separate_from_owner(t : T?) {
    owned_global_memory[0] = 42
    t |> equal(context_idle_forks().count, 0ul)
    set_jobque_threads(2)
    var results : array<int>; resize(results, 8)
    var destination = unsafe(addr(results[0]))
    with_job_que() {
        set_jobque_fork_pool(true, true)
        parallel_for(0, 8, 2) $(first, last, done) {
            var payload : array<uint8>; resize(payload, 4096)
            for (i in range(first, last)){unsafe(destination[i]) = i + length(payload)}
            delete payload
            done |> notify_and_release
        }
    }
    let idle = context_idle_forks()
    t |> success(idle.count > 0ul)
    t |> success(idle.memory.stack >= idle.count * 16384ul)
    t |> equal(idle.memory.globals, idle.count * context_memory().globals)
    t |> success(idle.memory.globals > 0ul)
    t |> equal(owned_global_memory[0], 42)
    t |> success(idle.memory.heap_reserved >= idle.memory.heap_used)
    t |> success(idle.memory.strings_reserved >= idle.memory.strings_used)
    t |> equal(idle.memory.native_stack, 0ul, "fork contexts are not native threads")
    for (v, i in results, count()){t |> equal(v, i + 4096)}
    delete results; set_jobque_threads(0)
}
