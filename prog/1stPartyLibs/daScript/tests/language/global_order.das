options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

var {
    go_some_a <- ["test"]
}

def go_test_a() : array<string> {
    assert(length(go_some_a) == 1)
    return <- clone(go_some_a)
}

let {
    go_some_0 <- go_test_a()
}

[export]
def go_test_0() : array<string> {
    return <- clone(go_some_0)
}

[test]
def test_global_order(t : T?) {
    t |> run("global init order with clone") @(t : T?) {
        t |> equal(length(go_some_a), 1)
        t |> equal(length(go_some_0), 1)
        let cloned <- go_test_0()
        t |> equal(length(cloned), 1)
    }
}
