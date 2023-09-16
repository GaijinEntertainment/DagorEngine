[export]
def test:bool
    let a = [[int[1] 1234]]
    for x in a
        assert(x==1234)
    return true
