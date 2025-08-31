options gen2
[export]
def test : bool {
    for (x in fixed_array<int>(1, 1)) {
        assert(x == 1)
    }
    return true
}
