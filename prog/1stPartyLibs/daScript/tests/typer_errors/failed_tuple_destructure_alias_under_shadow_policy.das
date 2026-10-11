options gen2
options allow_local_variable_shadowing

expect 30704 : 2

// Destructured names are assume aliases - same-scope REPLACEMENT, not scoped
// shadowing - so the alias collision checks stay on even when the shadowing
// policy is relaxed, exactly like the taken-by-alias check on a plain `let`.

def p1() : tuple<bool; int> {
    return (true, 1)
}

def rebind_between_destructures {
    let over = 1
    let over2 = over // shadowing policy: plain let shadowing is legal in this file
    let (ok, a) = p1()
    let (ok, b) = p1()      // 30704: alias replacement is never silent
    print("{ok} {a} {b} {over2}\n")
}

def rebind_within_one_destructure {
    let (dup, dup) = p1()   // 30704: same name twice in one pattern
    print("{dup}\n")
}

[export]
def main {
    rebind_between_destructures()
    rebind_within_one_destructure()
}
