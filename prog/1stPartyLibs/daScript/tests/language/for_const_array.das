options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_for_const_array(t : T?) {
    t |> run("iterate fixed_array constant") @(t : T?) {
        for (x in fixed_array<int>(1, 1)) {
            t |> equal(x, 1)
        }
    }
}
