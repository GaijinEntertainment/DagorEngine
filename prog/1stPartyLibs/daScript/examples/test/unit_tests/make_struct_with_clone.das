options gen2
require daslib/strings_boost

struct Foo {
    a : array<int>
    b : string
}

[export]
def test {
    let sa <- [1, 2, 3, 4]
    let sb <- [1, 2, 3]
    let fa <- Foo(a := sa)
    let fa2 <- [Foo(a := sa), Foo(a := sb)]
    assert(length(sa) == 4)
    assert(length(fa.a) == 4)
    assert(length(fa2[0].a) == 4)
    assert(length(fa2[1].a) == 3)
    var faw <- Foo(a := sa, b = "a is ")
    faw.b += join(faw.a, "-")
    assert(length(faw.a) == 4)
    assert(faw.b == "a is 1-2-3-4")
    return true
}