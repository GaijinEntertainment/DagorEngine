expect 30101:2, 30108:1    // invalid_type

def test(A:void&)       // 30101: can't be void reference
    let b:int & &       // 30108: local reference needs to be initialized
    debug(b)
    let c:int & ?       // 30101: can't be pointer to reference
    debug(c)
