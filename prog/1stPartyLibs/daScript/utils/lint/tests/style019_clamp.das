options gen2
// STYLE019: nested min/max → clamp(x, lo, hi)
//
// Problem: `min(max(x, lo), hi)` reads as a clamp; `clamp(x, lo, hi)` is a
// math builtin that says so directly.
//
// Bad pattern:
//   let bounded = min(max(x, lo), hi)
//   let bounded_alt = max(min(x, hi), lo)
//
// Good pattern:
//   let bounded = clamp(x, lo, hi)

expect 31209:4

require daslib/style_lint
require math

// --- Bad patterns ---

def bad_min_max(x, lo, hi : int) : int {
    return min(max(x, lo), hi)                     // STYLE019
}

def bad_max_min(x, lo, hi : int) : int {
    return max(min(x, hi), lo)                     // STYLE019 (mirror form)
}

def bad_min_max_inner_second(x, lo, hi : int) : int {
    return min(hi, max(x, lo))                     // STYLE019 — inner is arg[1]
}

def bad_min_max_float(x, lo, hi : float) : float {
    return min(max(x, lo), hi)                     // STYLE019
}

// --- Good patterns (no warnings) ---

def good_clamp(x, lo, hi : int) : int {
    return clamp(x, lo, hi)
}

def good_min_only(a, b : int) : int {
    return min(a, b)
}

def good_max_only(a, b : int) : int {
    return max(a, b)
}

def good_three_arg_min_no_nested(x, y, z : int) : int {
    return min(min(x, y), z)                       // not min(max ...) — NOT flagged
}
