options gen2

expect 30704 : 4
expect 30708 : 1

// Tuple destructuring binds names like `let` declarations, so it follows the same
// shadowing rules. Before the check, a repeated name silently rebound (the newest
// assume alias won in ExprVar resolution) with no diagnostic - while plain
// `let x` twice was error[30704]. The for-loop form reports 30708, matching the
// plain for-iterator shadowing diagnostics beside it.

def p1() : tuple<bool; int> {
    return (true, 1)
}

def rebind_between_destructures {
    let (ok, a) = p1()
    let (ok, b) = p1()      // 30704: ok is already bound by the first destructure
    print("{ok} {a} {b}\n")
}

def rebind_within_one_destructure {
    let (dup, dup) = p1()   // 30704: same name twice in one pattern
    print("{dup}\n")
}

def rebind_over_local {
    let ok = 5
    let (ok, c) = p1()      // 30704: ok is an existing local
    print("{ok} {c}\n")
}

def rebind_over_argument(ok : int) {
    let (ok, d) = p1()      // 30704: ok shadows the function argument
    print("{ok} {d}\n")
}

def rebind_in_for_loop {
    let k = 13
    var pairs : array<tuple<int; string>>
    for ((k, v) in pairs) { // 30708: k is an existing local (for-loop code, like plain iterators)
        print("{k} {v}\n")
    }
}

[export]
def main {
    rebind_between_destructures()
    rebind_within_one_destructure()
    rebind_over_local()
    rebind_over_argument(1)
    rebind_in_for_loop()
}
