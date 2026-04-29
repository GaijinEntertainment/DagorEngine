options gen2
require dastest/testing_boost public

struct A {
    a : int
}

struct BB {
    b : int
}

variant Foo {
    a : A
    b : BB
}

let foo <- fixed_array<Foo>(
    Foo(a = A(a = 1)),
    Foo(b = BB(b = 2))
);

[test]
def test_multiple_variants(t : T?) {
    t |> success(foo[0] is a)
    if (foo[0] is a) {
        t |> equal(1, (foo[0] as a).a)
    }
    t |> success(foo[1] is b)
    if (foo[1] is b) {
        t |> equal(2, (foo[1] as b).b)
    }
}
