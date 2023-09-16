[sideeffects]
def basic_goto()
    var fail = false
    var arr : int[10]
    var i = 0
    label 0:
    arr[i] = i
    i ++
    if i < 10
        goto label 0
        fail = true
        assert(false,"unreachable code")    // not a static assert without optimizations
    for t in range(10)
        assert(arr[t]==t)
    assert(!fail)

[sideeffects]
def nested_goto()
    var fail = false
    var arr : int[10]
    var i = 0
    label 0:
    arr[i] = i
    i ++
    if i < 10
        if i < 10
            goto label 0
            fail = true
        fail = true
    for t in range(10)
        assert(arr[t]==t)

[sideeffects]
def for_goto()
    var fail = false
    var arr : int[10]
    for j in range(10)
        var i = 0
        label 0:
        arr[i] = i + j
        i ++
        if i < 10
            goto label 0
            fail = true
        for t in range(10)
            assert(arr[t]==t + j)
        assert(!fail)
    for t in range(10)
        assert(arr[t]==t + 9)
    assert(!fail)

[sideeffects]
def while_goto()
    var fail = false
    var arr : int[10]
    var j = 0
    while j < 10
        var i = 0
        label 0:
        arr[i] = i + j
        i ++
        if i < 10
            goto label 0
            fail = true
        for t in range(10)
            assert(arr[t]==t + j)
        assert(!fail)
        j ++
    for t in range(10)
        assert(arr[t]==t + 9)
    assert(!fail)

[sideeffects]
def finally_goto(yes:bool = true)
    var fail = true
    if yes
        fail = true
        goto label 0
    finally
        fail = false
    label 0:
    assert(!fail)

[sideeffects]
def gotoN(n:int)
    var res = -1
    goto n
    label 0:
    res = 0
    goto label 3
    label 1:
    res = 1
    goto label 3
    label 2:
    res = 2
    goto label 3
    label 3:
    return res

[sideeffects]
def N_goto()
    verify(gotoN(0)==0)
    verify(gotoN(1)==1)
    verify(gotoN(2)==2)
    verify(gotoN(3)==-1)

[export]
def test()
    basic_goto()
    nested_goto()
    for_goto()
    while_goto()
    finally_goto()
    N_goto()
    return true
