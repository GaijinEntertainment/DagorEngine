[sideeffects]
def test_while_continue()
    var x = 0
    var y = 0
    while x < 10
        x++
        if (x & 1) == 0
            continue
        y += x
    assert(y==(1+3+5+7+9))

[sideeffects]
def test_for_continue()
    var y = 0
    for x in range(10)
        if (x & 1) == 0
            continue
        y += x
    assert(y==(1+3+5+7+9))

[sideeffects]
def test_complex_for_continue()
    var y = 0
    let arr = [[int 0;1;2;3;4;5;6;7;8;9]]
    for x,s in range(10), arr
        assert(x==s)
        if (x & 1) == 0
            continue
        y += x
    assert(y==(1+3+5+7+9))

[export]
def test()
    test_while_continue()
    test_for_continue()
    test_complex_for_continue()
    return true

