options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Helpers to build test expressions ───────────────────────────────────

def make_int(val : int) : ExpressionPtr {
    return <- qmacro($v(val))
}

def make_bool(val : bool) : ExpressionPtr {
    return <- qmacro($v(val))
}

def make_add_ab() : ExpressionPtr {
    return <- qmacro(a + b)
}

def make_add_xb() : ExpressionPtr {
    return <- qmacro(x + b)
}

// ── Basic matching ──────────────────────────────────────────────────────

[test]
def test_result_types(t : T?) {
    var r = qm_ok()
    t |> equal(r.matched, true)
    var r2 = qm_fail_null()
    t |> equal(r2.matched, false)
    t |> equal(int(r2.error), int(QMatchError.null_expression))
}

[test]
def test_const_int_match(t : T?) {
    var inscope expr <- make_int(42)
    t |> success(qmatch(expr, 42).matched, "42 should match 42")
}

[test]
def test_const_int_mismatch(t : T?) {
    var inscope expr <- make_int(42)
    let r = qmatch(expr, 99)
    t |> success(!r.matched, "42 should not match 99")
    t |> equal(int(r.error), int(QMatchError.field_mismatch))
}

[test]
def test_rtti_mismatch(t : T?) {
    var inscope expr <- make_int(42)
    let r = qmatch(expr, true)
    t |> success(!r.matched, "int should not match bool pattern")
    t |> equal(int(r.error), int(QMatchError.rtti_mismatch))
}

[test]
def test_const_bool_match(t : T?) {
    var inscope expr <- make_bool(true)
    t |> success(qmatch(expr, true).matched, "true == true")
}

[test]
def test_const_bool_mismatch(t : T?) {
    var inscope expr <- make_bool(false)
    t |> success(!qmatch(expr, true).matched, "false != true")
}

[test]
def test_op2_match(t : T?) {
    var inscope expr <- make_add_ab()
    t |> success(qmatch(expr, a + b).matched, "a+b matches a+b")
}

[test]
def test_op2_wrong_op(t : T?) {
    var inscope expr <- make_add_ab()
    t |> success(!qmatch(expr, a - b).matched, "a+b does not match a-b")
}

[test]
def test_op2_wrong_var(t : T?) {
    var inscope expr <- make_add_xb()
    t |> success(!qmatch(expr, a + b).matched, "x+b does not match a+b")
}

// ── $e capture ──────────────────────────────────────────────────────────

[test]
def test_capture_e_op2(t : T?) {
    // Match a + b, capture left and right as ExpressionPtr
    var inscope expr <- make_add_ab()
    var inscope left : ExpressionPtr
    var inscope right : ExpressionPtr
    let r = qmatch(expr, $e(left) + $e(right))
    t |> success(r.matched, "should match with $e captures")
    if (r.matched) {
        t |> equal(string(get_ptr(left).__rtti), "ExprVar")
        t |> equal(string(get_ptr(right).__rtti), "ExprVar")
    }
}

[test]
def test_capture_e_rtti_mismatch(t : T?) {
    // $e captures should still fail on RTTI mismatch at the outer level
    var inscope expr <- make_int(42)
    var inscope captured : ExpressionPtr
    let r = qmatch(expr, $e(captured) + $e(captured))
    t |> success(!r.matched, "int should not match op2 pattern even with $e")
    t |> equal(int(r.error), int(QMatchError.rtti_mismatch))
}

// ── $v capture ──────────────────────────────────────────────────────────

[test]
def test_capture_v_int(t : T?) {
    var inscope expr <- make_int(42)
    var val : int
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should match ExprConstInt with $v")
    if (r.matched) {
        t |> equal(val, 42)
    }
}

// TODO: $v with wrong type (float var on int expr) — currently only extracts int

// ── $i capture ──────────────────────────────────────────────────────────

// ── $i capture ──────────────────────────────────────────────────────────

[test]
def test_capture_i_var(t : T?) {
    // Extract variable name from ExprVar
    var inscope expr <- qmacro(my_variable)
    var name : string
    let r = qmatch(expr, $i(name))
    t |> success(r.matched, "should match ExprVar with $i")
    if (r.matched) {
        t |> equal(name, "my_variable")
    }
}

[test]
def test_capture_i_from_op2(t : T?) {
    // Extract variable names from a + b
    var inscope expr <- make_add_ab()
    var left_name : string
    var right_name : string
    let r = qmatch(expr, $i(left_name) + $i(right_name))
    t |> success(r.matched, "should match a + b with $i captures")
    if (r.matched) {
        t |> equal(left_name, "a")
        t |> equal(right_name, "b")
    }
}

// TODO: $c(var) for call name extraction (Phase 2)

// ── Null expression ─────────────────────────────────────────────────────

[test]
def test_null_expr_fails(t : T?) {
    var inscope expr : ExpressionPtr
    let r = qmatch(expr, 42)
    t |> success(!r.matched, "null expr should fail")
    t |> equal(int(r.error), int(QMatchError.null_expression))
}

// ── String constant ─────────────────────────────────────────────────────

[test]
def test_const_string_match(t : T?) {
    var inscope expr <- qmacro("hello")
    t |> success(qmatch(expr, "hello").matched, "hello == hello")
}

[test]
def test_const_string_mismatch(t : T?) {
    var inscope expr <- qmacro("hello")
    t |> success(!qmatch(expr, "world").matched, "hello != world")
}

// ── Float constant ──────────────────────────────────────────────────────

[test]
def test_const_float_match(t : T?) {
    var inscope expr <- qmacro(3.14)
    t |> success(qmatch(expr, 3.14).matched, "3.14 == 3.14")
}

[test]
def test_const_float_mismatch(t : T?) {
    var inscope expr <- qmacro(3.14)
    t |> success(!qmatch(expr, 2.71).matched, "3.14 != 2.71")
}

// ── Cross-type mismatch ─────────────────────────────────────────────────

[test]
def test_int_vs_string(t : T?) {
    var inscope expr <- make_int(42)
    t |> success(!qmatch(expr, "hello").matched, "int vs string")
}

[test]
def test_bool_vs_int(t : T?) {
    var inscope expr <- make_bool(true)
    t |> success(!qmatch(expr, 42).matched, "bool vs int")
}

[test]
def test_op2_vs_const(t : T?) {
    var inscope expr <- make_add_ab()
    t |> success(!qmatch(expr, 42).matched, "op2 vs const")
}

[test]
def test_const_vs_op2(t : T?) {
    var inscope expr <- make_int(42)
    t |> success(!qmatch(expr, a + b).matched, "const vs op2")
}

// ── qmatch_block ────────────────────────────────────────────────────────

def make_block_ab() : ExpressionPtr {
    // Block: { a + b; return c; }
    return <- qmacro($ { a + b; return c; })
}

[test]
def test_block_exact_match(t : T?) {
    var inscope expr <- make_block_ab()
    let r = qmatch_block(expr) $ {
        a + b
        return c
    }
    t |> success(r.matched, "exact block match should succeed")
}

[test]
def test_block_wrong_stmt(t : T?) {
    var inscope expr <- make_block_ab()
    let r = qmatch_block(expr) $ {
        a - b
        return c
    }
    t |> success(!r.matched, "wrong statement should fail")
}

[test]
def test_block_extra_stmt(t : T?) {
    var inscope expr <- make_block_ab()
    let r = qmatch_block(expr) $ {
        a + b
    }
    t |> success(!r.matched, "fewer pattern stmts without wildcard should fail")
}

[test]
def test_block_wildcard_trailing(t : T?) {
    var inscope expr <- make_block_ab()
    let r = qmatch_block(expr) $ {
        a + b
        _wildcard()
    }
    t |> success(r.matched, "wildcard should match remaining stmts")
}

[test]
def test_block_wildcard_leading(t : T?) {
    var inscope expr <- make_block_ab()
    let r = qmatch_block(expr) $ {
        _wildcard()
        return c
    }
    t |> success(r.matched, "leading wildcard + fixed should match")
}

// ── Wildcard edge cases ─────────────────────────────────────────────────

def make_block_1() : ExpressionPtr {
    // { a + b }
    return <- qmacro($ { a + b; })
}

def make_block_2() : ExpressionPtr {
    // { print(); a + b }
    return <- qmacro($ { print(); a + b; })
}

def make_block_3() : ExpressionPtr {
    // { print(); print(); a + b }
    return <- qmacro($ { print(); print(); a + b; })
}

def make_block_4() : ExpressionPtr {
    // { a + b; return c; print() }
    return <- qmacro($ { a + b; return c; print(); })
}

// _wildcard() before a fixed pattern — zero prefix

[test]
def test_wildcard_zero_prefix(t : T?) {
    // Pattern: _wildcard(); a + b  — actual has just a + b (wildcard matches 0)
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        _wildcard()
        a + b
    }
    t |> success(r.matched, "wildcard(0) + fixed should match single stmt")
}

[test]
def test_wildcard_one_prefix(t : T?) {
    // Pattern: _wildcard(); a + b  — actual: print(); a + b
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        _wildcard()
        a + b
    }
    t |> success(r.matched, "wildcard(1) + fixed should match")
}

[test]
def test_wildcard_two_prefix(t : T?) {
    // Pattern: _wildcard(); a + b  — actual: print(); print(); a + b
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        _wildcard()
        a + b
    }
    t |> success(r.matched, "wildcard(2) + fixed should match")
}

// _wildcard() after a fixed pattern — consume trailing

[test]
def test_wildcard_trailing_multi(t : T?) {
    // Pattern: a + b; _wildcard()  — actual: a + b; return c; print()
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        a + b
        _wildcard()
    }
    t |> success(r.matched, "fixed + wildcard should match with trailing stmts")
}

// No wildcard — must match exactly

[test]
def test_no_wildcard_exact(t : T?) {
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        a + b
    }
    t |> success(r.matched, "exact single stmt should match")
}

[test]
def test_no_wildcard_too_few(t : T?) {
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        a + b
    }
    t |> success(!r.matched, "single pattern on 2-stmt block without wildcard should fail")
}

// _wildcard() sandwiching a fixed pattern

[test]
def test_wildcard_sandwich(t : T?) {
    // Pattern: _wildcard(); a + b; _wildcard()  — actual: print(); a + b; return c; print()
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        _wildcard()
        a + b
        _wildcard()
    }
    t |> success(r.matched, "wildcard + fixed + wildcard should match")
}

// _wildcard() with no fixed — matches any block

[test]
def test_wildcard_only(t : T?) {
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        _wildcard()
    }
    t |> success(r.matched, "bare wildcard should match any block")
}

// Empty block

[test]
def test_wildcard_empty_block(t : T?) {
    var inscope expr <- qmacro($ {})
    let r = qmatch_block(expr) $ {
        _wildcard()
    }
    t |> success(r.matched, "wildcard should match empty block")
}

// _wildcard1 — requires at least one

[test]
def test_wildcard1_zero_fails(t : T?) {
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        _wildcard1()
        a + b
    }
    t |> success(!r.matched, "wildcard1 before the only stmt should fail (needs 1+ prefix)")
}

[test]
def test_wildcard1_one_passes(t : T?) {
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        _wildcard1()
        a + b
    }
    t |> success(r.matched, "wildcard1 with 1 prefix should pass")
}

[test]
def test_wildcard1_two_prefix(t : T?) {
    // { print(); print(); a + b } — _wildcard1 should scan forward to find a + b
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        _wildcard1()
        a + b
    }
    t |> success(r.matched, "wildcard1 with 2 prefix should pass (scan forward)")
}

[test]
def test_wildcard1_sandwich(t : T?) {
    // { print(); print(); a + b; return c; print() } — _wildcard1 on both sides
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        _wildcard1()
        return c
        _wildcard1()
    }
    t |> success(r.matched, "wildcard1 sandwich should match")
}

[test]
def test_wildcard1_sandwich_too_few_before(t : T?) {
    // { a + b; return c; print() } — a + b is at pos 0, no room for 1+ before it
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        _wildcard1()
        a + b
        _wildcard()
    }
    t |> success(!r.matched, "wildcard1 needs 1+ before a+b but it is at position 0")
}

// _any — exactly one

[test]
def test_any_exact(t : T?) {
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        _any()
        a + b
    }
    t |> success(r.matched, "any + fixed on 2-stmt block should match")
}

[test]
def test_any_zero_fails(t : T?) {
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        _any()
        a + b
    }
    t |> success(!r.matched, "any before the only stmt should fail (needs exactly 1)")
}

// _wildcard not found — fixed pattern doesn't exist in block

[test]
def test_wildcard_not_found(t : T?) {
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        _wildcard()
        return c
    }
    t |> success(!r.matched, "scanning for nonexistent stmt should fail")
}

// Multiple fixed patterns with wildcards between

[test]
def test_two_fixed_with_wildcards(t : T?) {
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        a + b
        _wildcard()
        print()
    }
    if (!r.matched) {
        if (r.expr != null) {
            t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
        } else {
            t |> success(false, "error={int(r.error)} expr=null")
        }
    } else {
        t |> success(true)
    }
}

// ── _optional — zero or one ──────────────────────────────────────────

[test]
def test_optional_zero(t : T?) {
    // { a + b } — _optional() matches 0, then fixed matches a + b
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        _optional()
        a + b
    }
    t |> success(r.matched, "optional(0) + fixed on 1-stmt block should match")
}

[test]
def test_optional_one(t : T?) {
    // { print(); a + b } — _optional() matches print(), then fixed matches a + b
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        _optional()
        a + b
    }
    t |> success(r.matched, "optional(1) + fixed on 2-stmt block should match")
}

[test]
def test_optional_two_fails(t : T?) {
    // { print(); print(); a + b } — _optional() can't match 2, should fail
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        _optional()
        a + b
    }
    t |> success(!r.matched, "optional + fixed on 3-stmt block should fail (2 before fixed)")
}

[test]
def test_optional_trailing_zero(t : T?) {
    // { a + b } — fixed matches, trailing _optional() matches 0
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        a + b
        _optional()
    }
    t |> success(r.matched, "fixed + trailing optional(0) should match")
}

[test]
def test_optional_trailing_one(t : T?) {
    // { a + b; return c; print() } — fixed matches a+b, trailing _optional() matches 1? No, 2 remain
    // Use { print(); a + b } — fixed matches print(), trailing _optional() matches a+b (1)
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        print()
        _optional()
    }
    t |> success(r.matched, "fixed + trailing optional(1) should match")
}

[test]
def test_optional_trailing_two_fails(t : T?) {
    // { print(); print(); a + b } — fixed matches print(), trailing _optional() has 2 remaining
    var inscope expr <- make_block_3()
    let r = qmatch_block(expr) $ {
        print()
        _optional()
    }
    t |> success(!r.matched, "fixed + trailing optional with 2 remaining should fail")
}

[test]
def test_optional_trailing_only_zero(t : T?) {
    // Empty block — trailing _optional() alone matches 0
    var inscope expr <- qmacro($ { })
    let r = qmatch_block(expr) $ {
        _optional()
    }
    t |> success(r.matched, "trailing optional on empty block should match")
}

[test]
def test_optional_trailing_only_one(t : T?) {
    // { a + b } — _optional() alone matches 1
    var inscope expr <- make_block_1()
    let r = qmatch_block(expr) $ {
        _optional()
    }
    t |> success(r.matched, "trailing optional on 1-stmt block should match")
}

[test]
def test_optional_trailing_only_two_fails(t : T?) {
    // { print(); a + b } — _optional() alone can't match 2
    var inscope expr <- make_block_2()
    let r = qmatch_block(expr) $ {
        _optional()
    }
    t |> success(!r.matched, "trailing optional on 2-stmt block should fail")
}

[test]
def test_optional_sandwich(t : T?) {
    // { a + b; return c; print() } — fixed, _optional(), fixed
    var inscope expr <- make_block_4()
    let r = qmatch_block(expr) $ {
        a + b
        _optional()
        print()
    }
    t |> success(r.matched, "fixed + optional(1) + fixed should match")
}

[test]
def test_optional_sandwich_zero(t : T?) {
    // { a + b; print() } — 2 stmts, fixed + _optional(0) + fixed
    var inscope expr <- qmacro($ { a + b; print(); })
    let r = qmatch_block(expr) $ {
        a + b
        _optional()
        print()
    }
    t |> success(r.matched, "fixed + optional(0) + fixed should match")
}

[test]
def test_optional_sandwich_two_fails(t : T?) {
    // { a + b; foo(); bar(); print() } — 4 stmts, 2 between fixed stmts
    var inscope expr <- qmacro($ { a + b; foo(); bar(); print(); })
    let r = qmatch_block(expr) $ {
        a + b
        _optional()
        print()
    }
    t |> success(!r.matched, "fixed + optional + fixed with 2 between should fail")
}
