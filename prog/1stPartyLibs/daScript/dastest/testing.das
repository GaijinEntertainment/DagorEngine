options gen2
options indenting = 4

module testing shared

require daslib/rtti
require daslib/math_bits
require math
require daslib/unroll

class Asserter {
    def abstract const errorAt(msg : string; at : LineInfo) : void

    def abstract const fatalAt(msg : string; at : LineInfo) : void
}

class T : Asserter {
    name : string
    startTick : int64
    verbose : bool
    onLog : lambda<(msg : string; at : LineInfo) : void>
    onFail : lambda<(now : bool) : void>
    onSkipNow : lambda<() : void>
    onRun : lambda<(name : string; fn : RunT) : void>

    failed : bool = false
    skipped : bool = false

    def T(name_ : string; start_tick : int64; verbose_ : bool) {
        name = name_
        startTick = start_tick
        verbose = verbose_
    }

    def const failNow() {
        onFail |> invoke(true)
        panic("test failed")
    }

    def const fail() {
        onFail |> invoke(false)
    }

    def const error(msg : string) {
        errorAt(msg, get_line_info(1))
    }

    def override const errorAt(msg : string; at : LineInfo) {
        logAt(msg, at)
        fail()
    }

    def const fatal(msg : string) {
        fatalAt(msg, get_line_info(1))
    }

    def override const fatalAt(msg : string; at : LineInfo) {
        logAt(msg, at)
        failNow()
    }

    def const log(msg : string) {
        logAt(msg, get_line_info(1))
    }

    def const logAt(msg : string; at : LineInfo) {
        onLog |> invoke(msg, at)
    }

    def const skipNow() {
        onSkipNow |> invoke()
        panic("test skipped")
    }

    def const skip(msg : string) {
        skipAt(msg, get_line_info(1))
    }

    def const skipAt(msg : string; at : LineInfo) {
        logAt(msg, at)
        skipNow()
    }

    def const getTimeUsec() : int {
        return get_time_usec(startTick)
    }

    def const getTimeSec() : double {
        return double(getTimeUsec()) / 1000000.0lf
    }
}

let private BENCH_UNROLL = 10
let private BENCH_UNROLL_MIN_BATCH = 100 * BENCH_UNROLL

struct private AllocSnapshot {
    allocs : uint64
    heap_bytes : uint64
    string_allocs : uint64
    string_heap_bytes : uint64
}

def private alloc_snapshot() : AllocSnapshot {
    return AllocSnapshot(
        allocs = heap_allocation_count(),
        heap_bytes = heap_allocation_stats()[0],
        string_allocs = string_heap_allocation_count(),
        string_heap_bytes = string_heap_allocation_stats()[0])
}

class B : Asserter {
    name : string
    current_sub_name : string
    started : bool = false
    finished : bool = false

    on_fail : lambda<(now : bool) : void>
    //! called after the benchmark's first batch, so a benchmark that fails fast never announces its name
    on_start : lambda<(sub_name : string) : void>
    on_finished : lambda<(stats : BenchmarkRunStats) : void>
    on_log : lambda<(msg : string; at : LineInfo) : void>

    failed : bool = false

    def const failNow() {
        on_fail |> invoke(true)
        panic("benchmark failed")
    }

    def const fail() {
        on_fail |> invoke(false)
    }

    def override const errorAt(msg : string; at : LineInfo) {
        logAt(msg, at)
        fail()
    }

    def override const fatalAt(msg : string; at : LineInfo) {
        logAt(msg, at)
        failNow()
    }

    def const logAt(msg : string; at : LineInfo) {
        on_log |> invoke(msg, at)
    }
}

// BenchmarkRunStats is a structured result of a single benchmark executed.
// A value of this type is serialized when json output format is used.
struct BenchmarkRunStats {
    name : string
    sub_name : string
    n : int
    time_ns : int64
    allocs : uint64
    heap_bytes : uint64
    string_allocs : uint64
    string_heap_bytes : uint64

    func_type : string = "INTERP"
}

def equal(tb : Asserter?; a; b; msg = "") : bool {
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->errorAt("\tgot: {b}", get_line_info(1))
    return false
}

def equal(tb : Asserter?; a, b : array<auto(TT)>; msg = "") : bool {
    if (length(a) != length(b)) {
        tb->errorAt(!empty(msg) ? msg : "array lengths differ", get_line_info(1))
        tb->errorAt("\texpected length: {length(a)}", get_line_info(1))
        tb->errorAt("\tgot length: {length(b)}", get_line_info(1))
        return false
    }
    for (i in range(length(a))) {
        if (!_::equal(tb, a[i], b[i])) {
            tb->errorAt(!empty(msg) ? msg : "arrays differ at [{i}]", get_line_info(1))
            tb->errorAt("\texpected[{i}]: {a[i]}", get_line_info(1))
            tb->errorAt("\tgot[{i}]: {b[i]}", get_line_info(1))
            return false
        }
    }
    return true
}

[sideeffects]
def accept(tb : Asserter?; value) {
    pass
}

def strictEqual(tb : Asserter?; a; b; msg = "") : bool {
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float2; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float3; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float4; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}


def strictEqual(tb : Asserter?; a, b : double; msg = "") : bool {
    if (double_bits_to_int64(a) == double_bits_to_int64(b)) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a; b; msg = "") : bool {
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a, b : float; msg = "") : bool {
    if ((is_nan(a) && is_nan(b)) || a == b) return true
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; _a, _b : float2; msg = "") : bool {
    var a = _a
    var b = _b
    if (is_nan(a.x) && is_nan(b.x)) {
        a.x = 0.
        b.x = 0.
    }
    if (is_nan(a.y) && is_nan(b.y)) {
        a.y = 0.
        b.y = 0.
    }
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {_a}", get_line_info(1))
    tb->fatalAt("\tgot: {_b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; _a, _b : float3; msg = "") : bool {
    var a = _a
    var b = _b
    if (is_nan(a.x) && is_nan(b.x)) {
        a.x = 0.
        b.x = 0.
    }
    if (is_nan(a.y) && is_nan(b.y)) {
        a.y = 0.
        b.y = 0.
    }
    if (is_nan(a.z) && is_nan(b.z)) {
        a.z = 0.
        b.z = 0.
    }
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {_a}", get_line_info(1))
    tb->fatalAt("\tgot: {_b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; _a, _b : float4; msg = "") : bool {
    var a = _a
    var b = _b
    if (is_nan(a.x) && is_nan(b.x)) {
        a.x = 0.
        b.x = 0.
    }
    if (is_nan(a.y) && is_nan(b.y)) {
        a.y = 0.
        b.y = 0.
    }
    if (is_nan(a.z) && is_nan(b.z)) {
        a.z = 0.
        b.z = 0.
    }
    if (is_nan(a.w) && is_nan(b.w)) {
        a.w = 0.
        b.w = 0.
    }
    if (a == b) return true
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {_a}", get_line_info(1))
    tb->fatalAt("\tgot: {_b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a, b : double; msg = "") : bool {
    if ((is_nan(a) && is_nan(b)) || a == b) return true
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def success(tb : Asserter?; a; msg = "") : bool {
    if (a) return true
    tb->errorAt(!empty(msg) ? msg : "expected success, got failure", get_line_info(1))
    return false
}

def failure(tb : Asserter?; msg = "") : bool {
    tb->errorAt(!empty(msg) ? msg : "test failed", get_line_info(1))
    return false
}

//! Skip the current test arm: prints `msg` and lands the arm in the suite's skipped count (not
//! a pass). Does NOT unwind — use the arm-fatal shape `if (gate) { t |> skip("..."); return }`.
//! Deliberately avoids both T.skipNow (its panic from AOT-compiled test frames corrupted state
//! at volume) and onLog (the log handler grows dastest-owned arrays under the TEST context —
//! a cross-context allocation that corrupted the heap at skip volume; skips don't need the
//! failure-report message capture, so the reason prints directly).
def skip(t : T?; msg : string) {
    to_log(LOG_INFO, "\tskipped: {msg}\n")
    t.onSkipNow |> invoke()
}

typedef RunT = variant<lmd1 : lambda<(t : T?) : void>; func1 : function<(t : T?) : void>>


def run(t : T?; name : string; var cb : lambda<(t : T?) : void>) {
    t.onRun |> invoke(name, RunT(lmd1 <- cb))
}


def run(t : T?; name : string; func : function<(t : T?) : void>) {
    t.onRun |> invoke(name, RunT(func1 = func))
}


def run(b : B?, name : string; op : block) {
    //! run executes a benchmark of a given name and runs the block in the loop,
    //! while collecting data like CPU time and heap allocations.
    //! It assumes that every op invocation performs a single logical operation.
    //! If op is a combined operation, consider using an overloaded version
    //! with chunk_size parameter.
    run_impl(b, name, 1l, op)
}


def run(b : B?, name : string; chunk_size : int; op : block) {
    //! This overloading allows a custom chunk size configuration
    //! to adjust the ns/op to it.
    run_impl(b, name, int64(chunk_size), op)
}


[arch(at="README.md#benchmark-measurement")]
def private run_batch_ns(num_runs : int; op : block) : int64 {
    if (num_runs >= BENCH_UNROLL_MIN_BATCH) {
        let loop_runs = num_runs / BENCH_UNROLL
        let start = ref_time_ticks()
        for (_i in range(loop_runs)) {
            unroll() $ {
                for (_ in range(BENCH_UNROLL)) {
                    op()
                }
            }
        }
        return get_time_nsec(start)
    }
    var time_total = 0l
    for (_i in range(num_runs)) {
        let start = ref_time_ticks()
        op()
        time_total += get_time_nsec(start)
    }
    return time_total
}

[arch(at="README.md#benchmark-measurement")]
def private run_impl(b : B?, name : string; chunk_size : int64; op : block) {
    let budget_ns = 1_000_000_000l
    let max_runs = 1_000_000_000l
    var num_runs = 1
    var time_total = 0l
    var name_announced = false
    var before : AllocSnapshot
    var after : AllocSnapshot
    while (true) {
        before = alloc_snapshot()
        time_total = run_batch_ns(num_runs, op)
        after = alloc_snapshot()
        if (!name_announced) {
            b.on_start(name)
            name_announced = true
        }
        if (time_total >= budget_ns || int64(num_runs) >= max_runs) {
            break
        }
        let per_run_ns = max(double(time_total) / double(num_runs), 1.0lf)
        var next = int64(double(budget_ns) / per_run_ns * 1.2lf)
        next = min(next, 100l * int64(num_runs))
        next = max(next, int64(num_runs) + 1l)
        next = min(next, max_runs)
        if (next >= int64(BENCH_UNROLL_MIN_BATCH)) {
            next = (next / int64(BENCH_UNROLL)) * int64(BENCH_UNROLL)
        }
        num_runs = int(next)
    }

    var stats = BenchmarkRunStats(
        name = b.name,
        sub_name = name,
        n = num_runs,
        time_ns = time_total,
        allocs = after.allocs - before.allocs,
        heap_bytes = after.heap_bytes - before.heap_bytes,
        string_allocs = after.string_allocs - before.string_allocs,
        string_heap_bytes = after.string_heap_bytes - before.string_heap_bytes,
    )

    if (chunk_size > 1l) {
        stats.time_ns = max(stats.time_ns / chunk_size, 1l)
        if (stats.allocs > 0ul) {
            stats.allocs = max(stats.allocs / uint64(chunk_size), 1ul)
        }
        if (stats.heap_bytes > 0ul) {
            stats.heap_bytes = max(stats.heap_bytes / uint64(chunk_size), 1ul)
        }
    }

    b.on_finished(stats)
}
