[export]
def test
    // regular to-array
    let d = [[ int 1; 2; 3; 4 ]]
    let a <- to_array([[ int 1; 2; 3; 4 ]])
    for x,y in d,a
        assert(x==y)
    // range
    for x,y in range(5),each(range(5))
        assert(x==y)
    for x,y in range(5),to_array(each(range(5)))
        assert(x==y)
    // static array
    unsafe
        for x,y in d,each(d)
            assert(x==y)
    for x,y in d,to_array(d)
        assert(x==y)
    unsafe
        for x,y in d,to_array(each(d))
            assert(x==y)
    // dynamic array
    unsafe
        for x,y in a,each(a)
            assert(x==y)
    unsafe
        for x,y in a,to_array(each(a))
            assert(x==y)
    return true

