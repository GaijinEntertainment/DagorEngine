// 256 doesn't fit in uint8 — emits exactly one exceeds_constant_range
options gen2

expect 30515

[export]
def test {
    var d : uint8 = 256
    return true
}
