expect 30305

def test
    let a : int
    let a : int // 30305: local variable a is shadowed by another local variable a : int at line 4
    debug(a)
