options gen2
require dastest/testing_boost

require UnitTest

// doubleSamplePoint3 is a C++-bound extern taking `const Point3 &` — an implicit const&
// parameter. A named-variable argument works (a variable read is a ref); an rvalue such as
// `doubleSamplePoint3(a + b)` is a compile error by design ("can't pass non-ref to ref") —
// das deliberately has no materialization of value arguments to implicit const& parameters.
//
// The inliner, however, MANUFACTURES that shape: grafting `wrapped(a + b)` substitutes an
// rvalue expression for `p`, and the body hands `p` to the const& extern. The inliner must
// bind storage (a generated `__inlN_arg_p` temp local) instead of substituting the bare
// value. These tests pin that behavior — interp and JIT.

[inline]
def wrapped(p : float3) : float3 {
    return UnitTest::doubleSamplePoint3(p)
}

[inline]
def wrapped_chain(p : float3) : float3 {
    // two inline levels: the graft of this body hands another rvalue to `wrapped`
    return wrapped(p + float3(1, 1, 1))
}

def double_sum(a, b : float3) => wrapped(a + b)

[test]
def test_inlined_rvalue_to_const_ref(t : T?) {
    t |> run("inlined wrapper binds storage for an rvalue argument") @(t : T?) {
        let a = float3(1, 2, 3)
        let b = float3(1, 0, 0)
        t |> equal(wrapped(a + b), float3(4, 4, 6))
        t |> equal(wrapped(float3(1, 2, 3)), float3(2, 4, 6))       // constant-folded argument
        t |> equal(wrapped_chain(a + a), float3(6, 10, 14))
        t |> equal(double_sum(a, b), float3(4, 4, 6))
    }
}
