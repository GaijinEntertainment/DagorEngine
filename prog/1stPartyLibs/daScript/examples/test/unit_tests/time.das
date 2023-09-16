def testClockArg(c:clock):clock
    let d = c
    return d

[export]
def test
    var a1,a2:clock
    var dt:double
    a1 = get_clock()
    for z in range(3) // nada on my laptop
        var a,b:int[3]
        for x,y in a,b
            x += y
    a2 = get_clock()
    assert(a1 <= a2)
    let a3 = testClockArg(a2)
    assert(a2 == a3)
    dt = a2 - a1
    assert(dt <= 1.0lf)
    return true
