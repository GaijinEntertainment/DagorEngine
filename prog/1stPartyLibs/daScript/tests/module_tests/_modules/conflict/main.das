options gen2

require pkgA/foo
require pkgB/foo

[export]
def test() {
    let a = hello_a()
    let b = hello_b()
    assert(a == "a")
    assert(b == "b")
}
