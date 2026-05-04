options gen2
require dastest/testing_boost

def nullc(a, b) {
    return a ?? b
}

def nullc_ref(var a : Foo?; var b : Foo; c : int) {
    (a ?? b).a = c
}

struct Foo {
    a, b : int
}

[test]
def test_null_coalescing(t : T?) {
    t |> run("null coalescing value") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : int?; b : int) : void > nullc))
        var i = 13
        var pi = unsafe(addr(i))
        var pj : int?
        var n1 = nullc(pi, 1)
        var n2 = nullc(pj, 2)
        t |> equal(13, n1)
        t |> equal(2, n2)
    }
    t |> run("null coalescing ref") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@nullc_ref))
        var f = Foo(a = 13)
        var pf = unsafe(addr(f))
        var pn : Foo?
        var df = Foo(a = 1)
        nullc_ref(pf, df, 42)
        t |> equal(42, f.a)
        t |> equal(1, df.a)
        nullc_ref(pn, df, 33)
        t |> equal(33, df.a)
    }
}

def bOrD(f : Foo?; d : int) {
    return f?.b ?? d
}

struct Bar {
    f : Foo?
}

def bOrD2(b : Bar?; d : int) {
    return b?.f?.b ?? d
}

[test]
def test_safe_field(t : T?) {
    t |> run("safe_field ptr to structure") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@bOrD))
        var f = Foo(a = 1, b = 2)
        var pF : Foo? = unsafe(addr(f))
        var pN : Foo?
        t |> equal(2, bOrD(pF, 3))
        t |> equal(4, bOrD(pN, 4))
    }
    t |> run("safe_field ptr to structure field with ptr") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@bOrD2))
        var f = Foo(a = 1, b = 2)
        var bF = Bar(f = unsafe(addr(f)))
        var bN = Bar()
        t |> equal(2, bOrD2(unsafe(addr(bF)), 3))
        t |> equal(4, bOrD2(unsafe(addr(bN)), 4))
    }
}

def jit_safe_at(a, b, c) {
    return a?[b] ?? c
}

[test]
def test_safe_at(t : T?) {
    t |> run("safe_at dim") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : int const[3]; b, c : int) : int > jit_safe_at))
        let i = fixed_array(1, 2, 3)
        let v1 = jit_safe_at(i, 0, 42)
        let v2 = jit_safe_at(i, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
    }
    t |> run("safe_at ptr to dim") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : int const[3]?; b, c : int) : int > jit_safe_at))
        let i = fixed_array(1, 2, 3)
        let pi = unsafe(addr(i))
        let p0 : int[3]?
        let v1 = jit_safe_at(pi, 0, 42)
        let v2 = jit_safe_at(p0, 0, 42)
        let v3 = jit_safe_at(pi, 3, 42)
        let v4 = jit_safe_at(p0, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
        t |> equal(42, v3)
        t |> equal(42, v4)
    }
    t |> run("safe_at array") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : array<int>; b, c : int) : int > jit_safe_at))
        let i <- [ 1, 2, 3]
        let v1 = jit_safe_at(i, 0, 42)
        let v2 = jit_safe_at(i, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
    }
    t |> run("safe_at ptr to array") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : array<int> const?; b, c : int) : int > jit_safe_at))
        let i <- [ 1, 2, 3]
        let pi = unsafe(addr(i))
        let p0 : int[3]?
        let v1 = jit_safe_at(pi, 0, 42)
        let v2 = jit_safe_at(p0, 0, 42)
        let v3 = jit_safe_at(pi, 3, 42)
        let v4 = jit_safe_at(p0, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
        t |> equal(42, v3)
        t |> equal(42, v4)
    }
    t |> run("safe_at table") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : table<int; int>; b, c : int) : int > jit_safe_at))
        let i <- {0 => 1, 1 => 2, 2 => 3}
        let v1 = jit_safe_at(i, 0, 42)
        let v2 = jit_safe_at(i, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
    }
    t |> run("safe_at ptr to table") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a : table<int; int> const?; b, c : int) : int > jit_safe_at))
        let i <- {0 => 1, 1 => 2, 2 => 3}
        let pi = unsafe(addr(i))
        let p0 : int[3]?
        let v1 = jit_safe_at(pi, 0, 42)
        let v2 = jit_safe_at(p0, 0, 42)
        let v3 = jit_safe_at(pi, 3, 42)
        let v4 = jit_safe_at(p0, 3, 42)
        t |> equal(1, v1)
        t |> equal(42, v2)
        t |> equal(42, v3)
        t |> equal(42, v4)
    }
}

variant IOrF {
    i : int
    f : float
}

def safe_as_i(a : IOrF; b : int) {
    return (a ?as i) ?? b
}

def safe_as_pi(a : IOrF?; b : int) {
    return (a ?as i) ?? b
}

[test]
def test_safe_variant_as(t : T?) {
    t |> run("safe variant as") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@safe_as_i))
        var i = IOrF(i = 1)
        var f = IOrF(f = 1.)
        t |> equal(1, safe_as_i(i, 42))
        t |> equal(42, safe_as_i(f, 42))
    }
    t |> run("safe variant ptr as") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@safe_as_pi))
        var i = IOrF(i = 1)
        var f = IOrF(f = 1.)
        var pi = unsafe(addr(i))
        var pf = unsafe(addr(f))
        var p0 : IOrF?
        t |> equal(1, safe_as_pi(pi, 42))
        t |> equal(42, safe_as_pi(pf, 42))
        t |> equal(42, safe_as_pi(p0, 42))
    }
}

