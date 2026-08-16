options gen2
expect 30341:5

require dastest/testing_boost public

def foo(a : int; hint : string; extra : int = 7) { feint("{a} {hint} {extra}\n") }
def bar(a : int; b : int) { feint("{a} {b}\n") }

[test]
def test_bad_named(t : T?) {
    foo(1, zzz = "x")               // unknown named argument
    foo(1, hint = "x", hint = "y")  // duplicate named argument
    foo(1, extra = 9)               // missing required 'hint' (no default)
    foo(1, extra = 9, hint = "x")   // named arguments out of parameter order
    bar(1, 2, a = 3)                // named 'a' targets a positionally-filled slot
}
