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
    ast_gc_guard() {
        var expr = qmacro(a + b)
        var left : ExpressionPtr
        var right : ExpressionPtr
        let r = qmatch(expr, $e(left) + $e(right))
        t |> success(r.matched, "should capture left and right")
        if (r.matched) {
            t |> equal(string(left.__rtti), "ExprVar")
            t |> equal(string(right.__rtti), "ExprVar")
        }
    }
}

[test]
def test_e_whole_expression(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(a + b)
        var captured : ExpressionPtr
        let r = qmatch(expr, $e(captured))
        t |> success(r.matched, "$e on any expression should match")
        if (r.matched) {
            t |> equal(string(captured.__rtti), "ExprOp2")
        }
    }
}

[test]
def test_e_rtti_mismatch(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(42)
        var captured : ExpressionPtr
        let r = qmatch(expr, $e(captured) + $e(captured))
        t |> success(!r.matched, "int should not match op2 with $e")
        t |> equal(int(r.error), int(QMatchError.rtti_mismatch))
    }
}

[test]
def test_e_clone_independence(t : T?) {
    ast_gc_guard() {
        // Captured clone should be independent from original
        var expr = qmacro(42)
        var captured : ExpressionPtr
        let r = qmatch(expr, $e(captured))
        t |> success(r.matched, "capture should succeed")
        if (r.matched) {
            // Both should be ExprConstInt with value 42
            t |> equal(string(captured.__rtti), "ExprConstInt")
            // Pointers should differ (it's a clone)
            t |> success(captured != expr, "clone should be different pointer")
        }
    }
}

[test]
def test_e_mixed_with_fixed(t : T?) {
    ast_gc_guard() {
        // Pattern: $e(x) + 5  — capture left, fixed right
        var expr = qmacro(a + 5)
        var left : ExpressionPtr
        let r = qmatch(expr, $e(left) + 5)
        t |> success(r.matched, "should match $e + fixed")
        if (r.matched) {
            t |> equal(string(left.__rtti), "ExprVar")
        }
    }
}

[test]
def test_e_in_block(t : T?) {
    ast_gc_guard() {
        // $e capture inside qmatch_block
        var expr = qmacro($ { return a + b; })
        var captured : ExpressionPtr
        let r = qmatch_block(expr) $ {
            return $e(captured)
        }
        t |> success(r.matched, "should capture return value in block")
        if (r.matched) {
            t |> equal(string(captured.__rtti), "ExprOp2")
        }
    }
}
