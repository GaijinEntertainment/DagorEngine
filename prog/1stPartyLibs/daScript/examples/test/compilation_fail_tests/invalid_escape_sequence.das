expect 10008

[export]
def test()
    var a = "ab\xy"     // 10008: error, invalid escape sequence
    assert(a=="abxy")
    return true
