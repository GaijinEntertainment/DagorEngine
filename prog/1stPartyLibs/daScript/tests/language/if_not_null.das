options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/if_not_null

struct IfnnFoo {
    a : int
}

var g_ifnn_called = false

[sideeffects]
def ifnn_take3(var pfoo : IfnnFoo; b, c : int) {
    assert(pfoo.a == 1 && b == 2 && c == 3)
    g_ifnn_called = true
}

[test]
def test_if_not_null(t : T?) {
    t |> run("null pointer skips block") @(t : T?) {
        var pfoo : IfnnFoo?
        g_ifnn_called = false
        pfoo |> if_not_null() <| ifnn_take3(0, 0)
        t |> equal(g_ifnn_called, false)
    }
    t |> run("non-null pointer invokes block") @(t : T?) {
        var pfoo = new IfnnFoo(a = 1)
        g_ifnn_called = false
        pfoo |> if_not_null() <| ifnn_take3(2, 3)
        t |> equal(g_ifnn_called, true)
        unsafe { delete pfoo; }
    }
}
