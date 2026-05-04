options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

let {
    g_ctm_a <- [1, 2, 3]
    g_ctm_b := g_ctm_a
}

[test]
def test_clone_to_move(t : T?) {
    t |> run("global clone-assign arrays") @(t : T?) {
        t |> equal(length(g_ctm_a), 3)
        t |> equal(length(g_ctm_b), 3)
        for (x, y in g_ctm_a, g_ctm_b) {
            t |> equal(x, y)
        }
    }
    t |> run("local clone-assign arrays") @(t : T?) {
        let a <- [1, 2, 3]
        let b := a
        t |> equal(length(a), 3)
        t |> equal(length(b), 3)
        for (x, y in a, b) {
            t |> equal(x, y)
        }
    }
}
