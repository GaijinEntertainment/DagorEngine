options gen2
// STYLE016: adjacent guards leading to identical early-exit can be combined with '||'
//
// Problem: Two adjacent if-guards with the same exit (return/break/continue with the
// same payload) read as a single decision. Merging into one '||' gate is shorter and
// makes the relationship explicit.
//
// Bad pattern:
//   if (a) return -1
//   if (b) return -1
//
// Good pattern:
//   if (a || b) return -1
//
// Detection covers two AST shapes:
//   (a) two adjacent ExprIfThenElse statements in the same block
//   (b) `if (a) return X else if (b) return X` chain

expect 31209:7

require daslib/style_lint

// --- Bad patterns (warnings) ---
// All bodies use braceless form so STYLE005 doesn't also fire — focus this fixture
// on STYLE016 only. (STYLE005 has its own dedicated fixture.)

def bad_adjacent_returns(a, b : int) : int {
    if (a == 0) return -1                          // STYLE016
    if (b == 0) return -1
    return 0
}

def bad_else_chain(a, b : int) : int {
    if (a == 0) {                                  // STYLE016 (chained else-if form)
        return -1
    } elif (b == 0) return -1
    return 0
}

def bad_break_pair() : void {
    for (i in range(10)) {
        if (i == 3) break                          // STYLE016
        if (i == 5) break
    }
}

def bad_continue_pair() : void {
    for (i in range(10)) {
        if (i % 2 == 0) continue                   // STYLE016
        if (i % 3 == 0) continue
    }
}

def bad_three_deep(a, b, c : int) : int {
    if (a == 0) return -1                          // STYLE016 (a-guard with b-guard)
    if (b == 0) return -1                          // STYLE016 (b-guard with c-guard)
    if (c == 0) return -1
    return 0
}

def bad_return_with_value(a, b : int) : int {
    if (a == 0) return 42                          // STYLE016 (same int payload)
    if (b == 0) return 42
    return 0
}

// --- Good patterns (no warnings) ---

def good_combined(a, b : int) : int {
    if (a == 0 || b == 0) return -1
    return 0
}

def good_different_payload(a, b : int) : int {
    if (a == 0) return -1
    if (b == 0) return -2
    return 0
}

def good_multi_stmt_body(a, b : int) : int {
    if (a == 0) {
        print("a is zero\n")
        return -1
    }
    if (b == 0) return -1
    return 0
}

def good_intervening(a, b : int) : int {
    if (a == 0) return -1
    print("a is non-zero\n")
    if (b == 0) return -1
    return 0
}
