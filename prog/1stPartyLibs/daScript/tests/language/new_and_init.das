options gen2
// options log=true, log_nodes=true, log_stack=true
options always_export_initializer = true

require UnitTest
require dastest/testing_boost public

[safe_when_uninitialized]
struct Foo {
    x : int = 1
    y : int = 2
    z : int = 3
    w : int = 4
}

[test]
def test_new_and_init(t : T?) {
    var f : Foo
    let q : Foo? = unsafe(reinterpret<Foo?> new_and_init(f))
    t |> success(q.x == 1 && q.y == 2 && q.z == 3 && q.w == 4)
}


