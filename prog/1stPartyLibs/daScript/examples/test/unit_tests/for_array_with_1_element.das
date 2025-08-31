options gen2
[export]
def test : bool {
    let a = [ 1234]
    for (x in a) {
        assert(x == 1234)
    }
    return true
}
