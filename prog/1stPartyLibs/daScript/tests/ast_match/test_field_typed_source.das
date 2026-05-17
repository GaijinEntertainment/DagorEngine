options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ============================================================================
// ExprField typed-source matching
//
// `qm_skip_field` skips ExprField's typer-resolved metadata (fieldIndex,
// fieldRef, annotation, atField, derefFlags, fieldFlags). A qmacro pattern
// has these at their default values; a post-inference source has them
// resolved (fieldIndex set to the real struct/tuple slot, derefFlags/r2v
// flipped, etc.). Without the skip, every qmatch against a typed source
// would fail with field_mismatch even when the user-visible expression
// (`recv.name`) is identical.
//
// This is the production scenario inside macro_function bodies that walk
// post-Mode-2-expanded ASTs (the dasSQLITE _sql machinery is the canonical
// caller). These tests pin the behavior so a future qm_skip_field change
// doesn't reintroduce the regression.
// ============================================================================

[test]
def test_fieldindex_set_does_not_break_match(t : T?) {
    // Source has a non-default fieldIndex (post-typer state). Pattern has
    // fieldIndex=-1 (qmacro default). Without the skip, this would fail.
    ast_gc_guard() {
        var expr = qmacro(_._0)
        if (expr is ExprField) {
            var ef = expr as ExprField
            ef.fieldIndex = 0
        }
        let r = qmatch(expr, _._0)
        t |> success(r.matched, "_._0 should match _._0 with fieldIndex=0; matched={r.matched} error={r.error}")
    }
}

[test]
def test_capture_f_with_fieldindex_set(t : T?) {
    // $f capture against a typed source (fieldIndex resolved).
    ast_gc_guard() {
        var expr = qmacro(_._0)
        if (expr is ExprField) {
            var ef = expr as ExprField
            ef.fieldIndex = 0
        }
        var fname : string
        let r = qmatch(expr, _.$f(fname))
        t |> success(r.matched, "_.$f(fname) should match _._0 with fieldIndex=0")
        t |> equal(fname, "_0", "should capture _0 from typed source")
    }
}

[test]
def test_fieldindex_high_value(t : T?) {
    // Higher fieldIndex (e.g. 5th tuple slot) — same skip path.
    ast_gc_guard() {
        var expr = qmacro(_.Salary)
        if (expr is ExprField) {
            var ef = expr as ExprField
            ef.fieldIndex = 5
        }
        var fname : string
        let r = qmatch(expr, _.$f(fname))
        t |> success(r.matched, "_.$f(fname) should match _.Salary with fieldIndex=5")
        t |> equal(fname, "Salary", "should capture struct field name")
    }
}

[test]
def test_chained_fields_typed(t : T?) {
    // Two-level field access — both ExprFields have resolved fieldIndex.
    // Mimics `_._0._1` (multi-key group projection).
    ast_gc_guard() {
        var expr = qmacro(_._0._1)
        if (expr is ExprField) {
            var outer = expr as ExprField
            outer.fieldIndex = 1
            if (outer.value is ExprField) {
                var inner = outer.value as ExprField
                inner.fieldIndex = 0
            }
        }
        let r = qmatch(expr, _._0._1)
        t |> success(r.matched, "_._0._1 should match through both typed levels; matched={r.matched} error={r.error}")
    }
}

[test]
def test_chained_capture_outer(t : T?) {
    // Pattern captures the outer field, inner fixed.
    ast_gc_guard() {
        var expr = qmacro(_._0._2)
        if (expr is ExprField) {
            var outer = expr as ExprField
            outer.fieldIndex = 2
            if (outer.value is ExprField) {
                var inner = outer.value as ExprField
                inner.fieldIndex = 0
            }
        }
        var fname : string
        let r = qmatch(expr, _._0.$f(fname))
        t |> success(r.matched, "_._0.$f(fname) should match _._0._2; matched={r.matched} error={r.error}")
        t |> equal(fname, "_2")
    }
}

// ── Receiver-discrimination contract ───────────────────────────────────────
//
// `$i(name).$f(field)` over-matches: `$i` accepts any expression and pulls
// a name via `qm_extract_name` (which returns the field name for an
// ExprField receiver, the call name for an ExprCall, etc). For dasSQLITE
// shape checks like "<arg>.<field>" we need to reject nested chains
// (`a.b.c`, `f().X`) because their receiver isn't a bare lambda parameter.
//
// The idiom is `$e(recv).$f(field)` + an explicit `recv is ExprVar` check:
// `$e` captures the underlying expression (auto-peeling ExprRef2Value),
// the caller pins the type. These tests document the contract so a future
// matcher rewrite that loses the discrimination property fails loudly.

[test]
def test_e_recv_dot_f_captures_var_receiver(t : T?) {
    // Single-level access — receiver is a bare ExprVar, captured via $e.
    ast_gc_guard() {
        var expr = qmacro(a.foo)
        var receiver : ExpressionPtr
        var fname : string
        let r = qmatch(expr, $e(receiver).$f(fname))
        t |> success(r.matched, "$e(recv).$f(name) should match a.foo")
        t |> equal(fname, "foo", "captured field name")
        t |> success(receiver != null && receiver is ExprVar, "receiver should be ExprVar")
        if (receiver is ExprVar) {
            t |> equal(string((receiver as ExprVar).name), "a", "receiver var name")
        }
    }
}

[test]
def test_e_recv_dot_f_rejects_nested_receiver(t : T?) {
    // Two-level access — receiver of the outer ExprField is itself an
    // ExprField (a.b), NOT an ExprVar. The qmatch DOES match (it's still
    // a field access at top), but the captured receiver is the nested
    // chain, so an `is ExprVar` gate at the call site correctly rejects.
    ast_gc_guard() {
        var expr = qmacro(a.b.c)
        var receiver : ExpressionPtr
        var fname : string
        let r = qmatch(expr, $e(receiver).$f(fname))
        t |> success(r.matched, "outer ExprField still matches")
        t |> equal(fname, "c", "outer field name = c")
        t |> success(receiver != null && !(receiver is ExprVar),
                     "receiver of nested chain MUST NOT pass an `is ExprVar` gate")
        t |> success(receiver is ExprField, "captured receiver is ExprField (a.b)")
    }
}

[test]
def test_e_recv_dot_f_rejects_call_receiver(t : T?) {
    // Receiver is a function call (`compute().X`) — also not a bare
    // ExprVar; the discrimination contract rejects this shape too.
    ast_gc_guard() {
        var expr = qmacro(compute().X)
        var receiver : ExpressionPtr
        var fname : string
        let r = qmatch(expr, $e(receiver).$f(fname))
        t |> success(r.matched, "outer ExprField still matches")
        t |> equal(fname, "X", "outer field name")
        t |> success(receiver != null && !(receiver is ExprVar),
                     "ExprCall receiver MUST NOT pass an `is ExprVar` gate")
    }
}
