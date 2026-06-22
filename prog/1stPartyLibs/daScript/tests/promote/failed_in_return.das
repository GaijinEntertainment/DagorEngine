// out-of-range literal in return — promotion's range error must suppress invalid_return_type
options gen2

expect 30515

def f() : uint8 {
    return 256
}

[export]
def test {
    return true
}
