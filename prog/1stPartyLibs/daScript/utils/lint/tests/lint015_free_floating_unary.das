options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT015: free-floating unary +/- expression as a statement
//
// Problem:
//   At statement level daslang is one-statement-per-line. A multi-line
//   arithmetic RHS without wrapping parens splits each continuation into its
//   own statement: `+ b` re-parses as unary `+b` (pure), and the optimizer
//   SILENTLY drops it — terms vanish, wrong result, no diagnostic.
//
// Bad pattern:
//   let x = a
//       + b          // becomes a separate `+b` statement, dropped
//       + c
//
// Good pattern:
//   let x = (a
//       + b
//       + c)
//
// Only `+`/`-` qualify — they are the only operators that are both binary and
// unary. `~`/`!` aren't binary (can't be a split continuation); `++`/`--` mutate
// (real statements); `*`/`|>`/etc. can't start a statement (loud parse error).

expect 50503:3

require daslib/lint

// --- Bad: orphaned continuation lines ---

def bad_plus_continuation(a, b, c : int) : int {
    let x = a
        + b                                          // LINT015
        + c                                          // LINT015
    return x
}

def bad_minus_continuation(a, b : int) : int {
    let x = a
        - b                                          // LINT015
    return x
}

// --- Good: wrapped RHS (newlines fall inside the parens) ---

def good_wrapped(a, b, c : int) : int {
    let x = (a
        + b
        + c)
    return x
}

// --- Good: single-line arithmetic ---

def good_single_line(a, b, c : int) : int {
    let x = a + b + c
    return x
}

// --- Good: unary in expression context (not a statement) ---

def good_unary_in_expr(a : int) : int {
    let y = -a
    return y
}

// --- Good: increment / decrement are real (mutating) statements ---

def good_inc_dec(start : int) : int {
    var i = start
    ++ i
    i ++
    return i
}
