options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

class IdClass {
    def test_method() {
        pass
    }
}

[test]
def test_ignore_deref(t : T?) {
    t |> run("deref pointer from array by value") @(t : T?) {
        var arr <- [new IdClass(), new IdClass()]
        var a = arr[0]
        a->test_method()
        t |> success(true)
    }
    t |> run("explicit pointer type from array") @(t : T?) {
        var arr <- [new IdClass(), new IdClass()]
        var b : IdClass? = arr[0]
        b->test_method()
        t |> success(true)
    }
}
