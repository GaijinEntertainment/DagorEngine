require UnitTest

[export]
def test
    let line = testCallLine();
    assert(line==5 || line==0)  // 0 with aot
    return true
