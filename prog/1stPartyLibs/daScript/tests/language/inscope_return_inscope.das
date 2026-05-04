options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct InscopeFoo {
    a : int
}

[sideeffects]
def inscope_buggy(skip : bool) {
    var a = new InscopeFoo
    if (skip) {
        return
    }
    var b = new InscopeFoo
} finally {
    unsafe {
        delete a
        delete b
    }
}

[test]
def test_inscope_return(t : T?) {
    t |> run("inscope return with finally delete") @(t : T?) {
        inscope_buggy(true)
        t |> success(true)
    }
}
