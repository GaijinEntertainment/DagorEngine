def test_from_gte_to
    // SimNode_ForRangeNF
    for x in range(3,2)
        assert(false)
    for x in urange(3,2)
        assert(false)
    // SimNode_ForRangeNF
    for x in range(3,2)
        print("x={x}\n")
        assert(false)
    for x in urange(3,2)
        print("x={x}\n")
        assert(false)
    // SimNode_ForRange1
    for x in range(3,2)
        break                // don't know how to test
    for x in urange(3,2)
        break                // don't know how to test
    // SimNode_ForRange
    for x in range(3,2)
        assert(false)
        break
    for x in urange(3,2)
        assert(false)
        break
    // range iterator
    for x,y in range(3,2),urange(3,2)
        assert(false)
    for x,y in urange(3,2),urange(3,2)
        assert(false)
    // each
    for x in each(range(3,2))
        assert(false)

def test_for64
    if true
        var t = 0l
        for x,y in range64(100),count()
            assert(t==x)
            assert(t==int64(y))
            t ++
        assert(t==100l)
    if true
        var t = 0ul
        for x in urange64(100)
            assert(t==x)
            t ++
        assert(t==100ul)

[export]
def test:bool
    // 1 variable
    if true
        var a:int[10]
        var i:int=0
        for l in a
            l = i
            i++
        assert(i==10)
        i = 0
        for l in a
            assert(l==i)
            i++
    // 1 variable array
    if true
        var a:array<int>
        var i:int=0
        resize(a,13)
        for l in a
            l = i++
        assert(i==13)
        i = 0
        for l in a
            assert(l==i)
            i++
    // 2 variable, array
    if true
        var a,b:int[10]
        var i:int=0
        for l,r in a,b
            r = i*2-5
            l = i++
        assert(i==10)
        i = 0
        for l,r in a,b
            assert(r==i*2-5)
            verify(l==i++)
        i = 0
        for l,r in a,b
            if l>r
                assert(l>r)
                i++
        assert(i==5)
    // 2 variable, hybrid
    if true
        var a:array<int>
        var b:int[10]
        var i:int=0
        resize(a,4)
        for l,r in a,b
            assert(i<4)
            r = i*2-5
            l = i++
        assert(i==4)
        i = 0
        for l,r in a,b
            assert(r==i*2-5)
            verify(l==i++)
        assert(i==4)
        i = 0
        for l,r in a,b
            if l>r
                assert(l>r)
                i++
        assert(i==4)
    // 1 variable, range
    if true
        var i:int=0
        for a in range(10)
            verify(a==i++)
        assert(i==10)
    // 2 variable, range hybrid
    if true
        var b:int[10]
        var i:int=0
        for l,r in range(10),b    // note, this one is optimized out
            r = i++*2-5
        assert(i==10)
        i = 0
        for l,r in range(10),b
            assert(r==i*2-5)
            verify(l==i++)
        i = 0
        for l,r in range(10),b
            if l>r
                assert(l>r)
                i++
        assert(i==5)
    if true
        var u = 0u
        for t in urange(0u,3u)
            u += t
        assert(u==3u)
    // reverse order
    test_from_gte_to()
    // 64 bit for
    test_for64()
    return true



