options gen2
// LINT009: `then` branch is equivalent to `else` branch
//
// Problem:
//   `if (c) { A } else { A }` runs A regardless of c. Usually the author
//   copy-pasted one branch and forgot to edit the other. Caught even when
//   the body has side effects, since this structural pattern is suspicious
//   regardless of purity.
//
// Bad pattern:
//   if (flag) {
//       x = 1
//   } else {
//       x = 1
//   }
//
// Good pattern:
//   if (flag) {
//       x = 1
//   } else {
//       x = 2
//   }

expect 50503:3

require daslib/lint

// --- Bad: then and else identical ---

def bad_same_assign(cond : bool; var x : int&) {
    if (cond) {                                      // LINT009
        x = 1
    } else {
        x = 1
    }
}

def bad_same_return(cond : bool; x : int) : int {
    if (cond) {                                      // LINT009
        return x + 1
    } else {
        return x + 1
    }
}

def bad_same_block(cond : bool; x : int) : int {
    if (cond) {                                      // LINT009
        let y = x * 2
        return y
    } else {
        let y = x * 2
        return y
    }
}

// --- Good: branches differ ---

def good_diff_assign(cond : bool; var x : int&) {
    if (cond) {
        x = 1
    } else {
        x = 2
    }
}

def good_no_else(cond : bool; var x : int&) {
    if (cond) {
        x = 1
    }
}

def good_diff_return(cond : bool; x : int) : int {
    if (cond) {
        return x + 1
    } else {
        return x - 1
    }
}
