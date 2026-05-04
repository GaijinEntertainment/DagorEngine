options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_to_table(t : T?) {
    t |> run("from fixed_array of tuples") @(t : T?) {
        let a = fixed_array((1, "one"), (2, "two"), (3, "three"))
        var b <- to_table(fixed_array((1, "one"), (2, "two"), (3, "three")))
        for (tup in a) {
            t |> equal(b?[tup._0] ?? "", tup._1)
        }
    }
}
