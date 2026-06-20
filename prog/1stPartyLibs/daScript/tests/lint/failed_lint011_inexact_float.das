// LINT011 — int literal promoted to float/double with precision loss.
// Combined-assertion fixture: one expect 50503 (runtime_macro) verifies
// FOUR behaviors in a single test.
//
// Lines and their expected behavior:
//   _exact       : exactly representable (2^24)    — must NOT fire
//   _inexact     : 2^24 + 1                        — fires LINT011 (the 1 expected)
//   _suppressed  : 2^24 + 3 + nolint:LINT011       — must be suppressed
//   _dbl         : int → double, uint32 fits 2^53  — must NOT fire
//
// If any of those four behaviors regresses, the expected count won't match
// the actual count and the test fails.
options gen2

require daslib/lint

expect 50503

[export]
def test : bool {
    let _exact : float = 16777216
    let _inexact : float = 16777217
    let _suppressed : float = 16777219   // nolint:LINT011
    let _dbl : double = 100
    return true
}
