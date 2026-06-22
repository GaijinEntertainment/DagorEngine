options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``peel_tuple_field_read`` checks for the shape ``<bindName>._<fieldIndex>``
// (a tuple-slot read on a named bind). Single-level ``ExprRef2Value`` peel
// on each side — the typer never nests these wrappers.

// ── Builders ────────────────────────────────────────────────────────────

def private mk_var(name : string) : Expression? {
    var v = new ExprVar(at = LineInfo())
    v.name := name
    return v
}

def private mk_field(var value : Expression?; fieldName : string) : Expression? {
    var f = new ExprField(at = LineInfo(), value = value)
    f.name := fieldName
    return f
}

def private mk_r2v(var inner : Expression?) : Expression? {
    return new ExprRef2Value(at = LineInfo(), subexpr = inner)
}

// ── Direct matches ──────────────────────────────────────────────────────

[test]
def test_match_bind_field0(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_var("g"), "_0")
        t |> success(peel_tuple_field_read(e, "g", 0), "g._0 should match (g, 0)")
    }
}

[test]
def test_match_bind_field5(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_var("bind"), "_5")
        t |> success(peel_tuple_field_read(e, "bind", 5), "bind._5 should match (bind, 5)")
    }
}

// ── Mismatches ──────────────────────────────────────────────────────────

[test]
def test_wrong_bind_name(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_var("g"), "_0")
        t |> success(!peel_tuple_field_read(e, "other", 0), "wrong bindName must not match")
    }
}

[test]
def test_wrong_field_index(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_var("g"), "_0")
        t |> success(!peel_tuple_field_read(e, "g", 1), "wrong fieldIndex must not match")
    }
}

[test]
def test_field_not_tuple_slot(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_var("g"), "name")
        t |> success(!peel_tuple_field_read(e, "g", 0), "non-_N field must not match")
    }
}

[test]
def test_non_field_does_not_match(t : T?) {
    ast_gc_guard() {
        let e = mk_var("g")
        t |> success(!peel_tuple_field_read(e, "g", 0), "ExprVar alone must not match")
    }
}

[test]
def test_value_not_var(t : T?) {
    ast_gc_guard() {
        // field over a literal — value is not ExprVar.
        var lit = qmacro(42)
        let e = mk_field(lit, "_0")
        t |> success(!peel_tuple_field_read(e, "g", 0), "ExprField over non-ExprVar must not match")
    }
}

// ── ExprRef2Value peeling (single level on each side) ──────────────────

[test]
def test_r2v_wrapped_outer(t : T?) {
    ast_gc_guard() {
        var inner = mk_field(mk_var("g"), "_0")
        let wrapped = mk_r2v(inner)
        t |> success(peel_tuple_field_read(wrapped, "g", 0), "outer R2V wrapper must peel")
    }
}

[test]
def test_r2v_wrapped_inner_value(t : T?) {
    ast_gc_guard() {
        let e = mk_field(mk_r2v(mk_var("g")), "_0")
        t |> success(peel_tuple_field_read(e, "g", 0), "R2V wrapping inner ExprVar must peel")
    }
}

[test]
def test_r2v_wrapped_both(t : T?) {
    ast_gc_guard() {
        var inner = mk_field(mk_r2v(mk_var("g")), "_0")
        let wrapped = mk_r2v(inner)
        t |> success(peel_tuple_field_read(wrapped, "g", 0), "R2V on both sides must peel")
    }
}

// ── Null guards ────────────────────────────────────────────────────────

[test]
def test_null_does_not_match(t : T?) {
    var e : Expression?
    t |> success(!peel_tuple_field_read(e, "g", 0), "null input must not match")
}
