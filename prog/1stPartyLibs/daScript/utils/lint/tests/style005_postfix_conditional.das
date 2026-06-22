options gen2
// STYLE005: single-statement if can use postfix conditional (off by default)
//
// Problem: Single-statement if blocks can be shorter as postfix conditionals.
//
// Pattern:
//   if (x > 0) { return x }
//
// Suggestion:
//   return x if (x > 0)

// bad: single-statement if with return (1 warning when --postfix-conditionals)
def bad_early_return(x : int) : int {
    if (x > 0) {
        return x
    }
    return -x
}

// good: postfix conditional
def good_early_return(x : int) : int {
    return x if (x > 0)
    return -x
}
