// 65536 doesn't fit in uint16 (range 0..65535)
options gen2

expect 30515

[export]
def test {
    var d : uint16 = 65536
    return true
}
