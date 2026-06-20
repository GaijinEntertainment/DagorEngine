// -1 doesn't fit in uint8 — uint is unsigned
options gen2

expect 30515

[export]
def test {
    var d : uint8 = -1
    return true
}
