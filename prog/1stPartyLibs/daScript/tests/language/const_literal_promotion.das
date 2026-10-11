options gen2
options infer_time_folding = true

// In a binary operator a literal adapts to the other operand's type, so `a - 1` reads
// like `a - 1u`; `-1` and `(1 + 1)` are built from literals and adapt the same way
// once infer-time folding collapses them, which this file pins on. A constant folded
// from a typed expression (clz(0xFF), 1u + 1u) keeps its declared type and never
// adapts: the literal side moves.

require dastest/testing_boost public

let CONST_BITS = 0xFF

def typed_uint_on_right(bits : uint) : uint {
    return 31 - clz(bits)
}

def typed_uint_on_left(bits : uint) : uint {
    return clz(bits) - 31
}

def minus_one(a : auto(T)) {
    return a - 1
}

[test]
def test_folded_constant_keeps_its_type(t : T?) {
    t |> run("folded call result keeps uint, literal adapts") @(t : T?) {
        t |> equal(typeinfo typename(31 - clz(0xFF)), "uint const")
        t |> equal(typeinfo typename(clz(0xFF) - 31), "uint const")
        t |> equal(typeinfo typename(31 - clz(CONST_BITS)), "uint const")
        t |> equal(31 - clz(0xFF), 7u)
    }
    t |> run("runtime operand behaves the same as the folded one") @(t : T?) {
        t |> equal(typeinfo typename(typed_uint_on_right(0xFF)), "uint const")
        t |> equal(typed_uint_on_right(0xFF), 7u)
        t |> equal(typeinfo typename(typed_uint_on_left(0xFF)), "uint const")
        t |> equal(typed_uint_on_left(0xFF), 0xfffffff9u)
    }
}

[test]
def test_bare_literal_still_adapts(t : T?) {
    t |> run("literal promotes toward the typed operand") @(t : T?) {
        let ua = 100u
        t |> equal(typeinfo typename(ua - 1), "uint const")
        t |> equal(ua - 1, 99u)
        t |> equal(typeinfo typename(31u - 1), "uint const")
        t |> equal(31u - 1, 30u)
    }
    t |> run("negated and parenthesized literals still promote") @(t : T?) {
        let l = 5l
        let f = 2.0
        let d = 2.0lf
        let u = 7u
        t |> equal(l + -1, 4l)
        t |> equal(f * -1, -2.0)
        t |> equal(d * -2, -4.0lf)
        t |> success(l > -1)
        t |> equal(l & ~1, 4l)
        t |> equal(u * (1 + 1), 14u)
        t |> equal(l + (1 << 2), 9l)
    }
    t |> run("two literals keep the existing int result") @(t : T?) {
        t |> equal(typeinfo typename(31 - 5u), "int const")
        t |> equal(31 - 5u, 26)
        t |> equal(typeinfo typename(31 - (1u + 1u)), "int const")
        t |> equal(31 - (1u + 1u), 29)
    }
    t |> run("a generic instance keeps the literal mark through the body clone") @(t : T?) {
        t |> equal(minus_one(5u), 4u)
        t |> equal(minus_one(5l), 4l)
    }
}
