// out-of-range literal in compound assignment — promotion's range error must suppress mismatching_numeric_type
options gen2

expect 30515

[export]
def test {
    var d : uint8 = 0
    d += 256
    return true
}
