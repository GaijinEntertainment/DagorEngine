require UnitTest

typedef
    TestObjectSmartPtr = smart_ptr<TestObjectSmart>

[export]
def mks ()
    return [[TestObjectSmartPtr]]

[export]
def test
    let p <- mks()
    verify(get_ptr(p)==null)
    return true
