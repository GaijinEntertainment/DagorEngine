options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── module-qualified call names ─────────────────────────────────────────
// A resolved/parsed call may carry a qualified name (`math::sin`); an
// unqualified pattern name matches its last `::` segment via
// qm_call_name_matches. The suffix must be segment-aligned.

[test]
def test_unqualified_pattern_vs_qualified_call(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(math::sin(1.0))
        var arg : ExpressionPtr
        let r = qmatch(expr, sin($e(arg)))
        t |> success(r.matched, "bare `sin` should match `math::sin`")
        if (r.matched) {
            t |> equal("{arg.__rtti}", "ExprConstFloat")
        }
    }
}

[test]
def test_qualified_pattern_vs_qualified_call(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(math::sin(1.0))
        var arg : ExpressionPtr
        let r = qmatch(expr, math::sin($e(arg)))
        t |> success(r.matched, "exact qualified name should still match")
    }
}

[test]
def test_wrong_name_fails(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(math::sin(1.0))
        var arg : ExpressionPtr
        let r = qmatch(expr, cos($e(arg)))
        t |> success(!r.matched, "`cos` must not match `math::sin`")
    }
}

[test]
def test_suffix_is_segment_aligned(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(foosin(1.0))
        var arg : ExpressionPtr
        let r = qmatch(expr, sin($e(arg)))
        t |> success(!r.matched, "`sin` must not match `foosin` (not a `::` segment)")
    }
}

[test]
def test_qualified_pattern_requires_exact(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(sin(1.0))
        var arg : ExpressionPtr
        let r = qmatch(expr, math::sin($e(arg)))
        t |> success(!r.matched, "qualified pattern must not match a bare call")
    }
}
