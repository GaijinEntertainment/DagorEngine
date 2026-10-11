// out-of-range value in a variant arm clone-initializer
options gen2

expect 30515

variant V {
    a : uint8
}

[export]
def test {
    var v = V(a := 256)
    return true
}
