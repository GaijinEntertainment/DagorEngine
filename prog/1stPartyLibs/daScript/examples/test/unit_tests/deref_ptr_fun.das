require UnitTest
[export]
def test():bool
    print("{deref(getPtr())}")
    verify("{deref(getPtr())}"=="0")
    return true

