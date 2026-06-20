// 256 doesn't fit in bitfield8
options gen2

expect 30515

[export]
def test {
    var d : bitfield8 = 256
    return true
}
