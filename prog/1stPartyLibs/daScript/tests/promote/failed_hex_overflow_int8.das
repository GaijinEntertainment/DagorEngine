// 0xFF (255) doesn't fit in int8 (range -128..127) — hex literals are uint, promote-into-int8 range-checks
options gen2

expect 30515

[export]
def test {
    var d : int8 = 0xFF
    return true
}
