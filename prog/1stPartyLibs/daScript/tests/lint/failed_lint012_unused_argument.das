// LINT012 — unused function argument.
// Combined-assertion fixture: one `expect 50503` verifies several behaviors at
// once (50503 is the macro-error code every lint hit raises under
// `require daslib/lint`, so the count must equal the total intended hits).
//
//   f1.deadArg     : plainly-named, never read   — fires LINT012  (the 1 expected)
//   f1._ok         : underscore-prefixed, unused — suppressed
//   f1.usedArg     : read                         — not fired
//   f2._used       : underscore-prefixed but read — NOT fired (args don't get LINT004)
//   f3.annotatedArg: [unused_argument(...)]        — suppressed (marked_used)
//   f4.nolintArg   : // nolint:LINT012 on its line — suppressed
//
// Total expected 50503 hits: 1.
options gen2

require daslib/lint

expect 50503

def f1(usedArg : int; _ok : int; deadArg : int) : int {
    return usedArg
}

def f2(_used : int) : int {
    return _used
}

[unused_argument(annotatedArg)]
def f3(a : int; annotatedArg : int) : int {
    return a
}

def f4(a : int; nolintArg : int) : int { // nolint:LINT012
    return a
}

[export]
def test : bool {
    return f1(1, 2, 3) == 1 && f2(4) == 4 && f3(5, 6) == 5 && f4(7, 8) == 7
}
