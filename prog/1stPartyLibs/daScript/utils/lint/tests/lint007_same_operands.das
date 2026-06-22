options gen2
// LINT007: Left and right operands of binary operator are identical
//
// Problem:
//   `x op x` where both sides are the same expression produces a trivial
//   result: `x == x` is always true, `x - x` is always 0, `x && x` is just x.
//   Almost always a typo — the author meant `x op y`.
//
// Bad pattern:
//   if (size == size) { ... }          // meant `size == capacity` or similar
//
// Good pattern:
//   if (size == capacity) { ... }
//
// NaN idiom (rare, legitimate):
//   `x != x` on float/double is the canonical IEEE 754 NaN check; daslang has
//   no dedicated `is_nan` helper on scalar floats. Suppress on the one line
//   that needs it with `// nolint:LINT007 — NaN check`.
//
// Note on constant folding:
//   `let x = 1; x == x` is folded to `true` before lint runs and will NOT
//   fire. LINT007 catches cases where operands are runtime values (function
//   parameters, field reads, etc.). The fixture uses parameters on purpose.

expect 50503:17

require daslib/lint

// --- Bad: each LINT007 operator once ---

def bad_eq(x : int) : bool {
    return x == x                                    // LINT007
}

def bad_ne(x : int) : bool {
    return x != x                                    // LINT007
}

def bad_lt(x : int) : bool {
    return x < x                                     // LINT007
}

def bad_gt(x : int) : bool {
    return x > x                                     // LINT007
}

def bad_le(x : int) : bool {
    return x <= x                                    // LINT007
}

def bad_ge(x : int) : bool {
    return x >= x                                    // LINT007
}

def bad_sub(x : int) : int {
    return x - x                                     // LINT007
}

def bad_div(x : int) : int {
    return x / x                                     // LINT007
}

def bad_mod(x : int) : int {
    return x % x                                     // LINT007
}

def bad_and_and(b : bool) : bool {
    return b && b                                    // LINT007
}

def bad_or_or(b : bool) : bool {
    return b || b                                    // LINT007
}

def bad_bit_and(x : uint) : uint {
    return x & x                                     // LINT007
}

def bad_bit_or(x : uint) : uint {
    return x | x                                     // LINT007
}

def bad_xor(x : uint) : uint {
    return x ^ x                                     // LINT007
}

def bad_sub_assign(x : int) : int {
    var y = x
    y -= y                                           // LINT007
    return y
}

def bad_div_assign(x : int) : int {
    var y = x
    y /= y                                           // LINT007
    return y
}

def bad_mod_assign(x : int) : int {
    var y = x
    y %= y                                           // LINT007
    return y
}

// --- Good: different operands ---

def good_diff_eq(a : int; b : int) : bool {
    return a == b
}

def good_diff_sub(a : int; b : int) : int {
    return a - b
}

// --- Good: NaN idiom, suppressed on the one line that needs it ---

def good_nan_check(x : float) : bool {
    return x != x // nolint:LINT007 — canonical IEEE 754 NaN check
}

def good_is_not_nan(x : double) : bool {
    return x == x // nolint:LINT007 — canonical IEEE 754 non-NaN check (true for ±inf too)
}
