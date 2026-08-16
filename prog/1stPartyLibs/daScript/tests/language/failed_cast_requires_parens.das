options gen2
expect 30151

require dastest/testing_boost public

// the juxtaposition cast form is gone: `cast<T> expr` must now be spelled `cast<T>(expr)`.
// the old form silently swallowed trailing binary operators into the operand
// (`reinterpret<uint8?>(p) + 1` meant `reinterpret<uint8?>((p) + 1)`).

def foo : int {
    var x = 0
    return cast<int> x
}

[test]
def test_never_runs(t : T?) {
    t |> success(foo() == 0)
}
