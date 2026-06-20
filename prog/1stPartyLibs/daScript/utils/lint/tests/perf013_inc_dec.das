options gen2
// PERF013: a += 1 / a -= 1 should be a++ / a--
//
// Problem:
//   `a += 1` and `a -= 1` are 2-node operations in interpreted mode (read,
//   add/sub, write). The postfix `++` / `--` collapses to a single SimNode.
//   The +=/-= form also reads worse than the canonical inc/dec idiom.
//
// All four (op, ±RHS) combinations are flagged; the diagnostic suggests
// `++` for delta=+1 and `--` for delta=-1:
//
//   a += 1   → ++  (delta +1)
//   a -= 1   → --  (delta -1)
//   a += -1  → --  (delta -1, same as a -= 1)
//   a -= -1  → ++  (delta +1, same as a += 1)
//
// Bad pattern:
//   a += 1
//   a -= 1
//
// Good pattern:
//   a++
//   a--

// 6 numeric scalar types × 2 ops × 2 RHS signs would explode; this file
// covers the int and float paths plus the four signed-only neg-one cases
// (uint/uint64 omit -1 since negative literals don't apply).
expect 31208:12

require daslib/perf_lint

// --- Bad patterns: int (4 combos) ---

def bad_int_plus_one() : int {
    var a = 0
    a += 1                                     // PERF013 — suggests ++
    return a
}

def bad_int_minus_one() : int {
    var a = 0
    a -= 1                                     // PERF013 — suggests --
    return a
}

def bad_int_plus_neg_one() : int {
    var a = 0
    a += -1                                    // PERF013 — suggests -- (same as -= 1)
    return a
}

def bad_int_minus_neg_one() : int {
    var a = 0
    a -= -1                                    // PERF013 — suggests ++ (same as += 1)
    return a
}

// --- Bad patterns: float (4 combos) ---

def bad_float_plus_one() : float {
    var x = 0.0
    x += 1.0                                   // PERF013 — suggests ++
    return x
}

def bad_float_minus_one() : float {
    var x = 0.0
    x -= 1.0                                   // PERF013 — suggests --
    return x
}

def bad_float_plus_neg_one() : float {
    var x = 0.0
    x += -1.0                                  // PERF013 — suggests --
    return x
}

def bad_float_minus_neg_one() : float {
    var x = 0.0
    x -= -1.0                                  // PERF013 — suggests ++
    return x
}

// --- Bad patterns: other workhorse numeric types ---

def bad_int64() : int64 {
    var a = 0l
    a += 1l                                    // PERF013 — suggests ++
    return a
}

def bad_uint() : uint {
    var a = 0u
    a += 1u                                    // PERF013 — suggests ++
    return a
}

def bad_uint64() : uint64 {
    var a = 0ul
    a += 1ul                                   // PERF013 — suggests ++
    return a
}

def bad_double() : double {
    var x = 0.0lf
    x += 1.0lf                                 // PERF013 — suggests ++
    return x
}

// --- Good patterns (no warnings) ---

def good_postfix() : int {
    var a = 0
    a ++
    a --
    return a
}

def good_step_other() : int {
    var a = 0
    a += 2                                     // step != 1 — no warning
    a -= 5
    return a
}
