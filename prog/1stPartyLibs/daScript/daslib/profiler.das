options gen2
options indenting = 4
options unsafe_table_lookup = false
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true
options threadlock_context

module profiler shared private

//! Instrumenting profiler for daslang programs.
//!
//! Attaches a debug agent that records per-function timing and optional heap-usage
//! events. Results can be saved as Chrome-compatible trace JSON for visualization
//! in ``chrome://tracing`` or Perfetto.
//! See also ``profiler_boost`` for cross-context profiler control.

require daslib/debugger
require daslib/fio
require strings
require uriparser
require daslib/ast

struct PerfNode {
    //! Performance tree node with timing data.
    fun : SimFunction?
    count : uint64
    total_time : int64
    enter_time : int64
    total_heap_bytes : uint64
    enter_heap_bytes : uint64
    total_string_heap_bytes : uint64
    enter_string_heap_bytes : uint64
    children : table<SimFunction?; PerfNode?>
}

struct PerfEvent {
    //! Recorded performance event with mangled name.
    fun : SimFunction?
    ts : int64
    entering : bool
    heapBytes : uint64
    stringHeapBytes : uint64
}

struct PerfContext {
    //! Per-context profiling data with event history.
    events : array<PerfEvent>
    enabled : bool
    silent : bool = false
}

struct FlatEntry {
    //! Aggregate per-function own-memory stats for flat top-offender dumps.
    fun : SimFunction? = null
    count : uint64 = 0ul
    own_heap : int64 = 0l
    own_string_heap : int64 = 0l
}

struct ProfilerConfig {
    //! Parsed command-line configuration for the profiler.
    log_file : string = ""
    manual : bool = false
    report_memory : bool = false
    time_unit : string = "ns"
    use_thread_local : bool = true
    track_leaks : bool = false
}

var map_time_units : table<string; int64> <- {"ns" => 1l, "us" => 1_000l, "ms" => 1_000_000l, "s" => 1_000_000_000l}

def is_time_unit_correct(unit : string) {
    return key_exists(map_time_units, unit)
}

def convert_ns_to_unit(time : int64; to : string) {
    return time / map_time_units[to]
}

def parse_profiler_args() : ProfilerConfig {
    //! Parses daslang profiler command-line flags into a :ref:`struct ProfilerConfig`.
    //!
    //! Recognizes ``--das-profiler-log-file <path>``, ``--das-profiler-manual``,
    //! ``--das-profiler-memory``, ``--das-profiler-time-unit <ns|us|ms|s>``,
    //! ``--das-profiler-thread-local`` / ``--das-profiler-global``, and
    //! ``--das-profiler-leaks`` (memory-leak tracking mode, mutually exclusive
    //! with the performance profiler).
    var cfg = default<ProfilerConfig>
    var force_thread_local = -1
    var args <- get_command_line_arguments()
    for (argv, i in args, count()) {
        if (argv == "--das-profiler-log-file" && empty(cfg.log_file) && ((i + 1) < length(args))) {
            cfg.log_file = args[i + 1]
        } elif (argv == "--das-profiler-manual") {
            cfg.manual = true
        } elif (argv == "--das-profiler-memory") {
            cfg.report_memory = true
        } elif (argv == "--das-profiler-thread-local") {
            force_thread_local = 1
        } elif (argv == "--das-profiler-global") {
            force_thread_local = 0
        } elif (argv == "--das-profiler-leaks") {
            cfg.track_leaks = true
        } elif (argv == "--das-profiler-time-unit" && ((i + 1) < length(args))) {
            let unit = args[i + 1]
            if (!is_time_unit_correct(unit)) {
                to_log(LOG_ERROR, "Time unit '{unit}' is not defined!")
            } else {
                cfg.time_unit = unit
            }
        }
    }
    if (cfg.track_leaks) {
        cfg.use_thread_local = false   // leak agent is a named singleton
    } else {
        cfg.use_thread_local = force_thread_local == -1 ? !cfg.report_memory : (force_thread_local == 1)
    }
    return <- cfg
}

def profiler_category(cfg : ProfilerConfig) : string {
    if (cfg.track_leaks) {
        return "memleaks"
    }
    return cfg.use_thread_local ? "" : "profiler"
}

def public set_enable_profiler(ctxId : uint64, enabled : bool) {
    //! Enables or disables the profiler for the given context ID. No-op in leak mode.
    let cfg <- parse_profiler_args()
    if (cfg.track_leaks) {
        return
    }
    unsafe {
        invoke_debug_agent_method(profiler_category(cfg), "enable_profiler", ctxId, enabled)
    }
}

def public set_enable_profiler_log(ctxId : uint64, enabled : bool) {
    //! Enables or disables the profiler log for the given context ID. No-op in leak mode.
    let cfg <- parse_profiler_args()
    if (cfg.track_leaks) {
        return
    }
    unsafe {
        invoke_debug_agent_method(profiler_category(cfg), "enable_profiler_log", ctxId, enabled)
    }
}

class ProfilerBaseAgent : DapiDebugAgent {
    //! Shared base for the performance and memory-leak profiler agents.
    //! Owns state and helpers common to both concrete agents: the per-thread
    //! vs. singleton install mode flag, the per-code-allocator instrumentation
    //! dedup table, and the :ref:`isProfileable` / :ref:`ensure_instrumented`
    //! helpers. Subclasses provide their own ``onInstall`` / ``onCreateContext``
    //! / ``onDestroyContext`` and call ``ensure_instrumented`` from their
    //! ``onCreateContext``.
    use_thread_local : bool = true
    instrumented : table<uint64>

    def isProfileable(var ctx : Context) {
        return !(ctx.category.debug_context || ctx.category.macro_context || ctx.category.folding_context ||
            ctx.category.debugger_tick || ctx.category.debugger_attached)
    }

    def ensure_instrumented(var ctx : Context) {
        if (!(instrumented |> key_exists(ctx.getCodeAllocatorId))) {
            instrumented |> insert(ctx.getCodeAllocatorId)
            if (use_thread_local) {
                instrument_all_functions_thread_local(ctx)
            } else {
                instrument_all_functions(ctx)
            }
        }
    }
}

class PerformanceProfilerAgent : ProfilerBaseAgent {
    //! Debug agent that collects function-level timing and optional heap
    //! accounting data, and dumps a Chrome-compatible JSON trace.
    us0 : int64 = 0l
    events : table<uint64; PerfContext>
    out : FILE const?
    firstRecord : bool = true
    manual : bool = false
    report_memory : bool = false
    time_unit : string = "ns"

    def override onInstall(agent : DebugAgent?) : void {
        this_context().name := "__PROFILER__"
        let cfg <- parse_profiler_args()
        if (!empty(cfg.log_file)) {
            out = fopen(cfg.log_file, "wb")
        }
        manual = cfg.manual
        report_memory = cfg.report_memory
        use_thread_local = cfg.use_thread_local
        time_unit = cfg.time_unit
        if (report_memory) {
            unsafe {
                self.onInstrumentFunction = reinterpret<function<(var agent : DapiDebugAgent; var ctx : Context; fn : SimFunction?; entering : bool; userData : uint64) : void>> self.onInstrumentFunctionWithMemory
            }
        }
        write("[")
        us0 = ref_time_ticks()
    }
    def override onUninstall(agent : DebugAgent?) : void {
        if (out != null) {
            write("]")
            out |> fclose()
            out = null
        }
    }
    def override onCreateContext(var ctx : Context) : void {
        if (!isProfileable(ctx)) {
            return
        }
        let tid = intptr(unsafe(addr(ctx)))
        events[tid].enabled = !manual
        // note: silent defaults to false. In manual mode the user flips
        // `enabled` via set_enable_profiler; the dump on destroy is controlled
        // separately by set_enable_profiler_log.
        ensure_instrumented(ctx)
    }
    def enable_profiler(ctxId : uint64; enabled : bool) : void {
        events[ctxId].enabled = enabled
    }
    def enable_profiler_log(ctxId : uint64; enabled : bool) : void {
        events[ctxId].silent = !enabled
    }
    def override onDestroyContext(var ctx : Context) : void {
        if (!isProfileable(ctx)) {
            return
        }
        let tid = intptr(unsafe(addr(ctx)))
        if (events |> key_exists(tid)) {
            if (!events[tid].silent) {
                dump_context(ctx, tid)
            }
            delete events[tid]
        }
        instrumented |> erase(ctx.getCodeAllocatorId)
    }
    def onInstrumentFunctionWithMemory(var ctx : Context; var fun : SimFunction?; entering : bool; userData : uint64) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            var ev & = events[tid]
            if (ev.enabled) {
                var hb_shb : uint64[2]
                unsafe(get_heap_stats(ctx, unsafe(addr(hb_shb[0]))))
                ev.events |> push(PerfEvent(
                    fun = fun,
                    entering = entering,
                    ts = get_time_nsec(us0),
                    heapBytes = hb_shb[0],
                    stringHeapBytes = hb_shb[1]))
            }
        }
    }
    def override onInstrumentFunction(var ctx : Context; var fun : SimFunction?; entering : bool; userData : uint64) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            var ev & = events[tid]
            if (ev.enabled) {
                ev.events |> push(PerfEvent(
                    fun = fun,
                    entering = entering,
                    ts = get_time_nsec(us0)))
            }
        }
    }
    def dump(text : string) {
        if (firstRecord) {
            firstRecord = false
        } else {
            write(",\n")
        }
        write(text)
    }
    def write(text : string) {
        if (out != null) {
            out |> fprint(text)
        }
    }
    def dump_event(ev : PerfEvent; tid : uint64) {
        let phase = ev.entering ? "B" : "E"
        let fnName = ev.fun.mangledName |> escape()
        dump("\{\"name\":\"{fnName}\",\"cat\": \"PERF\",\"ph\":\"{phase}\",\"pid\": 0,\"tid\":{int64(tid)},\"ts\":{ev.ts/1000l}\}")
    }
    def dump_node(node : PerfNode?; tab : int = -1) {
        if (node == null) {
            return
        }
        if (report_memory && node.fun != null && node.total_heap_bytes == 0ul && node.total_string_heap_bytes == 0ul) {
            return
        }
        if (node.fun != null) {
            let tabs = repeat("  ", tab)
            if (report_memory) {
                var hb = node.total_heap_bytes
                var shb = node.total_string_heap_bytes
                for (ch in values(node.children)) {
                    hb -= ch.total_heap_bytes
                    shb -= ch.total_string_heap_bytes
                }
                to_log(LOG_INFO, "{tabs}{node.fun.mangledName} {int64(node.count)} heap={int64(hb)} string_heap={int64(shb)}\n")
            } else {
                let total_time = convert_ns_to_unit(node.total_time, time_unit)
                to_log(LOG_INFO, "{tabs}{node.fun.mangledName} {int64(node.count)} {total_time}{time_unit}\n")
            }
        }
        for (ch in values(node.children)) {
            dump_node(ch, tab + 1)
        }
    }
    def collect_flat(var node : PerfNode?; var flat : table<SimFunction?; FlatEntry>) {
        if (node == null) {
            return
        }
        if (node.fun != null) {
            var own_hb = int64(node.total_heap_bytes)
            var own_shb = int64(node.total_string_heap_bytes)
            for (ch in values(node.children)) {
                own_hb -= int64(ch.total_heap_bytes)
                own_shb -= int64(ch.total_string_heap_bytes)
            }
            unsafe {
                var e & = flat[node.fun]
                e.fun = node.fun
                e.own_heap += own_hb
                e.own_string_heap += own_shb
                e.count += node.count
            }
        }
        for (ch in values(node.children)) {
            collect_flat(ch, flat)
        }
    }
    def dump_top_heap_offenders(var flat : table<SimFunction?; FlatEntry>) {
        var entries : array<FlatEntry>
        for (e in values(flat)) {
            if (e.own_heap > 0l) {
                entries |> push(e)
            }
        }
        if (empty(entries)) {
            return
        }
        entries |> sort() <| $(a, b) => a.own_heap > b.own_heap
        to_log(LOG_INFO, "\nTop heap offenders (own allocations):\n")
        var shown = 0
        for (e in entries) {
            if (shown >= 10) {
                break
            }
            shown ++
            to_log(LOG_INFO, "  {e.fun.mangledName} heap={int64(e.own_heap)} count={int64(e.count)}\n")
        }
    }
    def dump_top_string_heap_offenders(var flat : table<SimFunction?; FlatEntry>) {
        var entries : array<FlatEntry>
        for (e in values(flat)) {
            if (e.own_string_heap > 0l) {
                entries |> push(e)
            }
        }
        if (empty(entries)) {
            return
        }
        entries |> sort() <| $(a, b) => a.own_string_heap > b.own_string_heap
        to_log(LOG_INFO, "\nTop string-heap offenders (own allocations):\n")
        var shown = 0
        for (e in entries) {
            if (shown >= 10) {
                break
            }
            shown ++
            to_log(LOG_INFO, "  {e.fun.mangledName} string_heap={int64(e.own_string_heap)} count={int64(e.count)}\n")
        }
    }
    def dump_context_stack(tid : uint64) {
        // print("context: {tid} // {ctx.name} {ctx.category}\n")
        var root = new PerfNode()
        var stack : array<PerfNode?>
        var sp = root
        for (ev in events[tid].events) {
            if (manual && ev.fun.mangledName |> starts_with("@profiler_boost::")) {
                continue
            }
            if (ev.entering) {
                stack |> push(sp)
                if (!sp.children |> key_exists(ev.fun)) {
                    sp.children[ev.fun] = new PerfNode(fun = ev.fun)
                }
                sp = sp.children[ev.fun]
                sp.count ++
                sp.enter_time = ev.ts
                sp.enter_heap_bytes = ev.heapBytes
                sp.enter_string_heap_bytes = ev.stringHeapBytes
            } else {
                sp.total_time += ev.ts - sp.enter_time
                sp.total_heap_bytes += ev.heapBytes - sp.enter_heap_bytes
                sp.total_string_heap_bytes += ev.stringHeapBytes - sp.enter_string_heap_bytes
                sp = length(stack) != 0 ? stack |> back() : root
                if (length(stack) > 0) {// protection for mismatching enable\disable
                    stack |> pop()
                }
            }
        }
        dump_node(root)
        if (report_memory) {
            var flat : table<SimFunction?; FlatEntry>
            collect_flat(root, flat)
            dump_top_heap_offenders(flat)
            dump_top_string_heap_offenders(flat)
        }
        unsafe {
            delete root
        }
    }
    def dump_meta(var ctx : Context; tid : uint64) {
        let ctxName = empty(ctx.name) ? "" : " '{ctx.name}'"
        let wasDead = (ctx.category & context_category_flags.dead) == context_category_flags.dead
        ctx.category &= ~context_category_flags.dead
        let ctxDesc = "{tid}{ctxName} cat:{ctx.category}" |> escape()
        dump("\{\"args\": \{ \"name\": \"{ctxDesc}\" \}, \"name\": \"thread_name\", \"cat\": \"__metadata\", \"ph\": \"M\", \"pid\": 0, \"tid\": {int64(tid)}, \"ts\": 0 \}")
        if (wasDead) { ctx.category |= context_category_flags.dead; }
    }
        // dump("\{\"args\": \{ \"value\" : \"ns\" \}, \"name\": \"displayTimeUnit\", \"cat\": \"__metadata\", \"ph\": \"M\", \"pid\": 0, \"tid\": {int64(tid)}, \"ts\": 0 \}")
    def dump_events(tid : uint64) {
        for (ev in events[tid].events) {
            dump_event(ev, tid)
        }
    }
    def dump_context(var ctx : Context; tid : uint64) {
        dump_context_stack(tid)
        dump_meta(ctx, tid)
        dump_events(tid)
        unsafe {
            delete events[tid].events
        }
        events |> erase(tid)
    }
}

struct AllocationRecord {
    //! One live heap allocation tracked by :ref:`class MemoryLeakProfilerAgent`.
    //! ``stack`` is a snapshot of the per-context shadow call stack at
    //! allocation time, leaf frame last (same push order as call entry).
    size : uint64 = 0ul
    stack : array<SimFunction?>
}

struct LeakContext {
    //! Per-instrumented-context leak-tracking state.
    shadow_stack : array<SimFunction?>
    allocations : table<uint64; AllocationRecord>
}

class MemoryLeakProfilerAgent : ProfilerBaseAgent {
    //! Debug agent that tracks live heap allocations and dumps leaks with
    //! captured daslang call stacks on context destroy. Installed as a
    //! singleton named ``"memleaks"`` agent so it observes allocations from
    //! every instrumented context in a single bookkeeping table.
    ctxs : table<uint64; LeakContext>
    out : FILE const?

    def override onInstall(agent : DebugAgent?) : void {
        this_context().name := "__MEMLEAK__"
        let cfg <- parse_profiler_args()
        if (!empty(cfg.log_file)) {
            out = fopen(cfg.log_file, "wb")
        }
        use_thread_local = false   // named/global install -> non-thread-local function instrumentation
    }
    def override onUninstall(agent : DebugAgent?) : void {
        if (out != null) {
            out |> fclose()
            out = null
        }
    }
    def override onCreateContext(var ctx : Context) : void {
        if (!isProfileable(ctx)) {
            return
        }
        let tid = intptr(unsafe(addr(ctx)))
        unsafe {
            var lc & = ctxs[tid]
            lc.shadow_stack |> clear()
        }
        ensure_instrumented(ctx)
        instrument_context_allocations(ctx, true)
    }
    def override onDestroyContext(var ctx : Context) : void {
        if (!isProfileable(ctx)) {
            return
        }
        let tid = intptr(unsafe(addr(ctx)))
        instrument_context_allocations(ctx, false)
        if (ctxs |> key_exists(tid)) {
            dump_leaks(ctx, tid)
            delete ctxs[tid]
        }
        // code-allocator ids are globally unique; keep them in `instrumented` so we never re-scan the same program
    }
    def override onInstrumentFunction(var ctx : Context; var fun : SimFunction?; entering : bool; userData : uint64) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            if (!(ctxs |> key_exists(tid))) {
                return
            }
            var lc & = ctxs[tid]
            if (entering) {
                lc.shadow_stack |> push(fun)
            } elif (length(lc.shadow_stack) > 0) {
                lc.shadow_stack |> pop()
            }
        }
    }
    def override onAllocate(var ctx : Context; data : void?; size : uint64; at : LineInfo) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            if (!(ctxs |> key_exists(tid))) {
                return
            }
            var lc & = ctxs[tid]
            var rec & = lc.allocations[intptr(data)]
            rec.size = size
            rec.stack := lc.shadow_stack
        }
    }
    def override onFree(var ctx : Context; data : void?; at : LineInfo) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            if (!(ctxs |> key_exists(tid))) {
                return
            }
            var lc & = ctxs[tid]
            lc.allocations |> erase(intptr(data))
        }
    }
    def override onReallocate(var ctx : Context; data : void?; size : uint64; newData : void?; newSize : uint64; at : LineInfo) : void {
        unsafe {
            let tid = intptr(addr(ctx))
            if (!(ctxs |> key_exists(tid))) {
                return
            }
            var lc & = ctxs[tid]
            lc.allocations |> erase(intptr(data))
            var rec & = lc.allocations[intptr(newData)]
            rec.size = newSize
            rec.stack := lc.shadow_stack
        }
    }
    def emit_line(text : string) {
        if (out != null) {
            out |> fprint("{text}\n")
        } else {
            to_log(LOG_INFO, "{text}\n")
        }
    }
    def format_frame(fn : SimFunction?) : string {
        if (fn == null) {
            return "  at ?"
        }
        let fname = fn.name
        let li = fn.lineInfo
        if (li != null && li.fileInfo != null && !empty(li.fileInfo.name)) {
            return "  at {fname} {li.fileInfo.name}:{int(li.line)}"
        }
        return "  at {fname}"
    }
    def dump_leaks(var ctx : Context; tid : uint64) {
        unsafe {
            var lc & = ctxs[tid]
            if (empty(lc.allocations)) {
                return
            }
            var sorted : array<tuple<uint64; uint64>>   // (size, addr-key)
            var totalBytes = 0ul
            for (k, v in keys(lc.allocations), values(lc.allocations)) {
                sorted |> push((v.size, k))
                totalBytes += v.size
            }
            sorted |> sort() <| $(a, b) => a._0 > b._0
            var ctxName = "<unnamed>"
            if (!empty(ctx.name)) {
                ctxName = "{ctx.name}"
            }
            emit_line("=== Memory leaks in context '{ctxName}' ({int(length(sorted))} allocations, {totalBytes} bytes) ===")
            for (e in sorted) {
                var rec & = lc.allocations[e._1]
                emit_line("[leak] size={rec.size} bytes")
                // innermost (last-pushed) first
                let stackLen = length(rec.stack)
                for (i in range(stackLen)) {
                    let idx = stackLen - 1 - i
                    emit_line(format_frame(rec.stack[idx]))
                }
            }
        }
    }
}

def debug_agent(ctx : Context) {
    assert(this_context().category.debug_context)
    let cfg <- parse_profiler_args()
    if (cfg.track_leaks) {
        install_new_debug_agent(new MemoryLeakProfilerAgent(), "memleaks")
    } elif (cfg.use_thread_local) {
        install_new_thread_local_debug_agent(new PerformanceProfilerAgent())
    } else {
        install_new_debug_agent(new PerformanceProfilerAgent(), "profiler")
    }
}

[_macro]
def installing {
    if (is_compiling_macros_in_module("profiler") && !is_in_completion() && !is_in_lint_check()) {
        if (!is_in_debug_agent_creation()) {
            if (!has_debug_agent_context("profiler")) {
                fork_debug_agent_context(@@debug_agent)
            }
        }
    }
}
