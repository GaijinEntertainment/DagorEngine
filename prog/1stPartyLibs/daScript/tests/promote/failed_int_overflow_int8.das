// 128 doesn't fit in int8 (range -128..127)
options gen2

expect 30515

[export]
def test {
    var d : int8 = 128
    return true
}
