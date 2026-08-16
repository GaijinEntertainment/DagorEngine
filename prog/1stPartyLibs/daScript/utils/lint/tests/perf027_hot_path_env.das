options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF027: environment lookup on a hot path
//
// Problem:
//   get_env_variable calls getenv AND allocates the result in the context
//   heap, so a per-step knob read is both a syscall-ish lookup and a leak
//   into the heap until the next reset. has_env_variable does not allocate,
//   but a per-step probe still belongs at init.
//
// Bad pattern:
//   [hot_path]
//   def step() {
//       if (get_env_variable("MY_KNOB") == "1") { ... }
//   }
//
// Good pattern:
//   var private g_knob = -1
//   [cold_path]
//   def knob_value : int {
//       if (g_knob < 0) { g_knob = ... }
//       return g_knob
//   }
//
// See perf026_hot_path_alloc.das for how the annotation-gated scan works.

expect 31208:2

require daslib/perf_lint
require daslib/fio

// 1. lookup written directly in the annotated function
[no_env]
def bad_direct() : bool {
    return get_env_variable("DAS_LINT_FIXTURE_KNOB") == "1"
}

// 2. lookup reached through an ordinary helper
def env_helper() : bool {
    return has_env_variable("DAS_LINT_FIXTURE_KNOB")
}

[no_env]
def bad_transitive() : bool {
    return env_helper()
}

// 3. [cold_path] is the intended fix: read once, behind a pruned helper
[cold_path]
def cold_knob() : bool {
    return get_env_variable("DAS_LINT_FIXTURE_KNOB") == "1"
}

[no_env]
def good_cold() : bool {
    return cold_knob()
}

// 4. per-line suppression still works
[no_env]
def good_nolint() : bool {
    return get_env_variable("DAS_LINT_FIXTURE_KNOB") == "1"   // nolint:PERF027
}

// 5. an unannotated function is never scanned
def good_unannotated() : bool {
    return get_env_variable("DAS_LINT_FIXTURE_KNOB") == "1"
}

// 6. [no_env] ignores allocation and I/O — those are PERF026 / PERF028
[no_env]
def good_other_contracts(var a : array<int>) {
    a |> push(1)
    print("not an env read\n")
}
