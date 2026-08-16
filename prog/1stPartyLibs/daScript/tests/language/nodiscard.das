options gen2
require dastest/testing_boost public

// A [nodiscard] result consumed by string interpolation is USED — the string builder formats
// it into the output. Before the fix InferTypes never marked string-builder elements as
// not-discarded, so "{answer()}" tripped a false error[30166] wherever it appeared
// (call-argument builders and let-init builders alike).

[nodiscard]
def answer : int => 42

[test]
def test_nodiscard_in_interpolation(t : T?) {
    // call-argument string builder (the reported shape: print("{f()}"))
    to_log(LOG_INFO, "nodiscard in interpolation: {answer()}\n")
    // let-init string builder
    let s = "value is {answer()}"
    t |> equal(s, "value is 42")
}

[test]
def test_nodiscard_consumed_forms(t : T?) {
    // the always-marked consumed positions keep working
    let x = answer()
    t |> equal(x, 42)
    t |> equal(answer() + 0, 42)
}
