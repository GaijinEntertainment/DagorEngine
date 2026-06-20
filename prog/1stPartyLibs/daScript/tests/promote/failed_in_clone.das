// out-of-range literal via clone (:=) — promotion's range error must suppress mismatching_clone_type
options gen2

expect 30515

[export]
def test {
    var d : uint8
    d := 256
    return true
}
