bitfield bits123
    one
    two
    three

[sideeffects]
def bit(b:int)
    return bitfield(1 << b)

[export]
def test()
    var t : bitfield < one; two; three > = bitfield(1<<1) | bit(2)
    assert(!t.one && t.two && t.three)
    assert("{t}"=="(two|three)")
    t ^= bitfield(1<<1)
    assert(!t.one && !t.two && t.three)
    assert(t==bits123 three)
    t |= bitfield(1<<0)
    assert(t.one && !t.two && t.three)
    return true
