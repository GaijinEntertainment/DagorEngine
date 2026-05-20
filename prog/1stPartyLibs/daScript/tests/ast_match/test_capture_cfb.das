options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── $c(var) — capture call name ─────────────────────────────────────────

[test]
def test_c_call_name(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(foo(1, 2))
        var cname = ""
        let r = qmatch(expr, $c(cname)(1, 2))
        t |> success(r.matched, "should match call with any name")
        t |> equal(cname, "foo", "should capture call name")
    }
}

[test]
def test_c_call_name_different(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(bar(a, b))
        var cname = ""
        let r = qmatch(expr, $c(cname)(a, b))
        t |> success(r.matched, "should match bar call")
        t |> equal(cname, "bar", "should capture bar")
    }
}

[test]
def test_c_args_mismatch(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(foo(1, 2))
        var cname = ""
        let r = qmatch(expr, $c(cname)(1, 3))
        t |> success(!r.matched, "wrong args should fail even with $c")
    }
}

[test]
def test_c_in_block(t : T?) {
    ast_gc_guard() {
        var expr = qmacro($ { foo(a); bar(b); })
        var cname = ""
        let r = qmatch_block(expr) $ {
            _wildcard()
            $c(cname)(b)
        }
        t |> success(r.matched, "should match bar in block")
        t |> equal(cname, "bar", "should capture bar from block")
    }
}

// ── $f(var) — capture field name ────────────────────────────────────────

[test]
def test_f_field_name(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(a.foo)
        var fname = ""
        let r = qmatch(expr, a.$f(fname))
        t |> success(r.matched, "should match field access with any name")
        t |> equal(fname, "foo", "should capture field name")
    }
}

[test]
def test_f_field_different(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(obj.bar)
        var fname = ""
        let r = qmatch(expr, obj.$f(fname))
        t |> success(r.matched, "should match obj.bar")
        t |> equal(fname, "bar", "should capture bar field name")
    }
}

[test]
def test_f_field_value_mismatch(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(a.foo)
        var fname = ""
        let r = qmatch(expr, b.$f(fname))
        t |> success(!r.matched, "wrong value should fail even with $f")
    }
}

[test]
def test_f_in_block(t : T?) {
    ast_gc_guard() {
        var expr = qmacro($ { a.x; b.y; })
        var fname = ""
        let r = qmatch_block(expr) $ {
            _wildcard()
            b.$f(fname)
        }
        t |> success(r.matched, "should match b.y in block")
        t |> equal(fname, "y", "should capture y from block")
    }
}

// ── $b(var) — capture statement range ───────────────────────────────────

[test]
def test_b_wildcard_trailing(t : T?) {
    ast_gc_guard() {
        // Capture all trailing statements
        var expr = qmacro($ { foo(); bar(); baz(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            foo()
            _wildcard($b(stmts))
        }
        t |> success(r.matched, "should match with trailing $b capture")
        t |> equal(length(stmts), 2, "should capture 2 trailing stmts")
    }
}

[test]
def test_b_wildcard_leading(t : T?) {
    ast_gc_guard() {
        // Capture all leading statements before a fixed match
        var expr = qmacro($ { foo(); bar(); baz(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            _wildcard($b(stmts))
            baz()
        }
        t |> success(r.matched, "should match with leading $b capture")
        t |> equal(length(stmts), 2, "should capture 2 leading stmts")
    }
}

[test]
def test_b_wildcard_empty(t : T?) {
    ast_gc_guard() {
        // Wildcard captures zero statements
        var expr = qmacro($ { foo(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            _wildcard($b(stmts))
            foo()
        }
        t |> success(r.matched, "should match with empty $b")
        t |> equal(length(stmts), 0, "should capture 0 stmts")
    }
}

[test]
def test_b_wildcard_sandwich(t : T?) {
    ast_gc_guard() {
        // Capture middle statements between two fixed
        var expr = qmacro($ { foo(); a(); b(); bar(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            foo()
            _wildcard($b(stmts))
            bar()
        }
        t |> success(r.matched, "should match sandwich")
        t |> equal(length(stmts), 2, "should capture 2 middle stmts")
    }
}

[test]
def test_b_wildcard1_trailing(t : T?) {
    ast_gc_guard() {
        // _wildcard1 with $b — must have at least 1
        var expr = qmacro($ { foo(); bar(); baz(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            foo()
            _wildcard1($b(stmts))
        }
        t |> success(r.matched, "should match with _wildcard1 trailing $b")
        t |> equal(length(stmts), 2, "should capture 2 trailing stmts")
    }
}

[test]
def test_b_any_capture(t : T?) {
    ast_gc_guard() {
        // _any with $b — captures exactly one statement
        var expr = qmacro($ { foo(); bar(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            _any($b(stmts))
            bar()
        }
        t |> success(r.matched, "should match with _any $b")
        t |> equal(length(stmts), 1, "should capture exactly 1 stmt")
    }
}

[test]
def test_b_no_tag_still_works(t : T?) {
    ast_gc_guard() {
        // Wildcards without $b still work as before
        var expr = qmacro($ { foo(); bar(); baz(); })
        let r = qmatch_block(expr) $ {
            _wildcard()
            baz()
        }
        t |> success(r.matched, "wildcard without $b should still work")
    }
}
