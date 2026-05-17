options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public
require _qmatch_r2v_helper

// ============================================================================
// ExprRef2Value transparency tests
//
// Post-inference, the typer wraps reference reads in ExprRef2Value(inner). The
// wrapper is invisible to user-written code, so the matcher peels it on both
// the source side (runtime) and the pattern side (compile time). These tests
// build the wrapper manually via `new ExprRef2Value(...)` since daslang has no
// surface syntax for it.
//
// Pattern-side coverage is exercised via `test_emit_qmatch_with_wrapped_pattern`
// in `_qmatch_r2v_helper.das`, a [call_macro] that hand-builds an
// `ExprCallMacro(name="qmatch")` whose pattern argument is wrapped in
// `ExprRef2Value`. This is the realistic macro-composition shape that motivates
// the pattern-side peel.
//
// ExprPtr2Ref (`*foo`) is intentionally NOT peeled — those tests pin the
// design decision so a future "consistency" refactor doesn't accidentally
// equate `*foo` with `foo`.
// ============================================================================

[test]
def test_source_wrapped_field(t : T?) {
    ast_gc_guard() {
        var inner = qmacro(a.foo)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        let r = qmatch(wrapped, a.foo)
        t |> success(r.matched, "ExprRef2Value(a.foo) should match pattern a.foo")
    }
}

[test]
def test_source_wrapped_var(t : T?) {
    ast_gc_guard() {
        var inner = qmacro(somevar)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        let r = qmatch(wrapped, somevar)
        t |> success(r.matched, "ExprRef2Value(somevar) should match pattern somevar")
    }
}

[test]
def test_source_wrapped_op(t : T?) {
    ast_gc_guard() {
        var inner = qmacro(a + b)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        let r = qmatch(wrapped, a + b)
        t |> success(r.matched, "ExprRef2Value(a + b) should match pattern a + b")
    }
}

[test]
def test_nested_wrappers(t : T?) {
    ast_gc_guard() {
        var inner = qmacro(a.foo)
        var w1 : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        var w2 : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = w1)
        var w3 : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = w2)
        let r = qmatch(w3, a.foo)
        t |> success(r.matched, "triple-wrapped ExprRef2Value should peel down to a.foo")
    }
}

[test]
def test_capture_e_peels_wrapper(t : T?) {
    // $e(var) capture must yield the underlying expression, not the wrapper —
    // downstream callers check `captured is ExprField`, not `is ExprRef2Value`.
    ast_gc_guard() {
        var inner = qmacro(target.col)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        var captured : ExpressionPtr
        let r = qmatch(wrapped, $e(captured))
        t |> success(r.matched, "$e on wrapped expression should match")
        if (r.matched) {
            t |> equal(string(captured.__rtti), "ExprField",
                       "captured value should be the inner ExprField, not ExprRef2Value")
        }
    }
}

[test]
def test_capture_f_peels_wrapper(t : T?) {
    // The hand-rolled is_underscore_field in sqlite_linq existed to do exactly
    // this: peel ExprRef2Value, then capture the field name. With auto-peel,
    // the matcher does it directly.
    ast_gc_guard() {
        var inner = qmacro(a.colname)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = inner)
        var fname = ""
        let r = qmatch(wrapped, a.$f(fname))
        t |> success(r.matched, "a.$f(...) should match through ExprRef2Value wrapper")
        t |> equal(fname, "colname", "should capture field name through wrapper")
    }
}

[test]
def test_clean_source_still_matches(t : T?) {
    // Regression — peeling logic must not break the no-wrapper case.
    ast_gc_guard() {
        var expr = qmacro(a.foo)
        let r = qmatch(expr, a.foo)
        t |> success(r.matched, "unwrapped source should still match")
    }
}

// ── ExprPtr2Ref NOT auto-peeled (negative tests) ─────────────────────────

[test]
def test_ptr2ref_not_peeled(t : T?) {
    // *foo is user-visible — should NOT match a pattern of bare foo.
    ast_gc_guard() {
        var inner = qmacro(a.foo)
        var wrapped : ExpressionPtr = new ExprPtr2Ref(at = inner.at, subexpr = inner)
        let r = qmatch(wrapped, a.foo)
        t |> success(!r.matched, "ExprPtr2Ref(a.foo) must NOT match pattern a.foo")
    }
}

[test]
def test_ref2value_around_ptr2ref(t : T?) {
    // Mixed: ExprRef2Value(ExprPtr2Ref(inner)). The Ref2Value layer peels
    // away, but the Ptr2Ref must remain — so a bare-inner pattern still fails.
    ast_gc_guard() {
        var inner = qmacro(a.foo)
        var ptr2ref : ExpressionPtr = new ExprPtr2Ref(at = inner.at, subexpr = inner)
        var wrapped : ExpressionPtr = new ExprRef2Value(at = inner.at, subexpr = ptr2ref)
        let r = qmatch(wrapped, a.foo)
        t |> success(!r.matched, "ExprRef2Value(ExprPtr2Ref(...)) must not match unwrapped pattern")
    }
}

// ── Pattern-side peel (via helper macro that emits qmatch with wrapped pattern) ──

[test]
def test_pattern_side_wrapper_peeled(t : T?) {
    // The helper macro emits: qmatch(<source>, ExprRef2Value(qmacro(a.foo))).
    // generate_match's pattern-side peel must strip the wrapper so a clean
    // a.foo source still matches.
    ast_gc_guard() {
        var src = qmacro(a.foo)
        let r = test_emit_qmatch_with_wrapped_pattern(src)
        t |> success(r.matched, "wrapped pattern (ExprRef2Value(a.foo)) should peel and match clean a.foo source")
    }
}
