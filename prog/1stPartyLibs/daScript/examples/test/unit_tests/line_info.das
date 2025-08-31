options gen2
require UnitTest

[export]
def test {
    let line = testCallLine()
    assert(line == 6 || line == 0)// 0 with aot
    return true
}
