// Allocation Tracking via Debug Agent
//
// Demonstrates memory allocation callbacks in DapiDebugAgent:
//   onAllocate       — called on every heap allocation
//   onReallocate     — called when a heap block is resized
//   onFree           — called when a heap block is freed
//   onAllocateString — called when a string is allocated
//   onFreeString     — called when a string is freed
//
// These callbacks enable:
//   - Memory leak detection (track unmatched alloc/free pairs)
//   - Allocation profiling (measure total bytes, allocation counts)
//   - Memory budget enforcement (cap allocations per frame/phase)
//
// The callbacks fire for the instrumented context only.  Enable
// tracking with `instrument_context_allocations(ctx, true)` and
// disable with `instrument_context_allocations(ctx, false)`.
//
// Each callback receives a `Context` reference and a `LineInfo`
// telling you where in the source code the allocation happened.
//
// Important: allocation callbacks are dispatched to the
// *thread-local* debug agent, so you must install with
// `install_new_thread_local_debug_agent` — not the global
// `install_new_debug_agent`.
//
// Run: daslang.exe examples/debugapi/allocation_tracking.das

options gen2
options persistent_heap = true
options debugger = true

require daslib/rtti
require daslib/debugger
require strings

// ============================================================
// A simple allocation tracker
// ============================================================
// Logs every heap and string allocation with source location.
// Tracks counts and total bytes for a summary at the end.

class AllocationTracker : DapiDebugAgent {
    heap_allocs : int = 0
    heap_frees : int = 0
    total_bytes : uint64 = uint64(0)
    string_allocs : int = 0
    string_frees : int = 0

    def override onAllocate(var ctx : Context; data : void?; size : uint64;
                            at : LineInfo) : void {
        heap_allocs++
        total_bytes += size
        print("  [alloc]   {size} bytes at {describe(at)}\n")
    }

    def override onReallocate(var ctx : Context; data : void?; size : uint64;
                              newData : void?; newSize : uint64; at : LineInfo) : void {
        total_bytes += newSize - size
        print("  [realloc] {size} -> {newSize} bytes at {describe(at)}\n")
    }

    def override onFree(var ctx : Context; data : void?; at : LineInfo) : void {
        heap_frees++
        print("  [free]    at {describe(at)}\n")
    }

    def override onAllocateString(var ctx : Context; data : void?; size : uint64;
                                  tempString : bool; at : LineInfo) : void {
        string_allocs++
        let str = escape(unsafe(reinterpret<string> data))
        let kind = tempString ? "temp" : "heap"
        print("  [str+]    ({kind}) '{str}' {size} bytes at {describe(at)}\n")
    }

    def override onFreeString(var ctx : Context; data : void?; tempString : bool;
                              at : LineInfo) : void {
        string_frees++
    }

    def print_summary() {
        print("\n  === allocation summary ===\n")
        print("  heap:   {heap_allocs} allocs, {heap_frees} frees, {total_bytes} bytes total\n")
        print("  string: {string_allocs} allocs, {string_frees} frees\n")
    }
}

// ============================================================
// Agent installation
// ============================================================
// Thread-local agent — allocation callbacks are dispatched to
// thread-local agents only, not global named agents.
// We also install as a named global agent so we can access
// the agent context later via `get_debug_agent_context`.

var tracker : AllocationTracker?

[export]
def install_tracker(ctx : Context) {
    assert(this_context().category.debug_context)
    var agent = new AllocationTracker()
    tracker = agent
    // Thread-local: receives allocation callbacks
    install_new_thread_local_debug_agent(agent)
    // Named global: allows `get_debug_agent_context` access
    install_new_debug_agent(new DapiDebugAgent(), "alloc_tracker")
}

// Helper invoked in the agent context to print the summary
[export, pinvoke]
def print_tracker_summary() {
    if (tracker != null) {
        tracker.print_summary()
    }
}

// ============================================================
// Application code that produces allocations
// ============================================================

struct Particle {
    x : float
    y : float
    name : string
}

def do_allocations() {
    // Array push triggers heap allocations as the array grows
    var arr : array<int>
    arr |> reserve(10)
    for (i in range(10)) {
        arr |> push(i)
    }
    delete arr

    // String interpolation allocates on the string heap
    var greeting = "hello world #{42}"
    print("  computed: {greeting}\n")

    // `new` allocates a struct on the heap
    var p = new Particle(x = 1.0, y = 2.0, name = "spark")
    unsafe {
        delete p
    }
}

// ============================================================
// Main — demonstrates the full workflow
// ============================================================

[export]
def main() {
    print("=== allocation tracking ===\n\n")

    // 1. Install the tracker agent (thread-local)
    fork_debug_agent_context(@@install_tracker)

    // 2. Enable allocation instrumentation for this context
    instrument_context_allocations(this_context(), true)

    print("--- tracked allocations ---\n")
    do_allocations()
    print("--- end tracked ---\n")

    // 3. Disable tracking
    instrument_context_allocations(this_context(), false)

    // These allocations are NOT tracked (instrumentation is off)
    print("\n(tracking disabled -- these allocations are silent)\n")
    var silent : array<int>
    silent |> push(1)
    delete silent

    // 4. Print summary — tracker lives in the agent context,
    //    so we invoke our helper there via the named agent
    unsafe {
        invoke_in_context(get_debug_agent_context("alloc_tracker"),
                          "print_tracker_summary")
    }

    delete_debug_agent_context("alloc_tracker")
}
