options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

struct MscFoo {
    a : array<int>
    b : string
}

[test]
def test_make_struct_with_clone(t : T?) {
    t |> run("struct with cloned array field") @(t : T?) {
        let sa <- [1, 2, 3, 4]
        let sb <- [1, 2, 3]
        let fa <- MscFoo(a := sa)
        let fa2 <- [MscFoo(a := sa), MscFoo(a := sb)]
        t |> equal(length(sa), 4)
        t |> equal(length(fa.a), 4)
        t |> equal(length(fa2[0].a), 4)
        t |> equal(length(fa2[1].a), 3)
    }
    t |> run("struct clone with string builder") @(t : T?) {
        let sa <- [1, 2, 3, 4]
        var faw <- MscFoo(a := sa, b = "a is ")
        faw.b += join(faw.a, "-")
        t |> equal(length(faw.a), 4)
        t |> equal(faw.b, "a is 1-2-3-4")
    }
}
