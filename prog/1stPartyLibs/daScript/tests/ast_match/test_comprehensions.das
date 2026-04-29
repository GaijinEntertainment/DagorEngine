options gen2
options indenting = 4
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Array comprehension ─────────────────────────────────────────────────

[test]
def test_array_comprehension_match(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, [for (x in range(10)); x])
    t |> success(r.matched, "array comprehension should match")
}

[test]
def test_array_comprehension_wrong_source(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, [for (x in range(5)); x])
    t |> success(!r.matched, "wrong source should fail")
}

[test]
def test_array_comprehension_wrong_subexpr(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, [for (x in range(10)); y])
    t |> success(!r.matched, "wrong subexpr should fail")
}

[test]
def test_array_comprehension_with_where(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x; where x > 5])
    let r = qmatch(expr, [for (x in range(10)); x; where x > 5])
    t |> success(r.matched, "array comprehension with where should match")
}

[test]
def test_array_comprehension_where_mismatch(t : T?) {
    // Pattern has where, actual doesn't
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, [for (x in range(10)); x; where x > 5])
    t |> success(!r.matched, "pattern with where vs actual without should fail")
}

[test]
def test_array_comprehension_no_where_mismatch(t : T?) {
    // Pattern has no where, actual does
    var inscope expr <- qmacro([for (x in range(10)); x; where x > 5])
    let r = qmatch(expr, [for (x in range(10)); x])
    t |> success(!r.matched, "pattern without where vs actual with should fail")
}

[test]
def test_array_comprehension_capture_subexpr(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x * 2])
    var inscope sub : ExpressionPtr
    let r = qmatch(expr, [for (x in range(10)); $e(sub)])
    t |> success(r.matched, "should match with $e capture on subexpr")
    if (r.matched) {
        t |> equal(string(get_ptr(sub).__rtti), "ExprOp2")
    }
}

[test]
def test_array_comprehension_capture_iterator(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    var name : string
    let r = qmatch(expr, [for ($i(name) in range(10)); x])
    t |> success(r.matched, "should match with $i on iterator")
    if (r.matched) {
        t |> equal(name, "x")
    }
}

[test]
def test_array_comprehension_wildcard_subexpr(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x * x + 1])
    let r = qmatch(expr, [for (x in range(10)); _any()])
    t |> success(r.matched, "wildcard subexpr should match anything")
}

// ── Array vs table mismatch ─────────────────────────────────────────────

[test]
def test_array_vs_table_mismatch(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, {for (x in range(10)); x})
    t |> success(!r.matched, "array pattern vs table actual should fail")
}

[test]
def test_table_vs_array_mismatch(t : T?) {
    var inscope expr <- qmacro({for (x in range(10)); x})
    let r = qmatch(expr, [for (x in range(10)); x])
    t |> success(!r.matched, "table pattern vs array actual should fail")
}

// ── Table comprehension ─────────────────────────────────────────────────

[test]
def test_table_comprehension_match(t : T?) {
    var inscope expr <- qmacro({for (x in range(10)); x => x * x})
    let r = qmatch(expr, {for (x in range(10)); x => x * x})
    t |> success(r.matched, "table comprehension should match")
}

[test]
def test_table_comprehension_key_only(t : T?) {
    var inscope expr <- qmacro({for (x in range(10)); x})
    let r = qmatch(expr, {for (x in range(10)); x})
    t |> success(r.matched, "table comprehension key-only should match")
}

[test]
def test_table_comprehension_capture_subexpr(t : T?) {
    var inscope expr <- qmacro({for (x in range(10)); x => x * x})
    var inscope sub : ExpressionPtr
    let r = qmatch(expr, {for (x in range(10)); $e(sub)})
    t |> success(r.matched, "should match with $e capture on table subexpr")
    if (r.matched) {
        // subexpr is x => x * x, which is ExprMakeTuple
        t |> success(get_ptr(sub) != null, "captured subexpr should not be null")
    }
}

[test]
def test_table_comprehension_wrong_subexpr(t : T?) {
    var inscope expr <- qmacro({for (x in range(10)); x => x * x})
    let r = qmatch(expr, {for (x in range(10)); x => x + x})
    t |> success(!r.matched, "wrong table subexpr should fail")
}

// ── Generator comprehension ─────────────────────────────────────────────

[test]
def test_generator_comprehension_match(t : T?) {
    var inscope expr <- qmacro([iterator for(x in range(10)); x])
    let r = qmatch(expr, [iterator for(x in range(10)); x])
    t |> success(r.matched, "generator comprehension should match")
}

[test]
def test_generator_vs_array_mismatch(t : T?) {
    var inscope expr <- qmacro([iterator for(x in range(10)); x])
    let r = qmatch(expr, [for (x in range(10)); x])
    t |> success(!r.matched, "generator pattern vs array should fail")
}

[test]
def test_array_vs_generator_mismatch(t : T?) {
    var inscope expr <- qmacro([for (x in range(10)); x])
    let r = qmatch(expr, [iterator for(x in range(10)); x])
    t |> success(!r.matched, "array pattern vs generator should fail")
}

// ── ExprMakeBlock mmFlags ───────────────────────────────────────────────

[test]
def test_block_matches_block(t : T?) {
    var inscope expr <- qmacro($ { return 1; })
    let r = qmatch(expr, $ { return 1; })
    t |> success(r.matched, "plain block should match plain block")
}

[test]
def test_block_not_lambda(t : T?) {
    var inscope expr <- qmacro(@() { return 1; })
    let r = qmatch(expr, $ { return 1; })
    t |> success(!r.matched, "plain block pattern should not match lambda")
}

[test]
def test_lambda_matches_lambda(t : T?) {
    var inscope expr <- qmacro(@(x) { return x + 1; })
    let r = qmatch(expr, @(x) { return x + 1; })
    t |> success(r.matched, "lambda should match lambda")
}

[test]
def test_lambda_not_block(t : T?) {
    var inscope expr <- qmacro($ { return 1; })
    let r = qmatch(expr, @() { return 1; })
    t |> success(!r.matched, "lambda pattern should not match plain block")
}

[test]
def test_local_function_matches(t : T?) {
    var inscope expr <- qmacro(@@(x) { return x + 1; })
    let r = qmatch(expr, @@(x) { return x + 1; })
    t |> success(r.matched, "local function should match local function")
}

[test]
def test_local_function_not_lambda(t : T?) {
    var inscope expr <- qmacro(@(x) { return x + 1; })
    let r = qmatch(expr, @@(x) { return x + 1; })
    t |> success(!r.matched, "local function pattern should not match lambda")
}

// ── Post-inference local function matching (ExprAddr fallback) ──────────

[export]
def target_with_local_fn() {
    var f = @@(x : int) { return x + 1; }
    feint("{f}")
}

[export]
def target_with_local_fn_wrong() {
    var f = @@(x : int) { return x - 1; }
    feint("{f}")
}

[test]
def test_local_fn_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_with_local_fn)
    t |> success(get_ptr(func) != null, "should find target_with_local_fn")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var f = @@(x : int) { return x + 1; }
        feint("{f}")
    }
    t |> success(r.matched, "should match post-inference local function via ExprAddr")
}

[test]
def test_local_fn_post_inference_wrong_body(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_with_local_fn)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var f = @@(x : int) { return x - 1; }
        feint("{f}")
    }
    t |> success(!r.matched, "wrong body in local function should fail")
}

[test]
def test_local_fn_capture_body(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_with_local_fn)
    if (get_ptr(func) == null) {
        return
    }
    var inscope body_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        var f = @@(x : int) { $e(body_expr); }
        feint("{f}")
    }
    t |> success(r.matched, "should match with $e capture on local fn body")
    if (r.matched) {
        t |> equal(string(get_ptr(body_expr).__rtti), "ExprReturn")
    }
}

[export]
def target_with_local_fn_two_args() {
    var f = @@(a : int; b : float) { return float(a) + b; }
    feint("{f}")
}

[test]
def test_local_fn_match_two_args(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_with_local_fn_two_args)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var f = @@(a : int; b : float) { return float(a) + b; }
        feint("{f}")
    }
    t |> success(r.matched, "should match local fn with two args")
}

[test]
def test_local_fn_capture_var_name(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_with_local_fn)
    if (get_ptr(func) == null) {
        return
    }
    var vname : string
    let r = qmatch_function(func) $() {
        var $i(vname) = @@(x : int) { return x + 1; }
        feint("{f}")
    }
    t |> success(r.matched, "should match with $i capture on let variable name")
    if (r.matched) {
        t |> equal(vname, "f")
    }
}

// ── Post-inference comprehension matching (ExprInvoke fallback) ─────────

[export]
def target_comp_array() {
    var a <- [for (x in range(10)); x * x]
    feint("{a}")
}

[export]
def target_comp_table() {
    var t <- {for (x in range(10)); x => x * x}
    feint("{t}")
}

[export]
def target_comp_where() {
    var a <- [for (x in range(10)); x * x; where x > 3]
    feint("{a}")
}

[test]
def test_comp_array_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_array)
    t |> success(get_ptr(func) != null, "should find target_comp_array")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var a <- [for (x in range(10)); x * x]
        feint("{a}")
    }
    t |> success(r.matched, "should match post-inference array comprehension")
}

[test]
def test_comp_table_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_table)
    t |> success(get_ptr(func) != null, "should find target_comp_table")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var t <- {for (x in range(10)); x => x * x}
        feint("{t}")
    }
    t |> success(r.matched, "should match post-inference table comprehension")
}

[test]
def test_comp_where_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_where)
    t |> success(get_ptr(func) != null, "should find target_comp_where")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var a <- [for (x in range(10)); x * x; where x > 3]
        feint("{a}")
    }
    t |> success(r.matched, "should match post-inference comprehension with where")
}

[test]
def test_comp_wrong_body_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_array)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $() {
        var a <- [for (x in range(10)); x + x]
        feint("{a}")
    }
    t |> success(!r.matched, "wrong subexpr should fail post-inference")
}

[test]
def test_comp_capture_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_array)
    if (get_ptr(func) == null) {
        return
    }
    var inscope sub_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        var a <- [for (x in range(10)); $e(sub_expr)]
        feint("{a}")
    }
    t |> success(r.matched, "should match with $e capture on comprehension subexpr")
    if (r.matched) {
        t |> success(sub_expr != null, "captured subexpr should not be null")
    }
}

[test]
def test_comp_capture_iterator_post_inference(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_comp_array)
    if (get_ptr(func) == null) {
        return
    }
    var iter_name : string
    let r = qmatch_function(func) $() {
        var a <- [for ($i(iter_name) in range(10)); $i(iter_name) * $i(iter_name)]
        feint("{a}")
    }
    t |> success(r.matched, "should match with $i capture on iterator name")
    if (r.matched) {
        t |> equal(iter_name, "x")
    }
}
