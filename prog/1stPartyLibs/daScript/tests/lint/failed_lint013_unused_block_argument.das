// LINT013 — unused block argument.
// Combined-assertion fixture (see failed_lint012 for the 50503 counting note).
//
//   b1 deadB    : plainly-named, never read       — fires LINT013  (the 1 expected)
//   b1 usedA    : read                             — not fired
//   b2 _ok      : underscore-prefixed, unused      — suppressed
//   b3 _used    : underscore-prefixed but read     — NOT fired (args don't get LINT004)
//   b4 nolintB  : // nolint:LINT013 on its line    — suppressed
//
// Total expected 50503 hits: 1.
options gen2

require daslib/lint

expect 50503

def call2(blk : block<(a : int; b : int) : void>) {
    invoke(blk, 1, 2)
}

def call1(blk : block<(a : int) : void>) {
    invoke(blk, 1)
}

[export]
def test : bool {
    call2() $(usedA : int; deadB : int) {
        assert(usedA == 1)
    }
    call2() $(usedA : int; _ok : int) {
        assert(usedA == 1)
    }
    call1() $(_used : int) {
        assert(_used == 1)
    }
    call2() $(usedA : int; nolintB : int) { // nolint:LINT013
        assert(usedA == 1)
    }
    return true
}
