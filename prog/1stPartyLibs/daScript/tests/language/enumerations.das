options gen2
require dastest/testing_boost public

require daslib/enum_trait

enum Foo {
    one
    two
    three
}

[test]
def test_enum_conversions(t : T?) {
    t |> run("enum to string") @(t : T?) {
        let s = "{Foo.two}"
        t |> equal("two", s)
        let q = string(Foo.two)
        t |> equal("two", q)
    }
    t |> run("string to enum") @(t : T?) {
        let q = to_enum(type<Foo>, "two")
        t |> equal(Foo.two, q)
        let r = to_enum(type<Foo>, "five", Foo.three)
        t |> equal(Foo.three, r)
    }
}


[string_to_enum]
enum Bar {
    one
    two
    three
}

[test]
def test_enum_macro_conversions(t : T?) {
    t |> run("enum macro") @(t : T?) {
        let b = Bar("three")                // this conversion can panic
        t |> equal(Bar.three, b)
        let s = "{b}"
        t |> equal("three", s)
        let q = string(Bar.two)
        t |> equal("two", q)
        let r = to_enum(type<Bar>, "one")   // conversion via standard path
        t |> equal(Bar.one, r)
        let bb = Bar("five", Bar.two)       // conversion with the default value
        t |> equal(Bar.two, bb)
    }
}

// Regression: a user-defined operator over two DIFFERENT enums must be honored.
// The "operations on different enumerations are prohibited" gate now runs only
// AFTER operator-overload lookup fails (reportOp2Errors), so this overload wins.
def operator |(a : Foo; b : Bar) : Foo {
    return unsafe(reinterpret<Foo>(int(a) | int(b)))
}

[test]
def test_cross_enum_operator(t : T?) {
    t |> run("user operator | combines two different enums") @(t : T?) {
        let r = Foo.two | Bar.three     // 1 | 2 == 3
        t |> equal(3, int(r))
    }
}

