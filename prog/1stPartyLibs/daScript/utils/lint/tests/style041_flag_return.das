options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE041: a bool flag set-then-returned is a return value in disguise
//
// Problem: `var flag = false` set on some paths, then consumed by a single
// `if (flag) return X` right after the statement that set it - the flag is a
// return value with extra steps, and the loop often keeps running after the
// answer is known. Return directly at each set site and drop the flag.
// Each good case below names the exemption it pins.

expect 31209:3

require daslib/style_lint

// --- Bad patterns (must fire - 3 total) ---

def bad_simple(xs : array<int>) : int {
    var found = false
    for (x in xs) {
        if (x < 0) {
            found = true
            break
        }
    }
    if (found) return -1        // STYLE041 - return -1 at the set site
    return 0
}

def bad_report_return(name : string; xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) {                  // STYLE041 - move the body to the set site
        to_log(LOG_ERROR, "bad input in {name}\n")
        return 0
    }
    return length(xs)
}

def bad_break_plumbing(xs : array<int>) : int {
    var failed = false
    var total = 0
    for (x in xs) {
        break if (failed)
        for (y in range(x)) {
            if (y == 13) {
                failed = true
                break
            }
        }
        continue if (failed)
        total += x
    }
    if (failed) return -1       // STYLE041 - the break/continue reads are plumbing
    return total
}

// --- Good patterns (must stay silent) ---

// module-scope latch, not a function-local flag (memo/re-entrancy idiom)
var private g_st041_latch = false

def neg_global_latch() : int {
    if (g_st041_latch) return 1
    g_st041_latch = true
    return 0
}

def each_int(xs : array<int>; blk : block<(x : int) : void>) {
    for (x in xs) {
        invoke(blk, x)
    }
}

// set inside a block callback - the walk-abort idiom, the flag is the only way out
def neg_callback_set(xs : array<int>) : int {
    var found = false
    each_int(xs) $(x) {
        if (x < 0) {
            found = true
        }
    }
    if (found) return -1
    return 0
}

// negative polarity - the search-miss family; the fix is extraction, not a return
def neg_search_miss(xs : array<int>; want : int) : int {
    var seen = false
    for (x in xs) {
        if (x == want) {
            seen = true
            break
        }
    }
    if (!seen) return -1
    return 1
}

// set is not terminal on its path - work happens after the answer is known ON PURPOSE
def neg_nonterminal(xs : array<int>) : int {
    var found = false
    var total = 0
    for (x in xs) {
        if (x < 0) {
            found = true
            total += x
        }
    }
    if (found) return -1
    return total
}

// a statement between the setter and the consume - the collapse would skip it
// (set is terminal and escaping, so adjacency is the ONE check that rejects)
def neg_between(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    let n = length(xs)
    if (bad) return -1
    return n
}

// payload is a mutable local - not provably the same value at the set site
// (set is terminal and escaping, so the payload rule is the ONE check that rejects)
def neg_unstable_payload(xs : array<int>) : int {
    var found = false
    var last = 0
    for (x in xs) {
        if (x < 0) {
            last = x
            found = true
            break
        }
    }
    if (found) return last
    return 0
}

// two consuming reads - the count rule is the ONE check that rejects (each
// consume alone would be a valid collapse target)
def neg_two_consumers(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) {
        to_log(LOG_ERROR, "bad input\n")
        return -2
    }
    if (bad) return -1
    return 0
}

// combined read - the flag is half of a wider condition
def neg_combined_read(xs : array<int>; verbose : bool) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad && verbose) return -1
    return 0
}

// a reference after the consume - the reference-count check catches it
def neg_read_after(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) return -1
    return bad ? 1 : 0
}

// init-true separator flag - read/written every iteration, not a return
def neg_separator(xs : array<int>) : string {
    var first = true
    var s = ""
    for (x in xs) {
        if (!first) {
            s += ","
        }
        s += "{x}"
        first = false
    }
    return s
}

// set + continue in an UNGUARDED loop that deliberately keeps reporting - the
// collapse would eat the remaining diagnostics (the hygiene tool's shape)
def neg_collect_diagnostics(xs : array<int>) : int {
    var any_missing = false
    for (x in xs) {
        if (x < 0) {
            to_log(LOG_ERROR, "missing {x}\n")
            any_missing = true
            continue
        }
        to_log(LOG_INFO, "ok {x}\n")
    }
    if (any_missing) return 2
    return 0
}

// sets span two top statements - the one-setter rule is the ONE check that rejects
def neg_two_writers(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    for (x in xs) {
        if (x > 100) {
            bad = true
            break
        }
    }
    if (bad) return -1
    return 0
}

// init-true flags never match - the init filter alone rejects this shape
def neg_init_true(xs : array<int>) : int {
    var found = true
    for (x in xs) {
        if (x < 0) {
            found = true
            break
        }
    }
    if (found) return -1
    return 0
}

// generic instances are skipped - instantiations share the template's location
def neg_generic(xs : array<auto(TT)>) : int {
    var found = false
    for (x in xs) {
        if (x < 0) {
            found = true
            break
        }
    }
    if (found) return -1
    return 0
}

// a write of anything but literal `true` disqualifies the flag outright
def neg_set_false(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
        if (x > 100) {
            bad = false
            break
        }
    }
    if (bad) return -1
    return 0
}

// the consuming if must BE the next top statement, not sit nested inside one
def neg_nested_consume(xs : array<int>; deep : bool) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (deep) {
        if (bad) return -1
    }
    return 0
}

// a lambda capturing the flag poisons the candidate - the capture may not surface as a read
def neg_lambda_capture(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) return -1
    var l = @() : bool {
        return bad
    }
    return invoke(l) ? 1 : 0
}

// a break-plumb read outside the setter statement disqualifies - it guards nothing
def neg_stray_plumb(xs : array<int>) : int {
    var bad = false
    for (x in xs) {
        break if (bad)
    }
    for (x in xs) {
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) return -1
    return 0
}

// a report argument that is not provably repeatable at the set site
def neg_local_report_arg(xs : array<int>) : int {
    var bad = false
    var total = 0
    for (x in xs) {
        total++
        if (x < 0) {
            bad = true
            break
        }
    }
    if (bad) {
        to_log(LOG_ERROR, "bad after {total} items\n")
        return -1
    }
    return 0
}

[export]
def main() {
    var xs <- [1, 2, 3]
    print("{bad_simple(xs)} {bad_report_return("n", xs)} {bad_break_plumbing(xs)}\n")
    print("{neg_global_latch()} {neg_callback_set(xs)} {neg_search_miss(xs, 2)} {neg_nonterminal(xs)}\n")
    print("{neg_between(xs)} {neg_unstable_payload(xs)} {neg_two_consumers(xs)} {neg_combined_read(xs, false)}\n")
    print("{neg_read_after(xs)} {neg_separator(xs)} {neg_collect_diagnostics(xs)} {neg_two_writers(xs)}\n")
    print("{neg_init_true(xs)} {neg_generic(xs)} {neg_set_false(xs)} {neg_nested_consume(xs, true)}\n")
    print("{neg_lambda_capture(xs)} {neg_stray_plumb(xs)} {neg_local_report_arg(xs)}\n")
}
