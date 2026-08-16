options gen2
options auto_inline_functions = true
require dastest/testing_boost
require UnitTest

// REPRO (crashes the compiler before the fix, see below).
//
// doubleSamplePoint3 is bound from C++ as `Point3 doubleSamplePoint3(const Point3 &)`, so its
// parameter is an implicit const ref.
//
// `private` + referenced once makes this callee budget-exempt, so it splices even though the body is
// over the heuristic budget. The splice substitutes the caller's argument EXPRESSION for `p`, and the
// caller passes `a + b` built from two const locals - which constant-folds. So the folder ends up
// evaluating doubleSamplePoint3(<constant>) at compile time and dereferences a `const Point3 &` that
// was never materialized:
//
//   CRASH: SIGSEGV
//     das::FoldingVisitor::evalAndFold -> das::Context::evalWithCatch -> doubleSamplePoint3
//
// Same family as fold_extern_at_compile.das - the compiler must not execute a C++-bound function - but
// this one crashes rather than baking a wrong constant, so it is the sharper case. Dagor closes it by
// refusing to fold builtIn (C++-bound) calls at all.
//
// A non-constant argument (two globals instead of const locals) does NOT crash: the value binds and
// runs fine. So the trigger is specifically inlining + constant propagation + a native const-ref param.
def private big_callee(p : float3; k : float) : float3 {
    var acc = float3()
    var i = 0
    while (i < 2) { acc += float3(float(i)); i++ }
    let scaled = acc * k
    let doubled = UnitTest::doubleSamplePoint3(p)
    var out = doubled + scaled
    out.x += 0.
    return out
}

[test]
def test_no_compile_time_call_of_native_const_ref(t : T?) {
    let a = float3(1, 2, 3)
    let b = float3(1, 0, 0)
    t |> equal(big_callee(a + b, 0.), float3(4, 4, 6))
}
