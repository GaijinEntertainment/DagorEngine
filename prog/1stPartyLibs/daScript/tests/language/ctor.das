options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct CtorFoo {
    i : int = 5
    f : float = 6.0
    s : string = "hello"
    n : int64
}

struct CtorBar {
    i : int
}

def CtorBar() : CtorBar {
    var b : CtorBar
    b.i = 13
    return b
}

[test]
def test_ctor(t : T?) {
    t |> run("struct default initializer") @(t : T?) {
        let tf = CtorFoo()
        t |> equal(tf.i, 5)
    }
    t |> run("struct custom constructor") @(t : T?) {
        let b = CtorBar()
        t |> equal(b.i, 13)
    }
}
