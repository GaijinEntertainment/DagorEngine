options gen2
require UnitTest
require dastest/testing_boost public

[export]
def make_handled_lambda {
    var a = @ { return makeDummy(); }
    unsafe { delete a; }
}

[test]
def test_lambda_handled_result(t : T?) {
    make_handled_lambda()
    t |> success(true, "lambda with a handled result builds")
}
