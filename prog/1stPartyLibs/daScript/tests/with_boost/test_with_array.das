options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/with_boost

struct A {
    f1 : int
    f2 : int
}

[test]
def test_single_arg_default_name(t : T?) {
    //! `with_(arr[i]) { _.field = ... }` — no block params, `_` injected,
    //! mutation persists in the underlying array.
    var arr <- [A(f1=1, f2=2), A(f1=3, f2=4)]
    with_(arr[0]) {
        _.f1 = 99
    }
    t |> equal(arr[0].f1, 99, "default _ mutation persists in arr[0]")
    t |> equal(arr[0].f2, 2, "unrelated field untouched")
    t |> equal(arr[1].f1, 3, "other element untouched")
    delete arr
}

[test]
def test_single_arg_named_binding(t : T?) {
    //! `with_(arr[i]) $(myA) { myA.field = ... }` — user-named param,
    //! constness stripped so mutation persists.
    var arr <- [A(f1=10, f2=20)]
    with_(arr[0]) $(myA) {
        myA.f1 = 111
        myA.f2 = 222
    }
    t |> equal(arr[0].f1, 111, "named binding mutation persists")
    t |> equal(arr[0].f2, 222, "second-field mutation persists")
    delete arr
}

[test]
def test_two_arg_array(t : T?) {
    //! Multi-arg form: two arrays bound to two block params,
    //! mutations persist in both.
    var src <- [A(f1=100, f2=200)]
    var dst <- [A(f1=0, f2=0)]
    with_(dst[0], src[0]) $(d, s) {
        d.f1 = s.f1 + 1
        d.f2 = s.f2 + 2
    }
    t |> equal(dst[0].f1, 101, "dst.f1 = src.f1 + 1")
    t |> equal(dst[0].f2, 202, "dst.f2 = src.f2 + 2")
    delete src
    delete dst
}

[test]
def test_three_arg_array(t : T?) {
    //! Arity 3: three arrays, three block params.
    var a <- [A(f1=0, f2=0)]
    var b <- [A(f1=10, f2=20)]
    var c <- [A(f1=100, f2=200)]
    with_(a[0], b[0], c[0]) $(va, vb, vc) {
        va.f1 = vb.f1 + vc.f1
        va.f2 = vb.f2 + vc.f2
    }
    t |> equal(a[0].f1, 110, "sum across three arrays in f1")
    t |> equal(a[0].f2, 220, "sum across three arrays in f2")
    delete a
    delete b
    delete c
}

struct Owner {
    children : array<A>
}

[test]
def test_field_chain_container(t : T?) {
    //! Container is `obj.field[i]` — subexpr is the ExprField
    //! `obj.children`, an lvalue chain rooted in `obj` (ExprVar).
    //! The lvalue-chain check passes and pre-bind works.
    var obj <- Owner(children <- [A(f1 = 5, f2 = 6), A(f1 = 7, f2 = 8)])
    with_(obj.children[1]) $(elem) {
        elem.f1 = 555
    }
    t |> equal(obj.children[1].f1, 555, "second-element mutation persists")
    t |> equal(obj.children[0].f1, 5, "first element untouched")
    delete obj.children
}

def helper_set_f1(var aRef : A&; v : int) {
    aRef.f1 = v
}

[test]
def test_pass_binding_to_function(t : T?) {
    //! The block-arg can be passed to another function expecting a
    //! mutable ref; mutations through that path also persist.
    var arr <- [A(f1=1, f2=2)]
    with_(arr[0]) $(a) {
        helper_set_f1(a, 777)
    }
    t |> equal(arr[0].f1, 777, "mutation through helper-fn ref persists")
    delete arr
}
