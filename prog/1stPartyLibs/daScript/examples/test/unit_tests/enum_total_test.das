require _enum_total

[enum_total]
enum FooBar
    foo
    bar

[export]
def test()
    assert(int(FooBar total)==2)    // not a const
    return true

[export]
def main
    test()
