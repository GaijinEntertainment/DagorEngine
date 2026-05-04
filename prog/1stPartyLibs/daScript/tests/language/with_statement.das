options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct WithPoint {
    x, y : int = 0
}

[test]
def test_with_statement(t : T?) {
    t |> run("with block sets fields") @(t : T?) {
        var p = WithPoint()
        with (p) {
            x = 1
            y = 2
        }
        t |> equal(p.x, 1)
        t |> equal(p.y, 2)
    }
}
