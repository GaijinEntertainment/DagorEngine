options gen2
require UnitTest
require dastest/testing_boost public

[test]
def test_deref_ptr_fun(t : T?) {
    feint("{deref(getPtr())}")
    t |> equal("{deref(getPtr())}", "0")
}

