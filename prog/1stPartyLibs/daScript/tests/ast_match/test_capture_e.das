options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

[test]
def test_e_op2_children(t : T?) {
    var inscope expr <- qmacro(a + b)
    var inscope left : ExpressionPtr
    var inscope right : ExpressionPtr
    let r = qmatch(expr, $e(left) + $e(right))
    t |> success(r.matched, "should capture left and right")
    if (r.matched) {
        t |> equal(string(get_ptr(left).__rtti), "ExprVar")
        t |> equal(string(get_ptr(right).__rtti), "ExprVar")
    }
}

[test]
def test_e_whole_expression(t : T?) {
    var inscope expr <- qmacro(a + b)
    var inscope captured : ExpressionPtr
    let r = qmatch(expr, $e(captured))
    t |> success(r.matched, "$e on any expression should match")
    if (r.matched) {
        t |> equal(string(get_ptr(captured).__rtti), "ExprOp2")
    }
}

[test]
def test_e_rtti_mismatch(t : T?) {
    var inscope expr <- qmacro(42)
    var inscope captured : ExpressionPtr
    let r = qmatch(expr, $e(captured) + $e(captured))
    t |> success(!r.matched, "int should not match op2 with $e")
    t |> equal(int(r.error), int(QMatchError.rtti_mismatch))
}

[test]
def test_e_clone_independence(t : T?) {
    // Captured clone should be independent from original
    var inscope expr <- qmacro(42)
    var inscope captured : ExpressionPtr
    let r = qmatch(expr, $e(captured))
    t |> success(r.matched, "capture should succeed")
    if (r.matched) {
        // Both should be ExprConstInt with value 42
        t |> equal(string(get_ptr(captured).__rtti), "ExprConstInt")
        // Pointers should differ (it's a clone)
        t |> success(get_ptr(captured) != get_ptr(expr), "clone should be different pointer")
    }
}

[test]
def test_e_mixed_with_fixed(t : T?) {
    // Pattern: $e(x) + 5  — capture left, fixed right
    var inscope expr <- qmacro(a + 5)
    var inscope left : ExpressionPtr
    let r = qmatch(expr, $e(left) + 5)
    t |> success(r.matched, "should match $e + fixed")
    if (r.matched) {
        t |> equal(string(get_ptr(left).__rtti), "ExprVar")
    }
}

[test]
def test_e_in_block(t : T?) {
    // $e capture inside qmatch_block
    var inscope expr <- qmacro($ { return a + b; })
    var inscope captured : ExpressionPtr
    let r = qmatch_block(expr) $ {
        return $e(captured)
    }
    t |> success(r.matched, "should capture return value in block")
    if (r.matched) {
        t |> equal(string(get_ptr(captured).__rtti), "ExprOp2")
    }
}
