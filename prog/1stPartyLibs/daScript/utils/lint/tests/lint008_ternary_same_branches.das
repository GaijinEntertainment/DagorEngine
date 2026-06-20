options gen2
// LINT008: Both branches of ternary `?:` are equivalent
//
// Problem:
//   `cond ? x : x` ignores `cond` entirely and always evaluates `x` — almost
//   always a copy-paste bug where one branch should have been something else.
//
// Bad pattern:
//   return cond ? value : value
//
// Good pattern:
//   return cond ? then_value : else_value

expect 50503:3

require daslib/lint

// --- Bad: both branches identical ---

def bad_same_var(cond : bool; x : int) : int {
    return cond ? x : x                              // LINT008
}

def bad_same_expr(cond : bool; x : int; y : int) : int {
    return cond ? (x + y) : (x + y)                  // LINT008
}

def bad_same_mul(cond : bool; x : int) : int {
    return cond ? (x * 2) : (x * 2)                  // LINT008
}

// --- Good: branches differ ---

def good_diff_values(cond : bool; x : int; y : int) : int {
    return cond ? x : y
}

def good_neg(cond : bool; x : int) : int {
    return cond ? x : -x
}

def good_constants(cond : bool) : int {
    return cond ? 1 : 0
}
