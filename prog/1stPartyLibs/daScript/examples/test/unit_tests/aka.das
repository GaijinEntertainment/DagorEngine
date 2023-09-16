
def test_aka_func ( var x aka X : int )
    let tx = X
    assert(tx==x)

def test_aka
    var x aka X = 5
    X = 13
    assert(x==13)

def call_block ( blk : block<(arg:int):void> )
    invoke(blk, 5)

[export]
def test
    test_aka()
    test_aka_func(5)
    call_block <| $ ( x aka X )
        let tx = x
        assert(tx==X)
    var tot = 0
    var same = 0
    for x aka X in range(10)        // think nested DECS
        for xx in range(10)
            tot ++
            if xx==X
                same ++
    assert(tot==100)
    assert(same==10)
    return true


