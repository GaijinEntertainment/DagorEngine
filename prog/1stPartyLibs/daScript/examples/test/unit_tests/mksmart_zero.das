require UnitTest

typedef
    TestObjectSmartPtr = smart_ptr<TestObjectSmart>

[export]
def mks ()
    return <- [[TestObjectSmartPtr]]

[export]
def test
    var inscope p <- mks()
    verify(get_ptr(p)==null)
    return true
