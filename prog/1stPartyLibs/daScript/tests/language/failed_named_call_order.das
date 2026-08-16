options gen2
expect 30151

require dastest/testing_boost public

def foo(a : int; hint : string) { feint("{a} {hint}\n") }

// named arguments must be a strict suffix -- a positional after a named arg is
// a syntax error (parsing halts at the first one, so this file has just one case)
[test]
def test_named_then_positional(t : T?) {
    foo(hint = "x", 13)
}
