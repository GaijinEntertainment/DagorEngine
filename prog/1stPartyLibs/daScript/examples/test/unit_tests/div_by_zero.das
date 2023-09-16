[sideeffects]
def div_by_zero(num:auto(numT))
    var ex = false
    try
        return num / numT(0)
    recover
        ex = true
    assert(ex)
    return num

[sideeffects]
def mod_by_zero(num:auto(numT))
    var ex = false
    try
        return num % numT(0)
    recover
        ex = true
    assert(ex)
    return num

[export]
def test()
    // int
    verify(div_by_zero(1)==1)
    verify(mod_by_zero(1)==1)
    // uint
    verify(div_by_zero(1u)==1u)
    verify(mod_by_zero(1u)==1u)
    // int64
    verify(div_by_zero(1l)==1l)
    verify(mod_by_zero(1l)==1l)
    // uint64
    verify(div_by_zero(1ul)==1ul)
    verify(mod_by_zero(1ul)==1ul)
    return true

