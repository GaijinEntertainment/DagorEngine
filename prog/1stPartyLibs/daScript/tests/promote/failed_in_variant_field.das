// out-of-range value in variant arm constructor
options gen2

expect 30515

variant V {
    a : uint8
}

[export]
def test {
    var v = V(a = 256)
    return true
}
