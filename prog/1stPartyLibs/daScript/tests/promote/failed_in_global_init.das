// out-of-range global variable init
options gen2

expect 30515

var g : uint8 = 256

[export]
def test {
    return g == 0u8
}
