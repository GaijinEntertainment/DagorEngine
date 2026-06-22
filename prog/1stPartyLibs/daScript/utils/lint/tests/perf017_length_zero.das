options gen2
// PERF017: length(s) == 0 / != 0 → empty(s) / !empty(s)
//
// Problem:
//   `length(string)` walks the whole string (strlen). `empty(string)` checks
//   one byte. For arrays/tables both are O(1) but `empty` is the idiomatic
//   form. The rule covers all six comparison ops with literal 0 / 1 on
//   either side (Yoda forms supported).
//
// The diagnostic prints the canonical equivalent: `1 <= length(x)` is
// equivalent to `length(x) >= 1`, so the message says
// `'length(x) >= 1' — use '!empty(x)'` not `'length(x) <= 1'`.
//
// Bad pattern:
//   if (length(s) == 0)   { ... }
//   if (length(arr) > 0)  { ... }
//
// Good pattern:
//   if (empty(s))         { ... }
//   if (!empty(arr))      { ... }
//
// Vector magnitude `length(float3_var)` (math module) is NOT flagged —
// different semantics and there's no `empty` for vectors.

expect 31208:15

require daslib/perf_lint
require strings

// --- Bad patterns (length on left) ---

def bad_string_eq(s : string) : bool {
    return length(s) == 0                      // PERF017 — empty
}

def bad_string_ne(s : string) : bool {
    return length(s) != 0                      // PERF017 — !empty
}

def bad_string_le(s : string) : bool {
    return length(s) <= 0                      // PERF017 — empty (length <= 0 ≡ length == 0)
}

def bad_string_gt(s : string) : bool {
    return length(s) > 0                       // PERF017 — !empty
}

def bad_array_eq(a : array<int>) : bool {
    return length(a) == 0                      // PERF017 — empty
}

def bad_array_ge_one(a : array<int>) : bool {
    return length(a) >= 1                      // PERF017 — !empty (length >= 1 ≡ !empty)
}

def bad_array_lt_one(a : array<int>) : bool {
    return length(a) < 1                       // PERF017 — empty (length < 1 ≡ empty)
}

// --- Bad patterns (Yoda — length on right; canonical-equivalent op) ---

def bad_yoda_eq(s : string) : bool {
    return 0 == length(s)                      // PERF017 — empty (canon: length == 0)
}

def bad_yoda_ne(s : string) : bool {
    return 0 != length(s)                      // PERF017 — !empty (canon: length != 0)
}

def bad_yoda_ge_zero(s : string) : bool {
    // 0 >= length(s) means length(s) <= 0, which is empty
    return 0 >= length(s)                      // PERF017 — empty (canon: length <= 0)
}

def bad_yoda_lt_zero(s : string) : bool {
    // 0 < length(s) means length(s) > 0, which is !empty
    return 0 < length(s)                       // PERF017 — !empty (canon: length > 0)
}

def bad_yoda_le_one(a : array<int>) : bool {
    // 1 <= length(a) means length(a) >= 1, which is !empty
    return 1 <= length(a)                      // PERF017 — !empty (canon: length >= 1)
}

def bad_yoda_gt_one(a : array<int>) : bool {
    // 1 > length(a) means length(a) < 1, which is empty
    return 1 > length(a)                       // PERF017 — empty (canon: length < 1)
}

// --- Bad patterns: table ---

def bad_table_eq(t : table<string; int>) : bool {
    return length(t) == 0                      // PERF017 — empty
}

// --- Good patterns (no warnings) ---

def good_empty(s : string) : bool {
    return empty(s)
}

def good_string_len_other(s : string; n : int) : bool {
    return length(s) == n                      // not 0/1 — NOT flagged
}

def good_length_three(s : string) : bool {
    return length(s) == 3                      // not 0/1 — NOT flagged
}

// --- Bad: user code has a method literally named `empty` ---
// The suppression for `current_function.name == "empty"` is gated on
// _module.name == "builtin", so a user-defined empty() outside builtin still
// gets linted. This is the regression guard for that gating.

class UserContainer {
    items : array<int>

    def empty() : bool {
        return length(items) == 0              // PERF017 — fires (NOT in builtin)
    }
}
