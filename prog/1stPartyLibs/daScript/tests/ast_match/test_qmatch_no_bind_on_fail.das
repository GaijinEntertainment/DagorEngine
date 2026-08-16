options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// The qmatch contract: capture variables are written ONLY when the whole match
// succeeds. A failing match leaves every input untouched, no matter where in the
// generated guard ladder the failure lands. Each test below fails AFTER the
// capture site would have run, per capture kind ($e $v $i $t $c $f $b $a).

// ── $e — expression capture ─────────────────────────────────────────────

[test]
def test_e_untouched_on_root_op_fail(t : T?) {
    ast_gc_guard() {
        // both children captured before the op guard compares "*" vs "+"
        var expr = qmacro(x * y)
        var a, b : ExpressionPtr
        let r = qmatch(expr, $e(a) + $e(b))
        t |> success(!r.matched, "op mismatch must fail")
        t |> success(a == null, "$e(a) untouched on failed match")
        t |> success(b == null, "$e(b) untouched on failed match")
    }
}

[test]
def test_e_untouched_on_nested_fail(t : T?) {
    ast_gc_guard() {
        // right subtree captures b,c then fails its own op guard; a never reached
        var expr = qmacro(x * (y - z))
        var a, b, c : ExpressionPtr
        let r = qmatch(expr, $e(a) * ($e(b) + $e(c)))
        t |> success(!r.matched, "nested op mismatch must fail")
        t |> success(a == null, "$e(a) untouched")
        t |> success(b == null, "$e(b) untouched")
        t |> success(c == null, "$e(c) untouched")
    }
}

[test]
def test_e_ladder_keeps_previous_bindings(t : T?) {
    ast_gc_guard() {
        // the real-world hazard: a failed second qmatch must not clobber
        // bindings a previous successful match established
        var good = qmacro(x + y)
        var bad = qmacro(p * q)
        var a, b : ExpressionPtr
        let r1 = qmatch(good, $e(a) + $e(b))
        t |> success(r1.matched, "first match succeeds")
        let a0 = a
        let b0 = b
        let r2 = qmatch(bad, $e(a) + $e(b))
        t |> success(!r2.matched, "second match fails")
        t |> success(a == a0, "a still holds the first match's binding")
        t |> success(b == b0, "b still holds the first match's binding")
    }
}

// ── $v — constant value capture ─────────────────────────────────────────

[test]
def test_v_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        // both constants extract fine; the op guard then fails
        var expr = qmacro(5 * 6)
        var k = 999
        var m = 888
        let r = qmatch(expr, $v(k) + $v(m))
        t |> success(!r.matched, "op mismatch must fail")
        t |> equal(999, k, "$v(k) untouched on failed match")
        t |> equal(888, m, "$v(m) untouched on failed match")
    }
}

// ── $i — name capture ───────────────────────────────────────────────────

[test]
def test_i_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(x * y)
        var nm = "keep1"
        var nm2 = "keep2"
        let r = qmatch(expr, $i(nm) + $i(nm2))
        t |> success(!r.matched, "op mismatch must fail")
        t |> equal("keep1", nm, "$i(nm) untouched on failed match")
        t |> equal("keep2", nm2, "$i(nm2) untouched on failed match")
    }
}

// ── $t — type capture ───────────────────────────────────────────────────

[test]
def test_t_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(foo(type<int>) - x)
        var td : TypeDeclPtr
        var z : ExpressionPtr
        let r = qmatch(expr, foo(type<$t(td)>) + $e(z))
        t |> success(!r.matched, "op mismatch must fail")
        t |> success(td == null, "$t(td) untouched on failed match")
        t |> success(z == null, "$e(z) untouched on failed match")
    }
}

// ── $c — call name capture ──────────────────────────────────────────────

[test]
def test_c_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(foo(1) - x)
        var cn = "keep"
        var z : ExpressionPtr
        let r = qmatch(expr, $c(cn)(1) + $e(z))
        t |> success(!r.matched, "op mismatch must fail")
        t |> equal("keep", cn, "$c(cn) untouched on failed match")
        t |> success(z == null, "$e(z) untouched on failed match")
    }
}

// ── $f — field name capture ─────────────────────────────────────────────

[test]
def test_f_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(a.foo - x)
        var fn = "keep"
        var z : ExpressionPtr
        let r = qmatch(expr, a.$f(fn) + $e(z))
        t |> success(!r.matched, "op mismatch must fail")
        t |> equal("keep", fn, "$f(fn) untouched on failed match")
        t |> success(z == null, "$e(z) untouched on failed match")
    }
}

// ── $b — statement range capture ────────────────────────────────────────

[test]
def test_b_untouched_on_late_fail(t : T?) {
    ast_gc_guard() {
        // _any($b) extracts its statement, then the fixed baz() fails on bar()
        var expr = qmacro($ { foo(); bar(); })
        var stmts : array<ExpressionPtr>
        let r = qmatch_block(expr) $ {
            _any($b(stmts))
            baz()
        }
        t |> success(!r.matched, "fixed statement mismatch must fail")
        t |> equal(0, length(stmts), "$b(stmts) untouched on failed match")
    }
}

// ── $a — remaining-args capture ─────────────────────────────────────────

[export, marker(no_coverage)]
def nbof_target(a, b, c : int) : int {
    return a + b + c
}

[test]
def test_a_untouched_on_body_fail(t : T?) {
    ast_gc_guard() {
        // args phase matches and captures rest; the body pattern then fails
        var func = find_module_function_via_rtti(compiling_module(), @@nbof_target)
        if (func == null) {
            return
        }
        var rest : array<VariablePtr>
        let r = qmatch_function(func) $(a : int; $a(rest)) {
            return a * 111
        }
        t |> success(!r.matched, "body mismatch must fail")
        t |> equal(0, length(rest), "$a(rest) untouched on failed match")
    }
}

// ── success still binds, including through the scan and direct paths ────

[test]
def test_scan_path_still_binds_on_success(t : T?) {
    ast_gc_guard() {
        // the wildcard scan runs the element matcher in an inner closure -
        // captures must still commit when the overall match succeeds
        var expr = qmacro($ { foo(); q + 1; })
        var cap : ExpressionPtr
        let r = qmatch_block(expr) $ {
            _wildcard()
            $e(cap) + 1
        }
        t |> success(r.matched, "scan should find q + 1")
        t |> success(cap != null, "$e(cap) bound through the scan path")
        t |> success(cap != null && cap is ExprVar, "cap is the scanned q")
    }
}

[test]
def test_direct_path_still_binds_on_success(t : T?) {
    ast_gc_guard() {
        // single-statement block pattern against a non-block source takes the
        // direct-match early-success path - commits must ride it too
        var expr = qmacro(x + 1)
        var cap : ExpressionPtr
        let r = qmatch_block(expr) $ {
            $e(cap) + 1
        }
        t |> success(r.matched, "direct path should match")
        t |> success(cap != null, "$e(cap) bound through the direct path")
    }
}
