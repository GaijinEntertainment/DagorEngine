options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_memzero(t : T?) {
    t |> run("memzero float") @(t : T?) {
        var x = 1.0f
        memzero(x)
        t |> equal(x, 0.0f)
    }
    t |> run("memzero fixed_array") @(t : T?) {
        var tf = fixed_array<int>(1, 2)
        memzero(tf)
        t |> equal(tf[0], 0)
        t |> equal(tf[1], 0)
    }
}
