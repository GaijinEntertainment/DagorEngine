// addr<T>(x) with a non-pointer T is always a typo for addr<T?>(x)
options gen2
expect 30133    // invalid_cast_type

def test {
    var i = 42
    debug(unsafe(addr<int>(i)))
}

[export]
def main {
    test()
}
