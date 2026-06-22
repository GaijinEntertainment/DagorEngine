// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins value semantics: assignment, init-copy, clone, zero-init, returns,
// struct-field copies, and non-copyable element handling.
options gen2
require dastest/testing_boost public

struct WithFA {
    name : string
    data : float[3]
}

def make_iota() : int[4] {
    var r : int[4]
    for (i in range(4)) {
        r[i] = i + 1
    }
    return r
}

[test]
def test_copy_semantics(t : T?) {
    t |> run("whole-array assignment copies") @(t : T?) {
        var a = make_iota()
        var b : int[4]
        b = a
        a[0] = 100
        t |> equal(b[0], 1)         // deep copy, not aliased
        t |> equal(b[3], 4)
    }
    t |> run("init-copy from variable") @(t : T?) {
        var a = make_iota()
        let c = a
        a[1] = 200
        t |> equal(c[1], 2)
    }
    t |> run("clone operator") @(t : T?) {
        let a = make_iota()
        var d : int[4]
        d := a
        t |> equal(d[2], 3)
    }
    t |> run("clone() builtin returns FA") @(t : T?) {
        var a = make_iota()
        let e = clone(a)
        t |> equal(e[3], 4)
        a[3] = 0
        t |> equal(e[3], 4)
    }
    t |> run("2D assignment") @(t : T?) {
        var m : float[2][2]
        m[0][0] = 1.0
        m[1][1] = 2.0
        var n : float[2][2]
        n = m
        t |> equal(n[0][0], 1.0)
        t |> equal(n[1][1], 2.0)
    }
}

[test]
def test_init_and_return(t : T?) {
    t |> run("zero-initialized by default") @(t : T?) {
        var a : int[8]
        var sum = 0
        for (x in a) {
            sum += x
        }
        t |> equal(sum, 0)
    }
    t |> run("fixed_array literal types") @(t : T?) {
        t |> equal(typeinfo typename(fixed_array(10, 20, 30)), "int[3]")
        let fa = fixed_array(10, 20, 30)
        t |> equal(fa[1], 20)
        t |> equal(typeinfo typename(fixed_array<float>(1.5, 2.5)), "float[2]")
        let fa2 = fixed_array<float>(1.5, 2.5)
        t |> equal(fa2[1], 2.5)
    }
    t |> run("function returns FA by value") @(t : T?) {
        let r = make_iota()
        t |> equal(r[0], 1)
        t |> equal(r[3], 4)
    }
    t |> run("struct with FA field copies deep") @(t : T?) {
        var s1 = WithFA(name = "x")
        s1.data[1] = 5.0
        let s2 = s1
        s1.data[1] = 9.0
        t |> equal(s2.data[1], 5.0)
    }
}

[test]
def test_noncopyable_elements(t : T?) {
    t |> run("FA of array<int>: clone + finalize_dim") @(t : T?) {
        var na : array<int>[3]
        na[0] |> push(1)
        na[0] |> push(2)
        na[2] |> push(9)
        var nb : array<int>[3]
        nb := na
        t |> equal(length(nb[0]), 2)
        t |> equal(nb[2][0], 9)
        na[0][0] = 100
        t |> equal(nb[0][0], 1)     // deep clone
        finalize_dim(na)
        t |> equal(length(na[0]), 0)
        t |> equal(length(na[2]), 0)
        finalize_dim(nb)
    }
}
