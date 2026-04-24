options gen2
require daslib/lpipe
require dastest/testing_boost public

[sideeffects]
def add(a, b) {
    return a + b
}

var g_add_res = 0

[sideeffects]
def gadd(a, b) {
    g_add_res = a + b
}

struct Foo {
    a : int
}

def operator +(a, b : Foo) {
    return Foo(a = a.a + b.a)
}

[test]
def test_lpipe(t : T?) {
    t |> run("lpipe into function call") @(t : T?) {
        g_add_res = 0
        gadd(1)
        lpipe(2)
        t |> equal(3, g_add_res)
    }
    t |> run("lpipe into variable init") @(t : T?) {
        var i = add(1)
        lpipe(2)
        t |> equal(3, i)
    }
    t |> run("lpipe into assignment operation") @(t : T?) {
        var i = 0
        i = add(1)
        lpipe(2)
        t |> equal(3, i)
    }
    t |> run("lpipe into clone operation") @(t : T?) {
        var i = 0
        i := add(1)
        lpipe(2)
        t |> equal(3, i)
    }
    t |> run("lpipe into move operation") @(t : T?) {
        var i = Foo(a = 0)
        i <- add(Foo(a = 1))
        lpipe(Foo(a = 2))
        t |> equal(3, i.a)
    }
    t |> run("lpipe into variable") @(t : T?) {
        g_add_res = 0
        gadd
        lpipe(1)
        lpipe(2)
        t |> equal(3, g_add_res)
    }
}
