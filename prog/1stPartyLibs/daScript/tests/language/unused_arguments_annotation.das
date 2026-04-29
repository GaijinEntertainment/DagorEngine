options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[sideeffects, unused_argument(b, c)]
def ua_haha(a, b, c) {
    return a * 2
}

[test]
def test_unused_arguments_annotation(t : T?) {
    t |> run("unused_argument annotation suppresses warning") @(t : T?) {
        let v = ua_haha(1, 2.0, "blah")
        t |> equal(v, 2)
    }
}
