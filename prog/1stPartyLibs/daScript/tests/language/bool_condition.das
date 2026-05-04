options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_bool_condition(t : T?) {
    t |> run("false condition skips body") @(t : T?) {
        let a = false
        if (a) {
            t |> failure("should not reach here")
        }
        t |> success(true)
    }
}
