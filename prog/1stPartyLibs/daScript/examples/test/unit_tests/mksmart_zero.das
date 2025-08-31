options gen2
require UnitTest

typedef TestObjectSmartPtr = smart_ptr<TestObjectSmart>


[export]
def mks() {
    return <- default<TestObjectSmartPtr>
}

[export]
def test {
    var inscope p <- mks()
    verify(get_ptr(p) == null)
    return true
}
