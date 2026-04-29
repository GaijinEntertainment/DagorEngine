options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_try_recover(t : T?) {
    t |> run("deref null recovers") @(t : T?) {
        var a = 1
        let b : int?
        try {
            a = deref(b)
        } recover {
            a = 13
        }
        t |> equal(a, 13)
    }
    t |> run("panic recovers") @(t : T?) {
        var a = 1
        try {
            panic("now panic")
        } recover {
            a = 14
        }
        t |> equal(a, 14)
    }
}
