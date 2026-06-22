// out-of-range literal via copy (=) — promotion's range error must suppress cant_copy
options gen2

expect 30515

[export]
def test {
    var d : uint8
    d = 256
    return true
}
