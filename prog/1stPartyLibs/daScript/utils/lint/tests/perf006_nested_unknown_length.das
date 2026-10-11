options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF006 regression: nesting a known-length and an unknown-length for must not
// unbalance the known-length counter for the rest of the program.
//
// The counter is saved per loop: an inner for over an iterator (unknown length)
// must not erase the outer array-loop's "known" state on exit (a later while-loop
// push must stay silent), and an inner known loop under an unknown outer must not
// drive the counter negative (a later genuine known-loop push must still fire).

expect 31208:2

require daslib/perf_lint

def known_outer_unknown_inner(tab : table<int; int>) : int {
    var sink : array<int>
    var total = 0
    for (i in range(10)) {
        sink |> push(i)                         // PERF006 - the genuine one
        for (k in keys(tab)) {                  // nolint:PERF018 — unknown-length inner is the point
            total += k
        }
    }
    delete sink
    return total
}

def while_push_stays_silent(n : int) : int {
    var buf : array<int>
    var i = 0
    while (i < n) {
        buf |> push(i)                          // no warning - while loops are runtime-bound
        i++
    }
    let k = length(buf)
    delete buf
    return k
}

def unknown_outer_known_inner(tab : table<int; int>) : int {
    var total = 0
    for (k in keys(tab)) {
        for (i in range(3)) {
            total += i + k
        }
    }
    return total
}

def genuine_after_the_nesting() : array<int> {
    var result : array<int>
    for (i in range(100)) {
        result |> push(i)                       // PERF006 - must still fire after the loops above
    }
    return <- result
}
