options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF028: console / file I/O on a hot path
//
// Problem:
//   A print or to_log left in a per-frame path is the debug line that never
//   got removed. It serializes on the console handle and, with an
//   interpolated message, allocates as well (that half is PERF026).
//
// Bad pattern:
//   [hot_path]
//   def step(i : int) {
//       to_log(LOG_INFO, "step {i}\n")
//   }
//
// Good pattern:
//   accumulate into a counter and report once outside the loop, or put the
//   logging behind a [cold_path] helper guarded by a debug flag.
//
// This is the rule to reach for DAS_LINT_DISABLE=PERF028 on: adding a log
// line while chasing a bug should not require editing the annotation.
//
// See perf026_hot_path_alloc.das for how the annotation-gated scan works.

expect 31208:2

require daslib/perf_lint

// 1. I/O written directly in the annotated function
[no_io]
def bad_direct() {
    print("tick\n")
}

// 2. I/O reached through an ordinary helper
def io_helper() {
    to_log(LOG_INFO, "tick\n")
}

[no_io]
def bad_transitive() {
    io_helper()
}

// 3. [cold_path] prunes the walk
[cold_path]
def cold_report() {
    print("summary\n")
}

[no_io]
def good_cold() {
    cold_report()
}

// 4. per-line suppression still works
[no_io]
def good_nolint() {
    print("tick\n")   // nolint:PERF028
}

// 5. an unannotated function is never scanned
def good_unannotated() {
    print("tick\n")
}

// 6. [no_io] ignores allocation and env reads — those are PERF026 / PERF027
[no_io]
def good_other_contracts(var a : array<int>) {
    a |> push(1)
}
