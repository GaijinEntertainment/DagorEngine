options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_for_single_element_array(t : T?) {
    t |> run("iterate single-element dim") @(t : T?) {
        let a = [1234]
        for (x in a) {
            t |> equal(x, 1234)
        }
    }
}
