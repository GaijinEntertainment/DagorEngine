options gen2
// PERF006 regression: an array comprehension is NOT a hand-written push loop.
//
// A comprehension lowers to accumulator + for + push, and auto-inline (which
// runs BEFORE the lint pass) splices that shape straight into the caller. The
// result looks exactly like `for (...) { arr |> push(x) }` with no reserve, but
// the destination is a compiler-generated variable and there is no reserve()
// the author could add. PERF006 skips generated destinations for that reason.
//
// Deliberately does NOT set `options auto_inline_functions = false` — the
// splice is the whole point of this fixture, so inlining must stay ON.
//
// This file must compile CLEAN. Before the fix it produced two PERF006.

require daslib/perf_lint

def comp_over_range(n : int) : int {
    var ids <- [for (i in range(n)); int64(i)]
    let k = length(ids)
    delete ids
    return k
}

def comp_over_array(src : array<int>) : int {
    var ids <- [for (v in src); int64(v)]
    let k = length(ids)
    delete ids
    return k
}

// the genuine hand-written shape still warns; that case lives in
// perf006_push_without_reserve.das

[export]
def main {
    to_log(LOG_INFO, "{comp_over_range(4)} {comp_over_array([1, 2, 3])}\n")
}
