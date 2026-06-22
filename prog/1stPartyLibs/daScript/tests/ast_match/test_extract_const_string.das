options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``extract_const_string(e) : tuple<bool; string>`` peels an ``ExprConstString``
// leaf and returns ``(true, value)``; non-literals return ``(false, "")``.
// Used by macro pipelines (sqlite_linq, future planners) that thread compile-time
// string literals through args before splicing them into the emitted AST.

// ── Null / non-literal inputs ────────────────────────────────────────────

[test]
def test_null_does_not_match(t : T?) {
    var expr : Expression?
    let r = extract_const_string(expr)
    t |> success(!r._0, "null input must return false")
    t |> equal(r._1, "")
}

[test]
def test_non_string_const_does_not_match(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(42)
        let r = extract_const_string(expr)
        t |> success(!r._0, "ExprConstInt must not match")
        t |> equal(r._1, "")
    }
}

[test]
def test_var_ref_does_not_match(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(some_var)
        let r = extract_const_string(expr)
        t |> success(!r._0, "ExprVar must not match")
        t |> equal(r._1, "")
    }
}

// ── Positive matches ─────────────────────────────────────────────────────

[test]
def test_const_string_literal(t : T?) {
    ast_gc_guard() {
        var expr = qmacro("hello")
        let r = extract_const_string(expr)
        t |> success(r._0, "ExprConstString must match")
        t |> equal(r._1, "hello")
    }
}

[test]
def test_empty_const_string(t : T?) {
    ast_gc_guard() {
        var expr = qmacro("")
        let r = extract_const_string(expr)
        t |> success(r._0, "empty ExprConstString must still match")
        t |> equal(r._1, "")
    }
}

[test]
def test_const_string_with_special_chars(t : T?) {
    ast_gc_guard() {
        var expr = qmacro("foo'bar\"baz")
        let r = extract_const_string(expr)
        t |> success(r._0, "ExprConstString with quotes must match")
        t |> equal(r._1, "foo'bar\"baz")
    }
}
