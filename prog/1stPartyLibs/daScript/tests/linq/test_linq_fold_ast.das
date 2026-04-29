options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require daslib/linq
require daslib/linq_boost
require dastest/testing_boost public

// ── Target functions (fold happens at macro time) ──────────────────────

[export, marker(no_coverage)]
def target_where_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._where(_ > 3)._fold()
}

[export, marker(no_coverage)]
def target_select_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._select(_ * 2)._fold()
}

[export, marker(no_coverage)]
def target_where_select_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._where(_ > 3)._select(_ * 2)._fold()
}

[export, marker(no_coverage)]
def target_select_where_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._select(_ * 2)._where(_ > 6)._fold()
}

[export, marker(no_coverage)]
def target_reverse_where_fold() : array<int> {
    return <- [1, 2, 3, 4, 5].to_sequence().reverse()._where(_ > 3).to_array()._fold()
}

[export, marker(no_coverage)]
def target_zip_fold() : array<tuple<int; int>> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1))._fold()
}

[export, marker(no_coverage)]
def target_zip3_fold() : array<tuple<int; int; int>> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1), [100, 200, 300]._select(_ / 10))._fold()
}

[export, marker(no_coverage)]
def target_zip3_predicate_fold() : array<int> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1), [100, 200, 300]._select(_ / 10), $(a, b, c : int) => a + b + c)._fold()
}

// ── Tests: fold_where — comprehension with where ───────────────────────

[test]
def test_where_fold_produces_comprehension(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_where_fold)
    t |> success(get_ptr(func) != null, "should find target_where_fold")
    if (get_ptr(func) == null) {
        return
    }
    // fold_where output: invoke($(var source) .. var pass_0 <- COMP; return <- pass_0 .., src)
    var inscope comp_expr : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- $e(comp_expr)
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match fold invoke structure, error={int(r.error)}")
    if (!r.matched) {
        return
    }
    // Verify the captured expression is a comprehension with where
    var inscope resolved <- qm_resolve_comprehension(comp_expr)
    t |> success(resolved != null, "inner expression should be a comprehension")
    if (resolved == null) {
        return
    }
    let ac = resolved as ExprArrayComprehension
    t |> success(ac.exprWhere != null, "comprehension should have where clause")
}

[test]
def test_where_fold_comprehension_pattern(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_where_fold)
    if (get_ptr(func) == null) {
        return
    }
    // Match the full structure including comprehension pattern
    var inscope where_cond : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- [for (it in source); it; where $e(where_cond)]
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match comprehension with where, error={int(r.error)}")
}

// ── Tests: fold_select — comprehension without where ───────────────────

[test]
def test_select_fold_produces_comprehension(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_select_fold)
    t |> success(get_ptr(func) != null, "should find target_select_fold")
    if (get_ptr(func) == null) {
        return
    }
    var inscope comp_expr : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- $e(comp_expr)
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match fold structure, error={int(r.error)}")
    if (!r.matched) {
        return
    }
    var inscope resolved <- qm_resolve_comprehension(comp_expr)
    t |> success(resolved != null, "inner should be a comprehension")
    if (resolved == null) {
        return
    }
    let ac = resolved as ExprArrayComprehension
    t |> success(ac.exprWhere == null, "select-only comprehension should have no where clause")
}

[test]
def test_select_fold_comprehension_pattern(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_select_fold)
    if (get_ptr(func) == null) {
        return
    }
    var inscope select_expr : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- [for (it in source); $e(select_expr)]
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match comprehension without where, error={int(r.error)}")
    if (!r.matched) {
        return
    }
    // Verify the select expression is a multiplication: it * 2
    let r2 = qmatch(select_expr, it * 2)
    t |> success(r2.matched, "select expression should be it * 2")
}

// ── Tests: fold_where_select — comprehension with both ─────────────────

[test]
def test_where_select_fold_comprehension(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_where_select_fold)
    t |> success(get_ptr(func) != null, "should find target_where_select_fold")
    if (get_ptr(func) == null) {
        return
    }
    var inscope select_expr : ExpressionPtr
    var inscope where_cond : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- [for (it in source); $e(select_expr); where $e(where_cond)]
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match comprehension with where+select, error={int(r.error)}")
    if (!r.matched) {
        return
    }
    // Verify select is multiplication and where is comparison
    let r_sel = qmatch(select_expr, it * 2)
    t |> success(r_sel.matched, "select should be it * 2")
    let r_where = qmatch(where_cond, it > 3)
    t |> success(r_where.matched, "where should be it > 3")
}

// ── Tests: fold_select_where — not a simple comprehension ──────────────

[test]
def test_select_where_fold_structure(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_select_where_fold)
    t |> success(get_ptr(func) != null, "should find target_select_where_fold")
    if (get_ptr(func) == null) {
        return
    }
    // select_where fold produces an invoke with a lambda that has a for loop + if
    // It is NOT a simple comprehension - verify the fold still happened
    var inscope inner_expr : ExpressionPtr
    var inscope source_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- invoke($(var source : array<int>) {
            var pass_0 : array<int> <- $e(inner_expr)
            return <- pass_0
        }, $e(source_expr))
    }
    t |> success(r.matched, "should match fold invoke structure, error={int(r.error)}")
    if (!r.matched) {
        return
    }
    // The inner expression should NOT be a comprehension (select_where uses a different strategy)
    var inscope resolved <- qm_resolve_comprehension(inner_expr)
    t |> success(resolved == null, "select_where should not produce a simple comprehension")
}

// ── Tests: multi-step fold (reverse + where) ───────────────────────────

[test]
def test_reverse_where_fold_structure(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_reverse_where_fold)
    t |> success(get_ptr(func) != null, "should find target_reverse_where_fold")
    if (get_ptr(func) == null) {
        return
    }
    // Multi-step fold: reverse_to_array + where comprehension
    var inscope body_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- $e(body_expr)
    }
    t |> success(r.matched, "should have a return expression")
    if (!r.matched) {
        return
    }
    t |> success(body_expr is ExprInvoke, "fold should produce invoke wrapper")
}

// ── Tests: zip fold with recursive subexpression folding ───────────────

[test]
def test_zip_fold_structure(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_zip_fold)
    t |> success(get_ptr(func) != null, "should find target_zip_fold")
    if (get_ptr(func) == null) {
        return
    }
    // zip fold recursively folds the second argument
    var inscope body_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- $e(body_expr)
    }
    t |> success(r.matched, "should match return expression")
    if (!r.matched) {
        return
    }
    t |> success(body_expr is ExprInvoke, "fold should produce invoke wrapper")
}

// ── Behavioral verification (fold produces correct results) ────────────

[test]
def test_where_fold_result(t : T?) {
    t |> run("where fold produces correct values") @(t : T?) {
        let result <- target_where_fold()
        t |> equal(length(result), 2)
        t |> equal(result[0], 4)
        t |> equal(result[1], 5)
    }
}

[test]
def test_select_fold_result(t : T?) {
    t |> run("select fold produces correct values") @(t : T?) {
        let result <- target_select_fold()
        t |> equal(length(result), 5)
        let expected = [2, 4, 6, 8, 10]
        for (i, v in 0..5, result) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_where_select_fold_result(t : T?) {
    t |> run("where_select fold produces correct values") @(t : T?) {
        let result <- target_where_select_fold()
        t |> equal(length(result), 2)
        t |> equal(result[0], 8)
        t |> equal(result[1], 10)
    }
}

[test]
def test_select_where_fold_result(t : T?) {
    t |> run("select_where fold produces correct values") @(t : T?) {
        let result <- target_select_where_fold()
        t |> equal(length(result), 2)
        t |> equal(result[0], 8)
        t |> equal(result[1], 10)
    }
}

[test]
def test_zip_fold_result(t : T?) {
    t |> run("zip fold produces correct values") @(t : T?) {
        let result <- target_zip_fold()
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 2)
        t |> equal(result[0]._1, 11)
        t |> equal(result[1]._0, 4)
        t |> equal(result[1]._1, 21)
        t |> equal(result[2]._0, 6)
        t |> equal(result[2]._1, 31)
    }
}

// ── Tests: zip3 fold — all 3 subexpressions fold ──────────────────────

[test]
def test_zip3_fold_structure(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_zip3_fold)
    t |> success(get_ptr(func) != null, "should find target_zip3_fold")
    if (get_ptr(func) == null) {
        return
    }
    // zip3 fold: all three sources should be folded into invoke wrappers
    var inscope body_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- $e(body_expr)
    }
    t |> success(r.matched, "should match return expression")
    if (!r.matched) {
        return
    }
    t |> success(body_expr is ExprInvoke, "zip3 fold should produce invoke wrapper")
}

[test]
def test_zip3_predicate_fold_structure(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_zip3_predicate_fold)
    t |> success(get_ptr(func) != null, "should find target_zip3_predicate_fold")
    if (get_ptr(func) == null) {
        return
    }
    var inscope body_expr : ExpressionPtr
    let r = qmatch_function(func) $() {
        return <- $e(body_expr)
    }
    t |> success(r.matched, "should match return expression")
    if (!r.matched) {
        return
    }
    t |> success(body_expr is ExprInvoke, "zip3 predicate fold should produce invoke wrapper")
}

[test]
def test_zip3_fold_result(t : T?) {
    t |> run("zip3 fold produces correct tuples") @(t : T?) {
        let result <- target_zip3_fold()
        t |> equal(length(result), 3)
        // [1,2,3]._select(_*2) = [2,4,6]
        // [10,20,30]._select(_+1) = [11,21,31]
        // [100,200,300]._select(_/10) = [10,20,30]
        t |> equal(result[0]._0, 2)
        t |> equal(result[0]._1, 11)
        t |> equal(result[0]._2, 10)
        t |> equal(result[1]._0, 4)
        t |> equal(result[1]._1, 21)
        t |> equal(result[1]._2, 20)
        t |> equal(result[2]._0, 6)
        t |> equal(result[2]._1, 31)
        t |> equal(result[2]._2, 30)
    }
}

[test]
def test_zip3_predicate_fold_result(t : T?) {
    t |> run("zip3 with predicate fold produces correct sums") @(t : T?) {
        let result <- target_zip3_predicate_fold()
        t |> equal(length(result), 3)
        // (2+11+10), (4+21+20), (6+31+30)
        t |> equal(result[0], 23)
        t |> equal(result[1], 45)
        t |> equal(result[2], 67)
    }
}
