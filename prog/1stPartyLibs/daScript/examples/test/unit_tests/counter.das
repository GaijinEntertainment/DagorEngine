[export]
def test()

    var A <- [{int 0; 2; 4; 6; 8}]
    var count = 0
    for a,c in A,count(0,2)
        assert(a==c)
        count += c
    assert(count==20)
    return true