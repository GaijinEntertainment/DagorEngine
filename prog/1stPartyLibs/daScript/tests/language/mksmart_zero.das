options gen2
require UnitTest
require dastest/testing_boost public

typedef TestObjectSmartPtr = smart_ptr<TestObjectSmart>


[export]
def mks() {
    return <- default<TestObjectSmartPtr>
}

[test]
def test_mksmart_zero(t : T?) {
    var inscope p <- mks()
    t |> equal(get_ptr(p), null)
}
