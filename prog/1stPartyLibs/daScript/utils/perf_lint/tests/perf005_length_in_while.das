options gen2
// PERF005: length(string) in while condition
//
// Problem:
//   `while (i < length(s))` recomputes strlen(s) on every iteration. If `s` is
//   not modified in the loop, this is wasted work — O(n) per iteration for a
//   value that never changes.
//
//   Note: `for (i in range(length(s)))` does NOT have this problem because `for`
//   computes its source expression once before iterating.
//
// Bad pattern:
//   var i = 0
//   while (i < length(s)) {    // strlen every iteration
//       i ++
//   }
//
// Good pattern:
//   let slen = length(s)
//   var i = 0
//   while (i < slen) {         // cached length
//       i ++
//   }

require strings

def bad_length_in_while(s : string) : int {
    var i = 0
    while (i < length(s)) {             // PERF005
        i ++
    }
    return i
}

// --- Good patterns (no warnings) ---

def good_cached_length(s : string) : int {
    let slen = length(s)
    var i = 0
    while (i < slen) {
        i ++
    }
    return i
}

def good_for_range_length(s : string) : int {
    var total = 0
    for (i in range(length(s))) {       // for computes source once — no warning
        total += i
    }
    return total
}
