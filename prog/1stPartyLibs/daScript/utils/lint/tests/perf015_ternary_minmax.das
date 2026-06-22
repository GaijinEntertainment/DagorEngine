options gen2
// PERF015: ternary min/max → min(a, b) / max(a, b)
//
// Problem:
//   `a < b ? a : b` reimplements `min(a, b)`. Same for the four orientations.
//   The math::min/max builtins are vec-friendly and the intent is clearer.
//
// Bad pattern:
//   let smaller = a < b ? a : b
//   let larger  = a > b ? a : b
//
// Good pattern:
//   let smaller = min(a, b)
//   let larger  = max(a, b)

expect 31208:8

require daslib/perf_lint
require math

// --- Bad patterns (PERF015) ---
//   <  / <=  : T==L,F==R → min ;  T==R,F==L → max
//   >  / >=  : T==L,F==R → max ;  T==R,F==L → min

def bad_lt_min(a, b : int) : int {
    return a < b ? a : b                          // PERF015 — min
}

def bad_lt_max(a, b : int) : int {
    return a < b ? b : a                          // PERF015 — max
}

def bad_le_min(a, b : int) : int {
    return a <= b ? a : b                         // PERF015 — min
}

def bad_le_max(a, b : int) : int {
    return a <= b ? b : a                         // PERF015 — max
}

def bad_gt_max(a, b : int) : int {
    return a > b ? a : b                          // PERF015 — max
}

def bad_gt_min(a, b : int) : int {
    return a > b ? b : a                          // PERF015 — min
}

def bad_ge_max(a, b : int) : int {
    return a >= b ? a : b                         // PERF015 — max
}

def bad_ge_min(a, b : int) : int {
    return a >= b ? b : a                         // PERF015 — min
}

// --- Good patterns (no warnings) ---

def good_min(a, b : int) : int {
    return min(a, b)
}

def good_max(a, b : int) : int {
    return max(a, b)
}

def good_asymmetric_branches(a, b, c : int) : int {
    return a < b ? a : c                          // T != R — NOT flagged
}

def good_different_operands(a, b, c, d : int) : int {
    return a < b ? c : d                          // condition operands ≠ branches
}

// Side-effectful operands: `f() < g() ? f() : g()` calls f() and g() up to twice;
// `min(f(), g())` calls each exactly once. The rewrite would silently change the
// number of calls, so the rule must NOT fire on impure operands.

[unused_argument(x)]
def impure_int(x : int) : int {
    print("called\n")                             // observable side effect
    return x
}

def good_impure_call(a, b : int) : int {
    return impure_int(a) < impure_int(b) ? impure_int(a) : impure_int(b)   // NOT flagged (impure)
}
