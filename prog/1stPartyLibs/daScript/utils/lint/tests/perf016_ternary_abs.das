options gen2
// PERF016: ternary abs → abs(x)
//
// Problem:
//   `x < 0 ? -x : x` reimplements `abs(x)`. abs() exists for all signed
//   numeric types and reads more clearly.
//
// Bad pattern:
//   let positive = x < 0 ? -x : x
//   let positive_alt = x > 0 ? x : -x
//
// Good pattern:
//   let positive = abs(x)
//
// `negabs(x)` (`x < 0 ? x : -x`) is NOT flagged — it's a different function.

expect 31208:6

require daslib/perf_lint
require math

// --- Bad patterns (PERF016) ---

def bad_lt_zero(x : int) : int {
    return x < 0 ? -x : x                          // PERF016
}

def bad_gt_zero(x : int) : int {
    return x > 0 ? x : -x                          // PERF016
}

def bad_le_zero(x : int) : int {
    return x <= 0 ? -x : x                         // PERF016
}

def bad_ge_zero(x : int) : int {
    return x >= 0 ? x : -x                         // PERF016
}

def bad_yoda_zero_gt_x(x : int) : int {
    return 0 > x ? -x : x                          // PERF016 (zero on left)
}

def bad_float(x : float) : float {
    return x < 0.0 ? -x : x                        // PERF016
}

// --- Good patterns (no warnings) ---

def good_abs(x : int) : int {
    return abs(x)
}

def good_negabs(x : int) : int {
    return x < 0 ? x : -x                          // negabs — NOT flagged
}

def good_unrelated(x, y : int) : int {
    return x < 0 ? -y : y                          // branches reference y, not x
}

// Side-effectful operand: `f() < 0 ? -f() : f()` calls f() up to twice and
// the two evaluations may even disagree on sign. `abs(f())` calls once. The
// rule must NOT fire when the operand isn't pure.

[unused_argument(x)]
def impure_int(x : int) : int {
    print("called\n")                              // observable side effect
    return x
}

def good_impure_call(x : int) : int {
    return impure_int(x) < 0 ? -impure_int(x) : impure_int(x)   // NOT flagged (impure)
}
