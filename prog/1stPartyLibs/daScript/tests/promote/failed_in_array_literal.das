// out-of-range int literal in array element position
// first element fixes array<uint8>; 256 in element 2 triggers promotion range check
options gen2

expect 30515

[export]
def test {
    let a <- [uint8(0), 100, 256, 200]
    return a[2] == 0u8
}
