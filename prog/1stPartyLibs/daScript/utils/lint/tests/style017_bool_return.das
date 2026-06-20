options gen2
// STYLE017: 'if (cond) return true; else return false' — use 'return cond'
//
// Problem: Three lines (or two if-else branches) that just propagate the boolean
// condition unchanged. Read better as `return cond` / `return !cond`.
//
// Bad pattern:
//   if (cond) {
//       return true
//   } else {
//       return false
//   }
//
// Good pattern:
//   return cond
//
// Detection covers two AST shapes:
//   (a) `if (cond) return b1 else return b2` (b1 != b2)
//   (b) `if (cond) return b1` immediately followed by `return b2` (b1 != b2)

expect 31209:4

require daslib/style_lint

// --- Bad patterns ---

def bad_if_else_true_false(c : bool) : bool {
    if (c) {                                       // STYLE017 → return c
        return true
    } else {
        return false
    }
}

def bad_if_else_false_true(c : bool) : bool {
    if (c) {                                       // STYLE017 → return !c
        return false
    } else {
        return true
    }
}

def bad_adjacent_true_false(c : bool) : bool {
    if (c) {                                       // STYLE017 → return c
        return true
    }
    return false
}

def bad_adjacent_false_true(c : bool) : bool {
    if (c) {                                       // STYLE017 → return !c
        return false
    }
    return true
}

// --- Good patterns (no warnings) ---

def good_direct(c : bool) : bool {
    return c
}

def good_negated(c : bool) : bool {
    return !c
}

def good_same_value(c : bool) : bool {
    if (c) return true                             // both true — different rule territory
    return true
}

def good_compute(c : bool; n : int) : bool {
    if (c) return n > 0
    return false
}
