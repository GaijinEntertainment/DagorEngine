options gen2
options indenting = 4

module testing shared

require daslib/rtti
require strings // to compare strings/das_strings
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

class B : Asserter {
    name : string
    current_sub_name : string
    started : bool = false
    finished : bool = false

    on_fail : lambda<(now : bool) : void>
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
    if (a == b) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->errorAt("\tgot: {b}", get_line_info(1))
    return false
}

[sideeffects]
def accept(tb : Asserter?; value) {
    pass
}

def strictEqual(tb : Asserter?; a; b; msg = "") : bool {
    if (a == b) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float2; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float3; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def strictEqual(tb : Asserter?; a, b : float4; msg = "") : bool {
    if (float_bits_to_int(a) == float_bits_to_int(b)) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}


def strictEqual(tb : Asserter?; a, b : double; msg = "") : bool {
    if (double_bits_to_int64(a) == double_bits_to_int64(b)) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a; b; msg = "") : bool {
    if (a == b) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a, b : float; msg = "") : bool {
    if (is_nan(a) && is_nan(b)) {
        return true
    }
    if (a == b) {
        return true
    }
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
    if (a == b) {
        return true
    }
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
    if (a == b) {
        return true
    }
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
    if (a == b) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "numeric values differ", get_line_info(1))
    tb->errorAt("\texpected: {_a}", get_line_info(1))
    tb->fatalAt("\tgot: {_b}", get_line_info(1))
    return false
}

def numericEqual(tb : Asserter?; a, b : double; msg = "") : bool {
    if (is_nan(a) && is_nan(b)) {
        return true
    }
    if (a == b) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "values differ", get_line_info(1))
    tb->errorAt("\texpected: {a}", get_line_info(1))
    tb->fatalAt("\tgot: {b}", get_line_info(1))
    return false
}

def success(tb : Asserter?; a; msg = "") : bool {
    if (a) {
        return true
    }
    tb->errorAt(!empty(msg) ? msg : "expected success, got failure", get_line_info(1))
    return false
}

def failure(tb : Asserter?; msg = "") : bool {
    tb->errorAt(!empty(msg) ? msg : "test failed", get_line_info(1))
    return false
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


def private run_impl(b : B?, name : string; chunk_size : int64; op : block) {
    let min_runs = 1
    let max_runs = 1_000_000_000

    // The first run serves two purposes: it will
    // find the appropriate N (num_runs) for the op,
    // plus it allows a warmup before we start measuring it.
    let run1_start = ref_time_ticks()
    op()
    let op1_time_ns = max(double(get_time_nsec(run1_start)), 1.0lf)

    // Make on_start run after the first op execution.
    // We don't want to print the benchmark name if it is
    // going to fail fast.
    b.on_start(name)

    let op_time_approx = op1_time_ns / 1000000000.0lf // In seconds

    let n_probe = int64(1.0lf / op_time_approx)
    var num_runs = int(clamp(n_probe, int64(min_runs), int64(max_runs)))

    // We will unroll fast-running (small) benchmarks
    // to decrease the loop's own time contribution.
    // A value of 5 gives good results, 10 should be very safe.
    let unroll_count = 10
    let unroll_threshold = 100 * unroll_count

    // Pre-compute all the stuff before the timer has started.
    let do_unroll = num_runs > unroll_threshold
    var loop_runs = num_runs
    if (do_unroll) {
        num_runs = int(round(float(num_runs) / float(unroll_count)) * float(unroll_count))
        loop_runs = num_runs / unroll_count
    }

    // We could use a builtin profile() function here
    // instead of doing loops in das directly.
    // At any rate, the auto-inferred N (count) can be useful
    // and even profile() can benefit from the unrolled statements.

    let heap_stats_start = heap_allocation_stats()
    let heap_bytes_start = heap_stats_start[0]
    let num_allocs_start = heap_allocation_count()

    let string_stats_start = string_heap_allocation_stats()
    let string_bytes_start = string_stats_start[0]
    let string_num_allocs_start = string_heap_allocation_count()

    var time_total = 0l
    if (do_unroll) {
        // A fast/small benchmarking func - run the timers
        // once, unroll the body to minimize the noise.
        let start = ref_time_ticks()
        for (i in range(loop_runs)) {
            unroll <| $() {
                for (_ in range(unroll_count)) {
                    op()
                }
            }
        }
        time_total = get_time_nsec(start)
    } else {
        // A long-running benchmark that should not be unrolled.
        // Its run time will not be dominated by the loop anyway.
        for (i in range(loop_runs)) {
            let start = ref_time_ticks()
            op()
            time_total += get_time_nsec(start)
        }
    }

    let heap_stats_end = heap_allocation_stats()
    let heap_bytes_end = heap_stats_end[0]
    let num_allocs_end = heap_allocation_count()

    let string_stats_end = string_heap_allocation_stats()
    let string_bytes_end = string_stats_end[0]
    let string_num_allocs_end = string_heap_allocation_count()

    var stats = BenchmarkRunStats(
        name = b.name,
        sub_name = name,
        n = num_runs,
        time_ns = time_total,
        allocs = num_allocs_end - num_allocs_start,
        heap_bytes = heap_bytes_end - heap_bytes_start,
        string_allocs = string_num_allocs_end - string_num_allocs_start,
        string_heap_bytes = string_bytes_end - string_bytes_start,
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
