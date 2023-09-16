[sideeffects]
def checkIt ( gen : iterator<int> )
    //print("testing:\n")
    var count = 0
    for x, y in gen, count()
        //print("{x} vs {y}\n")
        if y >= 10
            assert(false,"output too long")
        assert(x==y,"value mismatch")
        count ++
    assert(count==10,"output too short")

def test_yield
    let gen <- generator<int>() <|
        yield 0
        yield 1
        yield 2
        yield 3
        yield 4
        yield 5
        yield 6
        yield 7
        yield 8
        yield 9
        return false
    checkIt(gen)

def test_local
    let gen <- generator<int>() <|
        var  t = 0
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        return false
    checkIt(gen)

def test_local_ref
    let gen <- generator<int>() <|
        var t = 0
        var T : int & = t
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        yield t++
        yield T++
        return false
    checkIt(gen)

def test_if
    let gen <- generator<int>() <|
        var  t = 0
        yield t++
        // if
        if t == 1
            yield t++
        // if-else
        if t == 2
            yield t++
        else
            yield 13
        // nested if
        if t == 3
            if t < 10
                yield t++
            else
                yield 13
        else
            yield 12
        // if-elif-else
        if t == 13
            yield 0
        elif t ==4
            yield t++
        else
            yield -1
        yield t++
        yield t++
        yield t++
        yield t++
        yield t++
        return false
    checkIt(gen)

def test_while
    let gen <- generator<int>() <|
        var t = 0
        while t < 10
            yield t ++
        return false
    checkIt(gen)

def test_while_break
    let gen <- generator<int>() <|
        var t = 0
        while t < 100
            if t == 10
                break
            yield t ++
        return false
    checkIt(gen)

def test_while_continue
    let gen <- generator<int>() <|
        var t = 0
        while t < 100
            if t >= 10
                t ++
                continue
            yield t ++
        return false
    checkIt(gen)

def test_while_nested
    let gen <- generator<int>() <|
        var q = 0
        while q < 2
            var t = 0
            while t < 5
                yield q*5 + t ++
            q ++
        return false
    checkIt(gen)

def test_while_finally
    let gen <- generator<int>() <|
        var t = 0
        while t < 9
            yield t ++
        finally
            yield 9
        return false
    checkIt(gen)

def test_for
    let gen <- generator<int>() <|
        for t in range(10)
            yield t
        return false
    checkIt(gen)

def test_for_break
    let gen <- generator<int>() <|
        for t in range(100)
            if t == 10
                break
            yield t
        return false
    checkIt(gen)

def test_for_continue
    let gen <- generator<int>() <|
        for t in range(100)
            if t >= 10
                continue
            yield t
        return false
    checkIt(gen)

def test_for_nested
    let gen <- generator<int>() <|
        for q in range(2)
            for t in range(5)
                yield q*5 + t
        return false
    checkIt(gen)

def test_for_finally
    let gen <- generator<int>() <|
        for t in range(9)
            yield t
        finally
            yield 9
        return false
    checkIt(gen)

[export]
def test
    test_yield()
    test_local()
    test_local_ref()
    test_if()
    test_while()
    test_while_break()
    test_while_continue()
    test_while_nested()
    test_while_finally()
    test_for()
    test_for_break()
    test_for_continue()
    test_for_nested()
    test_for_finally()
    return true

