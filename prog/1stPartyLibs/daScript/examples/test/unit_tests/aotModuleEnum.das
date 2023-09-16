require _moduleEnum

[export]
def test
    var a : _moduleEnum::Numbers
    a = _moduleEnum::Numbers zero
    assert(int(a) == 0)
    return true
