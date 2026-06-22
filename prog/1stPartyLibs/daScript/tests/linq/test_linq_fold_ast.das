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

// `_fold` targets — used by behavioral *_fold_result tests + new loop-AST tests.
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
def target_reverse_where_fold() : array<int> {
    return <- [1, 2, 3, 4, 5].to_sequence().reverse()._where(_ > 3).to_array()._fold()
}

[export, marker(no_coverage)]
def target_zip_fold() : array<tuple<int; int>> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1))._fold()
}

// Phase 2 Z1 targets — bare zip (no upstream select) hits plan_zip splice.
[export, marker(no_coverage)]
def target_zip_bare_arr_fold() : array<tuple<int; int>> {
    return <- [1, 2, 3].zip([10, 20, 30])._fold()
}

[export, marker(no_coverage)]
def target_zip_bare_iter_fold() : array<tuple<int; int>> {
    return <- zip([1, 2, 3].each(), [10, 20, 30].each()).to_array()._fold()
}

[export, marker(no_coverage)]
def target_zip_bare_iter_seq_fold() : iterator<tuple<int; int>> {
    return <- zip([1, 2, 3].each(), [10, 20, 30].each())._fold()
}

[export, marker(no_coverage)]
def target_zip_bare_unequal_fold() : array<tuple<int; int>> {
    return <- [1, 2, 3, 4, 5].zip([10, 20, 30])._fold()
}

// Phase 2 Z2 targets — count / long_count terminators on bare zip.
[export, marker(no_coverage)]
def target_zip_count_fold() : int {
    return _fold([1, 2, 3].zip([10, 20, 30]).count())
}

[export, marker(no_coverage)]
def target_zip_long_count_fold() : int64 {
    return _fold([1, 2, 3].zip([10, 20, 30]).long_count())
}

[export, marker(no_coverage)]
def target_zip_count_iter_fold() : int {
    // to_sequence_move yields a real iterator (no peel, no length) → counter loop fallback.
    return _fold(zip([1, 2, 3, 4, 5].to_sequence_move(), [10, 20, 30].to_sequence_move()).count())
}

[export, marker(no_coverage)]
def target_zip_count_unequal_fold() : int {
    return _fold([1, 2, 3, 4, 5].zip([10, 20, 30]).count())
}

// Phase 2 Z3 targets — chain ops between zip and terminator (where_/select/take/skip/take_while/skip_while).
// All use the `$(t : tuple<int;int>)` block form so the typer accepts the chain at the inner-arg level (needed before plan_zip fires).
[export, marker(no_coverage)]
def target_zip_where_arr_fold() : array<tuple<int; int>> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_select_arr_fold() : array<int> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_where_select_fold() : array<int> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> select($(t : tuple<int; int>) => t._0 + t._1) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_take_fold() : array<tuple<int; int>> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> take(2) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_skip_fold() : array<tuple<int; int>> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> skip(3) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_take_while_fold() : array<tuple<int; int>> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> take_while($(t : tuple<int; int>) => t._0 < 30) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_skip_while_fold() : array<tuple<int; int>> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> skip_while($(t : tuple<int; int>) => t._0 < 30) |> _fold()
}

[export, marker(no_coverage)]
def target_zip_where_count_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> count())
}

[export, marker(no_coverage)]
def target_zip_take_count_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> take(3) |> count())
}

[export, marker(no_coverage)]
def target_zip_select_count_fold() : int {
    // Pure projection + count: the counter lane skips evaluating projection (no side effect to preserve), so this still beats a per-elem proj eval.
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> count())
}

// Phase 2 Z6 bail target — result-selector zip form. plan_zip bails on zipArgCount != 2; tier-2 cascade (fold_linq_default) handles it. Test only that result is correct end-to-end.
[export, marker(no_coverage)]
def target_zip_result_selector_bail() : array<int> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5], $(p : int; q : int) => p + q) |> _fold()
}

// Phase 2 Z6 bail target — chain op (order) that plan_zip doesn't fuse. Tier-2 cascade handles it.
[export, marker(no_coverage)]
def target_zip_order_bail() : array<int> {
    return <- [30, 10, 40, 20, 50].zip([3, 1, 4, 2, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> order() |> _fold()
}

// PR #2741 Copilot review thread #r3269663361 — impure select projection ahead of a range op. Eager pipeline runs proj on every element; spliced order would skip it on dropped items, so plan_zip bails. Track proj-eval count via a global counter to assert tier-2 semantics survived.
var g_zip_proj_hits = 0

def private side_effect_zip_proj(p : int) : int {
    g_zip_proj_hits++
    return p
}

[export, marker(no_coverage)]
def target_zip_impure_select_skip_bails() : array<int> {
    return <- [10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => side_effect_zip_proj(t._0)) |> skip(2) |> _fold()
}

// ── PR Phase 2B+ — accumulator (sum/min/max/average) terminators on zip ──
// sum/min/max/average require a `select` projection to scalarize the tuple element (accumulator ops aren't defined on tuples). `long_count` is the exception: it routes through the COUNTER path locally (length shortcut + counter loop) since the accumulator is just `int64 acc++` regardless of element type.
[export, marker(no_coverage)]
def target_zip_sum_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> sum())
}

[export, marker(no_coverage)]
def target_zip_min_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 - t._1) |> min())
}

[export, marker(no_coverage)]
def target_zip_max_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 * t._1) |> max())
}

[export, marker(no_coverage)]
def target_zip_average_proj_fold() : double {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> average())
}

[export, marker(no_coverage)]
def target_zip_where_sum_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> select($(t : tuple<int; int>) => t._0 + t._1) |> sum())
}

[export, marker(no_coverage)]
def target_zip_where_long_count_fold() : int64 {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> long_count())
}

// ── PR Phase 2B+ — early-exit (first/first_or_default/any/all/contains) terminators on zip ──
[export, marker(no_coverage)]
def target_zip_first_no_proj_fold() : tuple<int; int> {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> first())
}

[export, marker(no_coverage)]
def target_zip_first_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> first())
}

[export, marker(no_coverage)]
def target_zip_where_first_fold() : tuple<int; int> {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> where_($(t : tuple<int; int>) => t._0 > 20) |> first())
}

[export, marker(no_coverage)]
def target_zip_first_or_default_proj_fold() : int {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> where_($(v : int) => v > 1000) |> first_or_default(-1))
}

[export, marker(no_coverage)]
def target_zip_any_no_pred_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> any())
}

[export, marker(no_coverage)]
def target_zip_any_no_pred_empty_fold() : bool {
    var emptyA : array<int>
    return _fold(emptyA.zip([1, 2, 3]) |> any())
}

[export, marker(no_coverage)]
def target_zip_any_pred_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> any($(t : tuple<int; int>) => t._0 > 30))
}

[export, marker(no_coverage)]
def target_zip_all_pred_true_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> all($(t : tuple<int; int>) => t._0 > 0))
}

[export, marker(no_coverage)]
def target_zip_all_pred_false_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> all($(t : tuple<int; int>) => t._0 > 30))
}

[export, marker(no_coverage)]
def target_zip_contains_proj_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> contains(33))
}

[export, marker(no_coverage)]
def target_zip_contains_proj_miss_fold() : bool {
    return _fold([10, 20, 30, 40, 50].zip([1, 2, 3, 4, 5]) |> select($(t : tuple<int; int>) => t._0 + t._1) |> contains(999))
}

// Empty-source average target. Verifies the 2-site emit_accumulator_lane.average + emit_decs_accumulator.average alignment with linq.das (double accumulator + cnt==0 guard → 0.0lf). See test_zip_average_empty_returns_zero below.
[export, marker(no_coverage)]
def target_zip_average_empty_fold() : double {
    var emptyA : array<int>
    var emptyB : array<int>
    return _fold(emptyA.zip(emptyB) |> select($(t : tuple<int; int>) => t._0 + t._1) |> average())
}

[export, marker(no_coverage)]
def target_zip3_fold() : array<tuple<int; int; int>> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1), [100, 200, 300]._select(_ / 10))._fold()
}

[export, marker(no_coverage)]
def target_zip3_predicate_fold() : array<int> {
    return <- [1, 2, 3]._select(_ * 2).zip([10, 20, 30]._select(_ + 1), [100, 200, 300]._select(_ / 10), $(a, b, c : int) => a + b + c)._fold()
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

// Phase 2 Z1 — behavioral + AST-shape coverage for bare zip splice.

[test]
def test_zip_bare_arr_fold_result(t : T?) {
    t |> run("bare zip(arr, arr) produces matched tuples") @(t : T?) {
        let result <- target_zip_bare_arr_fold()
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 1);  t |> equal(result[0]._1, 10)
        t |> equal(result[1]._0, 2);  t |> equal(result[1]._1, 20)
        t |> equal(result[2]._0, 3);  t |> equal(result[2]._1, 30)
    }
}

[test]
def test_zip_bare_iter_fold_result(t : T?) {
    t |> run("bare zip(iter, iter) |> to_array produces matched tuples") @(t : T?) {
        let result <- target_zip_bare_iter_fold()
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 1);  t |> equal(result[0]._1, 10)
        t |> equal(result[2]._0, 3);  t |> equal(result[2]._1, 30)
    }
}

[test]
def test_zip_bare_iter_seq_fold_result(t : T?) {
    t |> run("bare zip(iter, iter) returns iterator that yields matched tuples") @(t : T?) {
        var n = 0
        for (pair in target_zip_bare_iter_seq_fold()) {
            t |> equal(pair._0, n + 1)
            t |> equal(pair._1, (n + 1) * 10)
            n ++
        }
        t |> equal(n, 3)
    }
}

[test]
def test_zip_bare_unequal_fold_result(t : T?) {
    t |> run("bare zip truncates to shorter source") @(t : T?) {
        let result <- target_zip_bare_unequal_fold()
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 1);  t |> equal(result[0]._1, 10)
        t |> equal(result[2]._0, 3);  t |> equal(result[2]._1, 30)
    }
}

[test]
def test_zip_bare_arr_emits_single_for(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_bare_arr_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "bare zip splice must emit exactly one (multi-iter) for-loop")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip call must be inlined into the for-loop, not preserved")
    }
}

[test]
def test_zip_bare_iter_emits_single_for(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_bare_iter_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "iter+iter zip splice must emit exactly one for-loop")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip call must be inlined, not preserved")
    }
}

// Phase 2 Z2 — behavioral + AST-shape for count/long_count terminators.

[test]
def test_zip_count_fold_result(t : T?) {
    t |> run("zip(arr, arr).count() returns matched length") @(t : T?) {
        t |> equal(target_zip_count_fold(), 3)
    }
}

[test]
def test_zip_long_count_fold_result(t : T?) {
    t |> run("zip(arr, arr).long_count() returns int64 matched length") @(t : T?) {
        t |> equal(target_zip_long_count_fold(), 3l)
    }
}

[test]
def test_zip_count_iter_fold_result(t : T?) {
    t |> run("zip(iter, iter).count() walks both and returns min length") @(t : T?) {
        t |> equal(target_zip_count_iter_fold(), 3)
    }
}

[test]
def test_zip_count_unequal_fold_result(t : T?) {
    t |> run("zip(arr, arr).count() truncates to shorter source") @(t : T?) {
        t |> equal(target_zip_count_unequal_fold(), 3)
    }
}

[test]
def test_zip_count_uses_length_shortcut(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 0, "zip(arr,arr).count() must use length shortcut (no for-loop)")
    }
}

[test]
def test_zip_long_count_uses_length_shortcut(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_long_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 0, "zip(arr,arr).long_count() must use length shortcut (no for-loop)")
        // Distinguish length-shortcut splice from tier-2 cascade by elimination: for_loops==0 rules out the counter-loop splice path; long_count==0 rules out tier-2 (which leaves the long_count call in place). Both together ⇒ length shortcut. (Can't grep for length() directly: count_call doesn't recurse into ExprOp3 ternary where length() lives in the min(...) expression.)
        let longCountCalls = count_call(body_expr, "long_count")
        t |> equal(longCountCalls, 0, "long_count must be inlined into the splice (no runtime call)")
    }
}

[test]
def test_zip_count_iter_emits_counter_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_count_iter_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "iter+iter source: count must walk both via counter loop (1 for)")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip must be inlined")
        let countCalls = count_call(body_expr, "count")
        t |> equal(countCalls, 0, "count must be inlined into acc++")
    }
}

// ── Phase 2 Z3 behavioral + AST shape tests (chain ops fused into zip splice) ─

[test]
def test_zip_where_arr_fold_result(t : T?) {
    t |> run("zip + where_ filters by tuple element") @(t : T?) {
        let result <- target_zip_where_arr_fold()
        // a=[10,20,30,40,50], b=[1..5]: pairs where a>20 → [(30,3),(40,4),(50,5)]
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 30)
        t |> equal(result[0]._1, 3)
        t |> equal(result[2]._0, 50)
        t |> equal(result[2]._1, 5)
    }
}

[test]
def test_zip_select_arr_fold_result(t : T?) {
    t |> run("zip + select projects per element") @(t : T?) {
        let result <- target_zip_select_arr_fold()
        // sum each pair: [11, 22, 33, 44, 55]
        t |> equal(length(result), 5)
        t |> equal(result[0], 11)
        t |> equal(result[4], 55)
    }
}

[test]
def test_zip_where_select_fold_result(t : T?) {
    t |> run("zip + where + select chains predicate and projection") @(t : T?) {
        let result <- target_zip_where_select_fold()
        // filter a>20, then sum: [33, 44, 55]
        t |> equal(length(result), 3)
        t |> equal(result[0], 33)
        t |> equal(result[2], 55)
    }
}

[test]
def test_zip_take_fold_result(t : T?) {
    t |> run("zip + take limits to N pairs") @(t : T?) {
        let result <- target_zip_take_fold()
        t |> equal(length(result), 2)
        t |> equal(result[0]._0, 10)
        t |> equal(result[1]._0, 20)
    }
}

[test]
def test_zip_skip_fold_result(t : T?) {
    t |> run("zip + skip drops K pairs from the front") @(t : T?) {
        let result <- target_zip_skip_fold()
        t |> equal(length(result), 2)
        t |> equal(result[0]._0, 40)
        t |> equal(result[1]._0, 50)
    }
}

[test]
def test_zip_take_while_fold_result(t : T?) {
    t |> run("zip + take_while yields prefix that matches predicate") @(t : T?) {
        let result <- target_zip_take_while_fold()
        // take while a < 30 → [(10,1),(20,2)]
        t |> equal(length(result), 2)
        t |> equal(result[0]._0, 10)
        t |> equal(result[1]._0, 20)
    }
}

[test]
def test_zip_skip_while_fold_result(t : T?) {
    t |> run("zip + skip_while yields tail after first false") @(t : T?) {
        let result <- target_zip_skip_while_fold()
        // skip while a < 30 → [(30,3),(40,4),(50,5)]
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 30)
        t |> equal(result[2]._0, 50)
    }
}

[test]
def test_zip_where_count_fold_result(t : T?) {
    t |> run("zip + where + count returns matched count") @(t : T?) {
        t |> equal(target_zip_where_count_fold(), 3)
    }
}

[test]
def test_zip_take_count_fold_result(t : T?) {
    t |> run("zip + take + count returns min(N, len)") @(t : T?) {
        t |> equal(target_zip_take_count_fold(), 3)
    }
}

[test]
def test_zip_select_count_fold_result(t : T?) {
    t |> run("zip + select + count returns full length (pure projection)") @(t : T?) {
        t |> equal(target_zip_select_count_fold(), 5)
    }
}

[test]
def test_zip_where_emits_single_for_with_filter(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_where_arr_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "zip+where must emit exactly one for-loop")
        let whereCalls = count_call(body_expr, "where_")
        t |> equal(whereCalls, 0, "where_ call must be inlined into if-wrap, not preserved")
    }
}

[test]
def test_zip_select_emits_inline_projection(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_select_arr_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "zip+select must emit one for-loop with inline projection")
        let selectCalls = count_call(body_expr, "select")
        t |> equal(selectCalls, 0, "select call must be inlined into push_clone(projection)")
    }
}

[test]
def test_zip_take_emits_break_in_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_take_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 1, "zip+take emits a single for-loop with break-guard")
        let breaks = count_break_continue(body_expr, true)
        t |> success(breaks >= 1, "zip+take must splice a `break` into the for-body")
        let takeCalls = count_call(body_expr, "take")
        t |> equal(takeCalls, 0, "take call must be inlined as counter+break")
    }
}

// Phase 2 Z6 bail behavioral tests — these compile and run through tier-2 cascade (fold_linq_default), not plan_zip. Verify correctness only.

[test]
def test_zip_result_selector_bail_correct(t : T?) {
    t |> run("zip(a, b, selector) form bails to tier-2 cascade and computes correct result") @(t : T?) {
        let result <- target_zip_result_selector_bail()
        // [10..50].zip([1..5], p+q) = [11, 22, 33, 44, 55]
        t |> equal(length(result), 5)
        t |> equal(result[0], 11)
        t |> equal(result[4], 55)
    }
}

[test]
def test_zip_order_bail_correct(t : T?) {
    t |> run("zip + select + order bails to tier-2 cascade and produces sorted result") @(t : T?) {
        let result <- target_zip_order_bail()
        // a=[30,10,40,20,50] b=[3,1,4,2,5]: pair sums [33,11,44,22,55] sorted ascending → [11,22,33,44,55]
        t |> equal(length(result), 5)
        t |> equal(result[0], 11)
        t |> equal(result[4], 55)
    }
}

// PR #2741 Copilot review #r3269663361: impure projection + range op must bail so the projection runs on every element (eager pipeline semantics).
[test]
def test_zip_impure_select_skip_bails_to_tier2(t : T?) {
    t |> run("impure select + skip bails to tier-2; projection runs on every element") @(t : T?) {
        g_zip_proj_hits = 0
        let result <- target_zip_impure_select_skip_bails()
        // skip(2) on a length-5 stream → 3 surviving elements [30, 40, 50]
        t |> equal(length(result), 3)
        // Critical: projection must have fired 5 times (once per input), not 3 (only survivors)
        t |> equal(g_zip_proj_hits, 5)
    }
}

// PR #2741 Copilot review #r3269663397: pure `zip+select+count` skips the `let it = (itA, itB)` body bind. Inner for-body should be a single stmt (`acc++`), not 2 (bind + acc++).
[test]
def test_zip_select_count_pure_skips_it_bind(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_select_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let nfor = count_inner_for_loops(body_expr)
        t |> equal(nfor, 1, "pure zip+select+count uses counter for-loop")
        let nbody = count_for_body_stmts(body_expr)
        t |> equal(nbody, 1, "pure zip+select+count counter body is just `acc++` — no `let it = (itA, itB)` bind")
    }
}

// ── PR Phase 2B+ accumulator (sum/min/max/average) terminators on zip ──
// Source: arrA=[10,20,30,40,50], arrB=[1,2,3,4,5]; pairs are (10,1)(20,2)(30,3)(40,4)(50,5).

[test]
def test_zip_sum_proj_fold_result(t : T?) {
    t |> run("zip+select+sum returns scalar accumulator") @(t : T?) {
        // (10+1)+(20+2)+(30+3)+(40+4)+(50+5) = 11+22+33+44+55 = 165
        t |> equal(target_zip_sum_proj_fold(), 165)
    }
}

[test]
def test_zip_min_proj_fold_result(t : T?) {
    t |> run("zip+select+min picks smallest projected value") @(t : T?) {
        // diffs: 9,18,27,36,45 → min 9
        t |> equal(target_zip_min_proj_fold(), 9)
    }
}

[test]
def test_zip_max_proj_fold_result(t : T?) {
    t |> run("zip+select+max picks largest projected value") @(t : T?) {
        // products: 10,40,90,160,250 → max 250
        t |> equal(target_zip_max_proj_fold(), 250)
    }
}

[test]
def test_zip_average_proj_fold_result(t : T?) {
    t |> run("zip+select+average returns double") @(t : T?) {
        // (11+22+33+44+55)/5 = 33.0
        t |> equal(target_zip_average_proj_fold(), 33.0lf)
    }
}

[test]
def test_zip_where_sum_proj_fold_result(t : T?) {
    t |> run("zip+where+select+sum filters then sums") @(t : T?) {
        // survivors where t._0>20: (30,3)(40,4)(50,5) → 33+44+55 = 132
        t |> equal(target_zip_where_sum_proj_fold(), 132)
    }
}

[test]
def test_zip_where_long_count_fold_result(t : T?) {
    t |> run("zip+where+long_count returns int64 survivor count") @(t : T?) {
        // 3 survivors
        t |> equal(target_zip_where_long_count_fold(), 3l)
    }
}

// long_count + chain op (where) must emit a single multi-iter counter for-loop, not cascade to tier-2. Regression guard against routing long_count through the ACCUMULATOR projection-required branch (PR #2742 Copilot review #r3270242609).
[test]
def test_zip_where_long_count_emits_counter_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_where_long_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let nfor = count_inner_for_loops(body_expr)
        t |> equal(nfor, 1, "zip+where+long_count must emit one multi-iter counter for-loop")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip call must be inlined into the splice")
        let longCountCalls = count_call(body_expr, "long_count")
        t |> equal(longCountCalls, 0, "long_count terminator must be fused (no runtime call)")
    }
}

// ── PR Phase 2B+ early-exit (first/first_or_default/any/all/contains) on zip ──

[test]
def test_zip_first_no_proj_fold_result(t : T?) {
    t |> run("zip+first (no projection) returns first tuple") @(t : T?) {
        let result = target_zip_first_no_proj_fold()
        t |> equal(result._0, 10)
        t |> equal(result._1, 1)
    }
}

[test]
def test_zip_first_proj_fold_result(t : T?) {
    t |> run("zip+select+first returns first projected scalar") @(t : T?) {
        // first pair (10,1) projected → 11
        t |> equal(target_zip_first_proj_fold(), 11)
    }
}

[test]
def test_zip_where_first_fold_result(t : T?) {
    t |> run("zip+where+first returns first survivor tuple") @(t : T?) {
        // first pair where t._0>20: (30,3)
        let result = target_zip_where_first_fold()
        t |> equal(result._0, 30)
        t |> equal(result._1, 3)
    }
}

[test]
def test_zip_first_or_default_proj_fold_result(t : T?) {
    t |> run("zip+select+where+first_or_default returns default when no survivor") @(t : T?) {
        // no projected sum > 1000, so default -1 fires
        t |> equal(target_zip_first_or_default_proj_fold(), -1)
    }
}

[test]
def test_zip_any_no_pred_fold_result(t : T?) {
    t |> run("zip+any (no pred) on non-empty pair → true") @(t : T?) {
        t |> equal(target_zip_any_no_pred_fold(), true)
    }
}

[test]
def test_zip_any_no_pred_empty_fold_result(t : T?) {
    t |> run("zip+any (no pred) on empty source → false") @(t : T?) {
        // emptyA.zip(...) → min(0,3) = 0 iters → any returns false
        t |> equal(target_zip_any_no_pred_empty_fold(), false)
    }
}

[test]
def test_zip_any_pred_fold_result(t : T?) {
    t |> run("zip+any(pred) returns true if any pair matches") @(t : T?) {
        // (40,4) and (50,5) satisfy t._0>30 → true
        t |> equal(target_zip_any_pred_fold(), true)
    }
}

[test]
def test_zip_all_pred_true_fold_result(t : T?) {
    t |> run("zip+all(pred) returns true when all match") @(t : T?) {
        t |> equal(target_zip_all_pred_true_fold(), true)
    }
}

[test]
def test_zip_all_pred_false_fold_result(t : T?) {
    t |> run("zip+all(pred) returns false when any fails") @(t : T?) {
        // 10, 20, 30 fail t._0>30
        t |> equal(target_zip_all_pred_false_fold(), false)
    }
}

[test]
def test_zip_contains_proj_fold_result(t : T?) {
    t |> run("zip+select+contains returns true on hit") @(t : T?) {
        // 30+3 = 33 matches
        t |> equal(target_zip_contains_proj_fold(), true)
    }
}

[test]
def test_zip_contains_proj_miss_fold_result(t : T?) {
    t |> run("zip+select+contains returns false on miss") @(t : T?) {
        t |> equal(target_zip_contains_proj_miss_fold(), false)
    }
}

// Regression guard for the 2-site average semantics fix (emit_accumulator_lane + emit_decs_accumulator align with linq.das:1545 — double accumulator + cnt==0 guard → 0.0lf on empty).
[test]
def test_zip_average_empty_returns_zero(t : T?) {
    t |> run("zip+select+average on empty source returns 0.0lf") @(t : T?) {
        t |> equal(target_zip_average_empty_fold(), 0.0lf)
    }
}

// AST-shape: zip+accumulator splices into a single for-loop with no surviving runtime `sum`/`zip` calls.
[test]
def test_zip_sum_proj_emits_inline_accumulator(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_sum_proj_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let nfor = count_inner_for_loops(body_expr)
        t |> equal(nfor, 1, "zip+select+sum must emit exactly one multi-iter for-loop")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip call must be inlined into the splice")
        let sumCalls = count_call(body_expr, "sum")
        t |> equal(sumCalls, 0, "sum terminator must be fused (no runtime sum call)")
    }
}

// AST-shape: zip+early-exit emits the same single-for shape with no surviving terminator call.
[test]
def test_zip_first_proj_emits_early_return(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_zip_first_proj_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        let nfor = count_inner_for_loops(body_expr)
        t |> equal(nfor, 1, "zip+select+first must emit exactly one multi-iter for-loop")
        let zipCalls = count_call(body_expr, "zip")
        t |> equal(zipCalls, 0, "zip call must be inlined")
        let firstCalls = count_call(body_expr, "first")
        t |> equal(firstCalls, 0, "first terminator must be fused (no runtime first call)")
    }
}

// ── Targets for `_fold` Phase-2A loop planner ──────────────────────────

[export, marker(no_coverage)]
def target_chained_where_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._where(_ > 1)._where(_ < 5)._fold()
}

[export, marker(no_coverage)]
def target_chained_select_fold() : array<int> {
    return <- [1, 2, 3, 4, 5]._select(_ * 2)._select(_ + 1)._fold()
}

[export, marker(no_coverage)]
def target_where_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 2).count())
}

[export, marker(no_coverage)]
def target_chained_where_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 1)._where(_ < 5).count())
}

[export, marker(no_coverage)]
def target_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5]).count())
}

[export, marker(no_coverage)]
def target_select_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._select(_ * 2).count())
}

var g_select_count_proj_hits = 0

def side_effect_select_proj(x : int) : int {
    g_select_count_proj_hits ++
    return x * 2
}

[export, marker(no_coverage)]
def target_select_count_fold_impure() : int {
    return _fold(each([1, 2, 3, 4, 5])._select(side_effect_select_proj(_)).count())
}

// `each(range(...))` — argument is a `range`, not an array. peel_each must NOT fire
// here; we'd otherwise replace the iterator-yielding each call with the raw range
// and downstream length-shortcut / reserve-by-length would silently misbehave.
[export, marker(no_coverage)]
def target_each_range_count() : int {
    return _fold(each(range(10))._where(_ > 5).count())
}

// ── Targets for Phase-2B Ring 1 accumulator lane ───────────────────────

[export, marker(no_coverage)]
def target_sum_fold() : int {
    return _fold(each([1, 2, 3, 4, 5]).sum())
}

[export, marker(no_coverage)]
def target_min_workhorse_fold() : int {
    return _fold(each([5, 3, 8, 1, 4]).min())
}

[export, marker(no_coverage)]
def target_max_workhorse_fold() : int {
    return _fold(each([5, 3, 8, 1, 4]).max())
}

[export, marker(no_coverage)]
def target_min_non_workhorse_fold() : tuple<int; int> {
    // tuple<int;int> is NOT workhorse — min must dispatch to `_::less` for the compare.
    // Single-return body so qmatch_function can extract the expression directly.
    return _fold(each([(1, 20), (3, 5), (2, 10)]).min())
}

[export, marker(no_coverage)]
def target_average_fold() : double {
    return _fold(each([2, 4, 6, 8]).average())
}

[export, marker(no_coverage)]
def target_long_count_loop_fold() : int64 {
    // Where filter forces the loop path (not the length shortcut).
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 2).long_count())
}

[export, marker(no_coverage)]
def target_long_count_shortcut_fold() : int64 {
    // Bare long_count on array source — pure path → length shortcut → int64(length(src)).
    return _fold(each([1, 2, 3, 4, 5]).long_count())
}

// ── Targets for Phase-2B Ring 2 early-exit lane ────────────────────────

[export, marker(no_coverage)]
def target_first_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 2).first())
}

[export, marker(no_coverage)]
def target_first_or_default_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 99).first_or_default(-1))
}

[export, marker(no_coverage)]
def target_any_loop_fold() : bool {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 3).any())
}

[export, marker(no_coverage)]
def target_any_shortcut_fold() : bool {
    // No upstream where + no per-element work + length-bearing source → `length(src) > 0`.
    return _fold(each([1, 2, 3, 4, 5]).any())
}

[export, marker(no_coverage)]
def target_all_fold() : bool {
    return _fold(each([1, 2, 3, 4, 5])._all(_ > 0))
}

[export, marker(no_coverage)]
def target_contains_fold() : bool {
    return _fold(each([1, 2, 3, 4, 5]).contains(3))
}

// ── Tests: `_fold` Phase-2A loop emission ──────────────────────────────
// Phase-2A `_fold` emits explicit for-loops inside an `invoke($block, $src)` wrapper
// (no `ExprArrayComprehension` nodes). Each test asserts the invoke wrapper exists
// and the inner body is NOT a comprehension. Out-of-scope shapes fall through
// unfolded — body is the raw chain, not an invoke.

[test]
def test_where_fold_emits_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "_fold should produce invoke wrapper")
        if (!(body_expr is ExprInvoke)) return
        var body_clone = clone_expression(body_expr)
        var maybe_comp <- qm_resolve_comprehension(body_clone)
        t |> success(maybe_comp == null, "loop planner must NOT emit a comprehension")
    }
}

[test]
def test_select_fold_emits_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "_fold should produce invoke wrapper")
        if (!(body_expr is ExprInvoke)) return
        var body_clone = clone_expression(body_expr)
        var maybe_comp <- qm_resolve_comprehension(body_clone)
        t |> success(maybe_comp == null, "loop planner must NOT emit a comprehension")
    }
}

[test]
def test_chained_where_fold_emits_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_chained_where_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        // Phase 2A fuses chained _where|_where into a single loop with && predicate
        t |> success(body_expr is ExprInvoke, "chained where should fuse into single invoke loop")
    }
}

[test]
def test_chained_select_fold_emits_loop(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_chained_select_fold)
        if (func == null) return
        // Chained _select|_select fuses via intermediate `var v_N = projection_N` bindings
        // — the next lambda's `_` is renamed straight to the prior binding's name so no
        // expression-substitution (and no ExprRef2Value-wrapping headaches) is needed.
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "chained _select|_select should fuse into single invoke loop")
    }
}

[test]
def test_where_count_fold_emits_counter(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "_where|_count should fuse into counter invoke")
    }
}

[test]
def test_chained_where_count_fold_emits_counter(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_chained_where_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "chained where + count should fuse into single counter invoke")
    }
}

[test]
def test_count_fold_emits_counter(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke, "bare count should fuse into unconditional counter invoke")
    }
}

// `select |> where` array-lane coverage is in the Phase 3d section below
// (target_select_where_to_array_splices_fold / test_select_where_*_emits_fused_loop).

// ── Behavioral parity: results of new shapes ───────────────────────────

[test]
def test_chained_where_fold_result(t : T?) {
    t |> run("chained where _fold produces correct values") @(t : T?) {
        let result <- target_chained_where_fold()
        t |> equal(length(result), 3)
        t |> equal(result[0], 2)
        t |> equal(result[1], 3)
        t |> equal(result[2], 4)
    }
}

[test]
def test_chained_select_fold_result(t : T?) {
    t |> run("chained select _fold produces correct values") @(t : T?) {
        let result <- target_chained_select_fold()
        t |> equal(length(result), 5)
        // [1,2,3,4,5] * 2 = [2,4,6,8,10] + 1 = [3,5,7,9,11]
        let expected = [3, 5, 7, 9, 11]
        for (i, v in 0..5, result) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_where_count_fold_result(t : T?) {
    t |> run("where _count _fold produces correct count") @(t : T?) {
        t |> equal(target_where_count_fold(), 3)
    }
}

[test]
def test_chained_where_count_fold_result(t : T?) {
    t |> run("chained where _count _fold produces correct count") @(t : T?) {
        t |> equal(target_chained_where_count_fold(), 3)
    }
}

[test]
def test_count_fold_result(t : T?) {
    t |> run("bare _count _fold produces source length") @(t : T?) {
        t |> equal(target_count_fold(), 5)
    }
}

[test]
def test_select_count_fold_result(t : T?) {
    t |> run("select _count _fold produces correct count (projection does not affect count value)") @(t : T?) {
        t |> equal(target_select_count_fold(), 5)
    }
}

// ── Counter-lane projection elision (has_sideeffects integration) ──────
// For pure counter chains (`_select(_ * 2).count()`, bare `.count()`, etc.) on
// length-supporting sources, the planner emits a `length(source)` shortcut and
// the for-loop is dropped entirely. For impure projections (function call w/
// side effects), the per-element loop is preserved with the discardable bind.

// Resolves the invoke wrapper's *effective* outer block. When the body uses
// `var inscope` (smart_ptr / table lifetimes), the typer wraps the whole body
// in an inner ExprBlock so its finally clause is scoped correctly. This helper
// returns the inner block in that case so for/let counting works the same way
// regardless of whether the splice used `var inscope`. Returns null if the body
// isn't the canonical invoke($block, $src) shape.
def invoke_outer_list(body_expr : Expression?) : ExprBlock? {
    if (!(body_expr is ExprInvoke)) return null
    let inv = body_expr as ExprInvoke
    if (empty(inv.arguments) || !(inv.arguments[0] is ExprMakeBlock)) return null
    let mb = inv.arguments[0] as ExprMakeBlock
    var outer = mb._block as ExprBlock
    if (outer == null) return null
    // Typer-inserted single-nested-block for `var inscope` finally scoping: unwrap once.
    if (length(outer.list) == 1 && outer.list[0] is ExprBlock) {
        outer = outer.list[0] as ExprBlock
    }
    return outer
}

// Returns the number of ExprLet/ExprFor statements in the counter-lane invoke's
// inner block. Pure shortcut: `[var src, return length(src)]` → 0 for-loops.
// Impure loop: `[var src, var acc=0, for {...}, return acc]` → 1 for-loop.
def count_inner_for_loops(body_expr : Expression?) : int {
    let outer = invoke_outer_list(body_expr)
    if (outer == null) return -1
    var n = 0
    for (stmt in outer.list) {
        if (stmt is ExprFor) {
            n ++
        }
    }
    return n
}

// Returns the number of stmts in the for-body, or -1 if no for loop exists.
def count_for_body_stmts(body_expr : Expression?) : int {
    let outer = invoke_outer_list(body_expr)
    if (outer == null) return -1
    for (stmt in outer.list) {
        if (stmt is ExprFor) {
            let fe = stmt as ExprFor
            let fbody = fe.body as ExprBlock
            if (fbody == null) return -1
            return length(fbody.list)
        }
    }
    return -1
}

[test]
def test_pure_projection_uses_length_shortcut(t : T?) {
    // `_select(_ * 2).count()` on a length-supporting source should collapse to
    // `length(source)` — no for-loop emitted at all.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 0, "pure select-count must emit length() shortcut (no for loop)")
    }
}

[test]
def test_bare_count_uses_length_shortcut(t : T?) {
    // Bare `.count()` on an array source should also use the length shortcut.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        let n = count_inner_for_loops(body_expr)
        t |> equal(n, 0, "bare count on length-supporting source must use length() shortcut")
    }
}

[test]
def test_impure_projection_keeps_bind(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_count_fold_impure)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected counter-lane invoke wrapper")
        let n = count_for_body_stmts(body_expr)
        t |> equal(n, 2, "impure projection should preserve vfinal bind (for-body has bind + ++acc)")
    }
}

// ── peel_each invariant: each(<array>) must always be peeled ──────────
// The planner's `peel_each` helper unwraps `each(x)` regardless of x's type so
// the emitted block sees the underlying container directly. Without this, the
// length() shortcut would never fire (each returns an iterator, which has no
// length) and array-lane reserve would emit against the iterator wrapper.

// Returns the second arg of the invoke (the source expression passed in). If
// it's still an ExprCall to `each`, peel didn't run.
def invoke_source_is_each_wrapped(body_expr : Expression?) : bool {
    if (!(body_expr is ExprInvoke)) return false
    let inv = body_expr as ExprInvoke
    if (length(inv.arguments) < 2 || !(inv.arguments[1] is ExprCall)) return false
    let src_call = inv.arguments[1] as ExprCall
    if (src_call.func == null) return false
    return (src_call.func.name == "each"
        || (src_call.func.fromGeneric != null && src_call.func.fromGeneric.name == "each"))
}

[test]
def test_peel_each_on_array_source(t : T?) {
    // Sanity: target_select_count_fold uses `each([1,2,3,4,5])`. After peel, the
    // invoke wrapper must NOT receive an each-wrapped source.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> success(!invoke_source_is_each_wrapped(body_expr),
            "peel_each must unwrap each(array) at macro time")
    }
}

[test]
def test_peel_each_on_bare_count(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> success(!invoke_source_is_each_wrapped(body_expr),
            "peel_each must unwrap each(array) at macro time")
    }
}

// Negative case: `each(range(...))` argument is an iterator-yielding range, not an
// array. peel_each must NOT fire — peeling would drop the each call and put the raw
// range in source position; the downstream length-shortcut and reserve hints would
// then misbehave on a non-indexable source.
[test]
def test_peel_each_skips_non_array_source(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_each_range_count)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> success(invoke_source_is_each_wrapped(body_expr),
            "peel_each must keep each(range) wrapper — only arrays may be peeled")
    }
}

[test]
def test_target_each_range_count_runs(t : T?) {
    // Behavioral: ensure the iterator-source chain still compiles and produces the
    // expected count. range(10) → [0,1,2,3,4,5,6,7,8,9]; filter > 5 → 4 elements.
    t |> equal(target_each_range_count(), 4)
}

// ── Phase 2B Ring 1 — accumulator lane shape assertions ────────────────
// Recursive AST walkers: count ExprOp2 with a given operator and ExprCall with a given
// function name across the entire subtree. These let us verify the workhorse-vs-`_::less`
// branch in min/max emission and the `acc += ...` / `+= 1l` shape in sum/long_count
// without locking into the exact ExprIfThenElse nesting (which is fragile to refactors).

def count_op2(expr : Expression?; op : string) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprOp2 && (expr as ExprOp2).op == op) {
        n ++
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_op2(s, op)
        }
        for (s in b.finalList) {
            n += count_op2(s, op)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_op2(s, op)
        }
        n += count_op2(f.body, op)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_op2(i.cond, op)
        n += count_op2(i.if_true, op)
        n += count_op2(i.if_false, op)
    } elif (expr is ExprOp2) {
        let o = expr as ExprOp2
        n += count_op2(o.left, op)
        n += count_op2(o.right, op)
    } elif (expr is ExprCall) {
        let c = expr as ExprCall
        for (a in c.arguments) {
            n += count_op2(a, op)
        }
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_op2(mb._block, op)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_op2(a, op)
        }
    } elif (expr is ExprReturn) {
        let r = expr as ExprReturn
        n += count_op2(r.subexpr, op)
    } elif (expr is ExprCopy) {
        let c = expr as ExprCopy
        n += count_op2(c.left, op)
        n += count_op2(c.right, op)
    } elif (expr is ExprMove) {
        let m = expr as ExprMove
        n += count_op2(m.left, op)
        n += count_op2(m.right, op)
    } elif (expr is ExprClone) {
        let c = expr as ExprClone
        n += count_op2(c.left, op)
        n += count_op2(c.right, op)
    } elif (expr is ExprOp1) {
        let o = expr as ExprOp1
        n += count_op2(o.subexpr, op)
    } elif (expr is ExprLet) {
        let l = expr as ExprLet
        for (v in l.variables) {
            if (v != null && v.init != null) {
                n += count_op2(v.init, op)
            }
        }
    }
    return n
}

def count_call(expr : Expression?; funcName : string) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprCall) {
        let c = expr as ExprCall
        if (c.func != null) {
            // Generic instances carry a mangled `name` (e.g. `less`17876...`); the
            // un-mangled function name lives in `func.fromGeneric.name`. Check both.
            // `das_string` supports `==` directly so no `string()` cast needed (PERF007).
            if (c.func.fromGeneric != null) {
                if (c.func.fromGeneric.name == funcName) {
                    n ++
                }
            } elif (c.func.name == funcName) {
                n ++
            }
        }
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_call(s, funcName)
        }
        for (s in b.finalList) {
            n += count_call(s, funcName)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_call(s, funcName)
        }
        n += count_call(f.body, funcName)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_call(i.cond, funcName)
        n += count_call(i.if_true, funcName)
        n += count_call(i.if_false, funcName)
    } elif (expr is ExprOp2) {
        let o = expr as ExprOp2
        n += count_call(o.left, funcName)
        n += count_call(o.right, funcName)
    } elif (expr is ExprCall) {
        let c = expr as ExprCall
        for (a in c.arguments) {
            n += count_call(a, funcName)
        }
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_call(mb._block, funcName)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_call(a, funcName)
        }
    } elif (expr is ExprReturn) {
        let r = expr as ExprReturn
        n += count_call(r.subexpr, funcName)
    } elif (expr is ExprOp1) {
        let o = expr as ExprOp1
        n += count_call(o.subexpr, funcName)
    } elif (expr is ExprLet) {
        let l = expr as ExprLet
        for (v in l.variables) {
            if (v != null && v.init != null) {
                n += count_call(v.init, funcName)
            }
        }
    }
    return n
}

def count_op1(expr : Expression?; op : string) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprOp1 && (expr as ExprOp1).op == op) {
        n ++
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_op1(s, op)
        }
        for (s in b.finalList) {
            n += count_op1(s, op)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_op1(s, op)
        }
        n += count_op1(f.body, op)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_op1(i.cond, op)
        n += count_op1(i.if_true, op)
        n += count_op1(i.if_false, op)
    } elif (expr is ExprOp2) {
        let o = expr as ExprOp2
        n += count_op1(o.left, op)
        n += count_op1(o.right, op)
    } elif (expr is ExprCall) {
        let c = expr as ExprCall
        for (a in c.arguments) {
            n += count_op1(a, op)
        }
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_op1(mb._block, op)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_op1(a, op)
        }
    } elif (expr is ExprReturn) {
        let r = expr as ExprReturn
        n += count_op1(r.subexpr, op)
    } elif (expr is ExprOp1) {
        let o = expr as ExprOp1
        n += count_op1(o.subexpr, op)
    } elif (expr is ExprLet) {
        let l = expr as ExprLet
        for (v in l.variables) {
            if (v != null && v.init != null) {
                n += count_op1(v.init, op)
            }
        }
    }
    return n
}

// Count ExprConstInt(value) occurrences anywhere in the expression tree. Used by the
// select+where dedup tests to prove that a pure projection's signature constant
// (`_ * 7919`) appears exactly once in the emission (in the projection-bind statement),
// rather than twice (inlined into predicate + cloned into valueExpr).
def count_const_int(expr : Expression?; value : int) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprConstInt && (expr as ExprConstInt).value == value) {
        n ++
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_const_int(s, value)
        }
        for (s in b.finalList) {
            n += count_const_int(s, value)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_const_int(s, value)
        }
        n += count_const_int(f.body, value)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_const_int(i.cond, value)
        n += count_const_int(i.if_true, value)
        n += count_const_int(i.if_false, value)
    } elif (expr is ExprOp2) {
        let o = expr as ExprOp2
        n += count_const_int(o.left, value)
        n += count_const_int(o.right, value)
    } elif (expr is ExprOp1) {
        let o = expr as ExprOp1
        n += count_const_int(o.subexpr, value)
    } elif (expr is ExprCall) {
        let c = expr as ExprCall
        for (a in c.arguments) {
            n += count_const_int(a, value)
        }
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_const_int(mb._block, value)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_const_int(a, value)
        }
    } elif (expr is ExprReturn) {
        let r = expr as ExprReturn
        n += count_const_int(r.subexpr, value)
    } elif (expr is ExprLet) {
        let l = expr as ExprLet
        for (v in l.variables) {
            if (v != null && v.init != null) {
                n += count_const_int(v.init, value)
            }
        }
    }
    return n
}

// Counts top-level `var` declarations in the outer block of an invoke wrapper.
// sum/long_count emit one accumulator; min/max emit two (first flag + best); average
// emits two (sum acc + count).
def count_outer_let_vars(body_expr : Expression?) : int {
    let outer = invoke_outer_list(body_expr)
    if (outer == null) return -1
    var n = 0
    for (stmt in outer.list) {
        if (stmt is ExprLet) n ++
    }
    return n
}

[test]
def test_sum_loop_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_sum_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // sum emits a single accumulator: `var acc : int = default<int>`
        t |> equal(1, count_outer_let_vars(body_expr), "sum: exactly one accumulator var")
        // Per-element update is `acc += value` — count ExprOp2 with op `+=`
        t |> success(count_op2(body_expr, "+=") >= 1, "sum: at least one '+=' per element")
    }
}

[test]
def test_min_workhorse_uses_lt(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_min_workhorse_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // Workhorse int → emit direct `<` compare, NOT `_::less` call
        t |> success(count_op2(body_expr, "<") >= 1, "min workhorse: at least one ExprOp2 '<'")
        t |> equal(0, count_call(body_expr, "less"), "min workhorse must NOT emit _::less call")
        // min/max emit two outer vars: `first_iter` flag + `best` accumulator
        t |> equal(2, count_outer_let_vars(body_expr), "min: first_iter flag + best")
    }
}

[test]
def test_max_workhorse_uses_gt(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_max_workhorse_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // Workhorse int max → direct `>` compare
        t |> success(count_op2(body_expr, ">") >= 1, "max workhorse: at least one ExprOp2 '>'")
        t |> equal(0, count_call(body_expr, "less"), "max workhorse must NOT emit _::less call")
        t |> equal(2, count_outer_let_vars(body_expr), "max: first_iter flag + best")
    }
}

[test]
def test_min_non_workhorse_uses_less(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_min_non_workhorse_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        // Non-workhorse tuple return is move-returned (`return <- ...`) — match that form.
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // tuple<int;int> is non-workhorse → fall back to _::less for the compare
        t |> success(count_call(body_expr, "less") >= 1, "min non-workhorse: at least one _::less call")
    }
}

[test]
def test_average_emits_sum_and_count(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_average_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // average emits two accumulators: sum + count
        t |> equal(2, count_outer_let_vars(body_expr), "average: sum + count accumulators")
        // Per-element work: sum += value (ExprOp2 '+=') and cnt ++ (postfix ExprOp1 '+++')
        t |> success(count_op2(body_expr, "+=") >= 1, "average: at least one '+=' for sum acc")
        t |> success(count_op1(body_expr, "++") >= 1, "average: at least one '++' for count")
    }
}

[test]
def test_long_count_loop_shape(t : T?) {
    ast_gc_guard() {
        // The where-filtered chain forces the loop path (length shortcut is gated on
        // whereCond == null) — verifies the int64 accumulator emission.
        var func = find_module_function_via_rtti(compiling_module(), @@target_long_count_loop_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_outer_let_vars(body_expr), "long_count: exactly one int64 accumulator")
        // long_count emits `acc++` (postfix ExprOp1 '+++') per matched element
        t |> success(count_op1(body_expr, "++") >= 1, "long_count: at least one '++' per matched element")
    }
}

[test]
def test_long_count_length_shortcut(t : T?) {
    ast_gc_guard() {
        // Bare long_count on array source → length shortcut elides the loop entirely.
        var func = find_module_function_via_rtti(compiling_module(), @@target_long_count_shortcut_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        let nForLoops = count_inner_for_loops(body_expr)
        t |> equal(0, nForLoops, "long_count shortcut: no for-loop (length emitted directly)")
    }
}

// `select |> where |> sum` accumulator-lane coverage is in the Phase 3d section below
// (target_select_where_sum_splices_fold / test_select_where_count_emits_fused_loop pattern).

// ── Phase 2B Ring 2 — early-exit lane shape assertions ─────────────────

[test]
def test_first_loop_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_first_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // first emits a loop with early return + tail panic. Verify presence of a panic call
        // (sentinel for first's "sequence contains no elements" branch).
        t |> success(count_call(body_expr, "panic") >= 1, "first: panic call in tail")
    }
}

[test]
def test_first_or_default_returns_d(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_first_or_default_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // first_or_default tail returns the bound default — never panics.
        t |> equal(0, count_call(body_expr, "panic"), "first_or_default: no panic call in tail")
    }
}

[test]
def test_any_loop_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_any_loop_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // any (with where) emits a loop. There IS a for-loop (not shortcut path).
        t |> equal(1, count_inner_for_loops(body_expr), "any (with where): one for-loop")
    }
}

[test]
def test_any_empty_shortcut(t : T?) {
    ast_gc_guard() {
        // Predicate-free `each(arr).any()` on length-bearing source → `!empty(src)`.
        // No for-loop, no per-element work.
        var func = find_module_function_via_rtti(compiling_module(), @@target_any_shortcut_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(0, count_inner_for_loops(body_expr), "any-shortcut: no for-loop emitted")
        t |> success(count_call(body_expr, "empty") >= 1, "any-shortcut: `empty(src)` call present")
        t |> success(count_op1(body_expr, "!") >= 1, "any-shortcut: `!empty(src)` negation present")
    }
}

[test]
def test_all_loop_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_all_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // all emits `if (!pred) return false` — at least one ExprOp1 with op `!`.
        t |> success(count_op1(body_expr, "!") >= 1, "all: at least one `!` for predicate negation")
        t |> equal(1, count_inner_for_loops(body_expr), "all: one for-loop")
    }
}

[test]
def test_contains_loop_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_contains_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // contains emits `if (it == v) return true` — at least one ExprOp2 with op `==`.
        t |> success(count_op2(body_expr, "==") >= 1, "contains: at least one `==` per element")
        t |> equal(1, count_inner_for_loops(body_expr), "contains: one for-loop")
    }
}

// `select |> where |> any` early-exit-lane coverage is in the Phase 3d section below
// (target_select_where_any_splices_fold).

// ── Phase 2C take/skip splice ──────────────────────────────────────────

[export, marker(no_coverage)]
def target_where_take_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])._where(_ % 2 == 0).take(3).count())
}

[export, marker(no_coverage)]
def target_skip_take_to_array_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5, 6, 7]).skip(2).take(3).to_array())
}

[export, marker(no_coverage)]
def target_order_by_take_falls_through() : array<int> {
    // Buffer-required (order_by) → splice planner returns null → silent fallback to raw linq.
    return <- _fold([1, 2, 3, 4, 5].to_sequence()._select(_ * 2)._order_by(_).take(2).to_array())
}

[export, marker(no_coverage)]
def target_distinct_to_array_fold() : array<int> {
    // Phase 3+ / D1: plan_distinct splices into streaming dedup (seen-set + buffer
    // populated in a single pass). Prior to D1 this chain cascaded to tier 2.
    return <- _fold([1, 2, 2, 3, 3, 3].to_sequence().distinct().to_array())
}

[export, marker(no_coverage)]
def target_take_sum_fold() : int {
    return _fold([1, 2, 3, 4, 5]._select(_ * 10).take(3).sum())
}

// AST helper — total break/continue count anywhere in the subtree. Used to assert that the
// take-limit `break` and skip-counter `continue` actually got spliced into the per-element body.
def count_break_continue(expr : Expression?; want_break : bool) : int {
    if (expr == null) return 0
    var n = 0
    if (want_break && expr is ExprBreak) {
        n ++
    } elif (!want_break && expr is ExprContinue) {
        n ++
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_break_continue(s, want_break)
        }
        for (s in b.finalList) {
            n += count_break_continue(s, want_break)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_break_continue(s, want_break)
        }
        n += count_break_continue(f.body, want_break)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_break_continue(i.cond, want_break)
        n += count_break_continue(i.if_true, want_break)
        n += count_break_continue(i.if_false, want_break)
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_break_continue(mb._block, want_break)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_break_continue(a, want_break)
        }
    }
    return n
}

[test]
def test_take_splices_break_into_loop(t : T?) {
    // where.take.count: counter lane with take guard → one for-loop + at least one break
    // (the `if (taken == N) break` guard).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_take_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_break_continue(body_expr, true) >= 1, "take guard must emit a break")
    }
}

[test]
def test_skip_take_splices_continue_and_break(t : T?) {
    // skip.take.to_array: array lane with both guards → continue (skip) + break (take).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_skip_take_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_break_continue(body_expr, true) >= 1, "take guard must emit a break")
        t |> success(count_break_continue(body_expr, false) >= 1, "skip guard must emit a continue")
    }
}

[test]
def test_take_sum_splices_in_accumulator(t : T?) {
    // select.take.sum: accumulator lane with take → one for-loop + acc += ... + break.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_take_sum_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_op2(body_expr, "+=") >= 1, "sum must emit `+=` in accumulator")
        t |> success(count_break_continue(body_expr, true) >= 1, "take guard must emit a break")
    }
}

[test]
def test_order_by_take_cascades_to_tier2(t : T?) {
    // `order_by |> take` — Phase 3 wires a BufferTopN splice arm via top_n_by.
    // Until then, the splice planner returns null and the cascade falls to tier 2
    // (fold_linq_default), which emits the array-shape invoke wrapper.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_by_take_falls_through)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(body_expr is ExprInvoke,
            "order_by |> take should cascade to fold_linq_default (invoke wrapper)")
    }
}

[test]
def test_distinct_to_array_emits_streaming_dedup(t : T?) {
    // D1: bare distinct + to_array splices into streaming dedup — single fused for-loop
    // with one key_exists check + one insert + one push_clone per source element. No
    // call to the runtime distinct/distinct_inplace helpers. Anti-cascade gate:
    // exactly two outer let-vars (the seen-set + the buffer).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_distinct_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "distinct"),
            "distinct runtime helper must NOT be called (splice elides it)")
        t |> equal(0, count_call(body_expr, "distinct_inplace"),
            "distinct_inplace must NOT be called (streaming dedup, not eager)")
        t |> success(count_call(body_expr, "key_exists") >= 1,
            "must emit key_exists check on the seen-set")
        t |> success(count_call(body_expr, "push_clone") >= 1,
            "must push unique elements into the buffer")
        t |> equal(2, count_outer_let_vars(body_expr),
            "exactly two outer vars (seen-set + buffer) — anti-cascade gate")
    }
}

// ── Phase 3 — order-family splice arms (plan_order_family) ─────────────

[export, marker(no_coverage)]
def target_order_by_take_splices_fold() : array<int> {
    return <- _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by(_).take(3).to_array())
}

[export, marker(no_coverage)]
def target_order_by_descending_take_splices_fold() : array<int> {
    return <- _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by_descending(_).take(3).to_array())
}

[export, marker(no_coverage)]
def target_bare_order_by_splices_fold() : array<int> {
    return <- _fold(each([10, 20, 5, 8, 30])._order_by(_).to_array())
}

[export, marker(no_coverage)]
def target_where_order_by_splices_fold() : array<int> {
    return <- _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._where(_ > 5)._order_by(_).to_array())
}

[export, marker(no_coverage)]
def target_where_order_by_take_splices_fold() : array<int> {
    return <- _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._where(_ > 5)._order_by(_).take(2).to_array())
}

struct private MkRec {
    a : int
    b : int
}

[export, marker(no_coverage)]
def target_where_order_by_keys_splices_fold() : array<MkRec> {
    // where + multi-key order_by_keys (a asc, b desc) over pure field keys → fused buffer + one composite stable_sort
    return <- _fold(each([MkRec(a = 2, b = 1), MkRec(a = 1, b = 2), MkRec(a = 1, b = 1), MkRec(a = 3, b = 0)])
        ._where(_.a >= 0)._order_by_keys((_.a, _.b), 2u).to_array())
}

[test]
def test_order_by_take_emits_top_n_by_with_cmp(t : T?) {
    // `order_by(key) |> take(K)` with an inlineable key now splices to the bounded-heap
    // walk: `spliced_push_heap` fill+replace pair + a `_::less(it, buf[0])` replace-test on
    // the heap top, `order_inplace(buf, cmp)` at the end. No top_n_by/top_n_by_with_cmp.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_by_take_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(count_call(body_expr, "spliced_push_heap"), 2, "bounded-heap fill + replace each call spliced_push_heap")
        t |> equal(count_call(body_expr, "spliced_pop_heap"), 1, "bounded-heap replace pops first")
        t |> success(count_call(body_expr, "order_inplace") >= 1, "final order_inplace on the bounded buf")
        t |> equal(0, count_call(body_expr, "top_n_by_with_cmp"), "bounded-heap supersedes top_n_by_with_cmp")
        t |> equal(0, count_call(body_expr, "top_n_by"), "should not emit non-cmp top_n_by")
        t |> equal(0, count_call(body_expr, "order_by"), "should not emit order_by")
        t |> equal(0, count_call(body_expr, "take"), "should not emit take")
    }
}

[test]
def test_order_by_descending_take_emits_top_n_by_with_cmp(t : T?) {
    // `order_by_descending(key) |> take(K)` with an inlineable key hits the same bounded-heap
    // entry as ascending — direction is encoded by flipping the inline less-test's operand
    // order (`_::less(buf[0], it)`).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(),
            @@target_order_by_descending_take_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(count_call(body_expr, "spliced_push_heap"), 2, "bounded-heap fill + replace each call spliced_push_heap (descending shares the same path)")
        t |> equal(count_call(body_expr, "spliced_pop_heap"), 1, "bounded-heap replace pops first")
        t |> equal(0, count_call(body_expr, "top_n_by_with_cmp"), "bounded-heap supersedes top_n_by_with_cmp")
        t |> equal(0, count_call(body_expr, "top_n_by_descending"),
            "should not emit the key-taking top_n_by_descending")
        t |> equal(0, count_call(body_expr, "order_by_descending"),
            "should not emit order_by_descending")
    }
}

[test]
def test_bare_order_by_emits_direct_call(t : T?) {
    // Bare `order_by(key)` with an inlineable key dispatches to the asc `order(src, cmp)`
    // block-taking overload — the key body is spliced into the comparator twice.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_bare_order_by_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(count_call(body_expr, "order") >= 1, "should emit an order(src, cmp) call")
        t |> equal(0, count_call(body_expr, "order_by"), "should not emit the key-taking order_by")
        t |> equal(0, count_call(body_expr, "top_n_by"), "should not emit top_n_by")
    }
}

[test]
def test_where_order_by_emits_fused_loop_with_inline_cmp(t : T?) {
    // `where |> order_by(key)` splices into a fused prefilter loop + order_inplace(buf, cmp)
    // when the key is inlineable — the comparator embeds the key body twice.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_order_by_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for fused emission")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused prefilter loop")
        t |> success(count_call(body_expr, "order_inplace") >= 1, "should call order_inplace(buf, cmp)")
        t |> equal(0, count_call(body_expr, "order_by_inplace"), "should not emit the key-taking order_by_inplace")
        t |> success(count_call(body_expr, "push_clone") >= 1, "should push_clone into the buffer")
    }
}

[test]
def test_where_order_by_take_emits_fused_top_n_with_cmp(t : T?) {
    // `where |> order_by(key) |> take(K)` with an inlineable key splices into the bounded-heap
    // walk wrapped in `if (whereCond)`. Single fused loop — no prefilter buffer, the heap IS
    // the buffer, and at most K elements ever live in it.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(),
            @@target_where_order_by_take_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for fused emission")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused walk over source")
        t |> equal(count_call(body_expr, "spliced_push_heap"), 2, "bounded-heap fill + replace each call spliced_push_heap")
        t |> equal(count_call(body_expr, "spliced_pop_heap"), 1, "bounded-heap replace pops first")
        t |> success(count_call(body_expr, "order_inplace") >= 1, "final order_inplace on the bounded buf")
        t |> equal(0, count_call(body_expr, "top_n_by_with_cmp"), "bounded-heap supersedes top_n_by_with_cmp")
        t |> equal(0, count_call(body_expr, "top_n_by"), "should not emit non-cmp top_n_by")
        t |> success(count_call(body_expr, "push_clone") >= 1, "should push_clone into the heap during fill")
    }
}

[test]
def test_where_order_by_keys_emits_stable_sort(t : T?) {
    // where + multi-key order_by_keys with an inlineable tuple key → fused prefilter loop + a SINGLE
    // composite stable_sort(buf, cmp). It must NOT use the unstable single-key order_inplace path, and
    // an inlineable key must splice (no cascade to the eager order_by_keys_inplace).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_order_by_keys_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for fused emission")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused prefilter loop")
        t |> equal(1, count_call(body_expr, "stable_sort"), "exactly one composite stable_sort on the fused buffer")
        t |> equal(0, count_call(body_expr, "order_inplace"), "multi-key must NOT use the unstable order_inplace")
        t |> equal(0, count_call(body_expr, "order_by_keys_inplace"), "inlineable key splices — no eager cascade")
        t |> success(count_call(body_expr, "push_clone") >= 1, "should push_clone into the buffer")
    }
}

[test]
def test_single_key_order_by_stays_unstable(t : T?) {
    // Regression guard: single-key order_by keeps the unstable sort/order_inplace path. The multi-key
    // stable_sort change must NOT leak into single-key orderby (no perf regression on the common case).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_order_by_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(count_call(body_expr, "order_inplace") >= 1, "single-key uses order_inplace")
        t |> equal(0, count_call(body_expr, "stable_sort"), "single-key must NOT emit stable_sort")
    }
}

// ── Phase 3 — runtime parity for order-family splice arms ──────────────

[test]
def test_order_by_take_correct_result(t : T?) {
    t |> run("order_by + take splice produces N smallest ascending") @(t : T?) {
        let got <- target_order_by_take_splices_fold()
        let expected = [2, 5, 8]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
}

[test]
def test_order_by_descending_take_correct_result(t : T?) {
    t |> run("order_by_descending + take splice produces N largest descending") @(t : T?) {
        let got <- target_order_by_descending_take_splices_fold()
        let expected = [30, 25, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
}

[test]
def test_where_order_by_take_correct_result(t : T?) {
    t |> run("where + order_by + take splice prefilters then takes top N") @(t : T?) {
        let got <- target_where_order_by_take_splices_fold()
        // src = [10, 20, 5, 8, 30, 15, 2, 25], where _>5 → [10, 20, 8, 30, 15, 25]
        // order_by(_).take(2) → [8, 10]
        let expected = [8, 10]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
}

// ── Phase 3d — select + where splice via replaceVariablePeeling ────────

[export, marker(no_coverage)]
def target_select_where_count_splices_fold() : int {
    return _fold(each([1, 2, 3, 4, 5, 6, 7, 8])._select(_ * 2)._where(_ > 5).count())
}

[export, marker(no_coverage)]
def target_select_where_sum_splices_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._select(_ * 10)._where(_ > 15).sum())
}

[export, marker(no_coverage)]
def target_select_where_any_splices_fold() : bool {
    return _fold(each([1, 2, 3])._select(_ * 100)._where(_ > 50).any())
}

[export, marker(no_coverage)]
def target_select_where_to_array_splices_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5])._select(_ * 2)._where(_ > 4).to_array())
}

[test]
def test_select_where_count_emits_fused_loop(t : T?) {
    // select |> where |> count: peel-substitution inlines projection into pred; loop emits
    // counter increments. NOT tier 2 (no pass_0/pass_1 binding shape).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(),
            @@target_select_where_count_splices_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop (not pass_0/pass_1 chain)")
        // tier 2 fold_linq_default would emit `var pass_0 = ...; var pass_1 = ...`.
        // tier 1 splice emits just the loop body + accumulator.
        t |> equal(0, count_call(body_expr, "where_"), "where_ should be inlined, not called")
        t |> equal(0, count_call(body_expr, "select"), "select should be inlined, not called")
    }
}

[test]
def test_select_where_count_correct_result(t : T?) {
    t |> run("select(x*2) |> where(>5) |> count") @(t : T?) {
        // src = [1..8], projected = [2,4,6,8,10,12,14,16], filtered >5 = [6,8,10,12,14,16], count = 6
        t |> equal(6, target_select_where_count_splices_fold())
    }
}

[test]
def test_select_where_sum_correct_result(t : T?) {
    t |> run("select(x*10) |> where(>15) |> sum") @(t : T?) {
        // src = [1..5], projected = [10,20,30,40,50], filtered >15 = [20,30,40,50], sum = 140
        t |> equal(140, target_select_where_sum_splices_fold())
    }
}

[test]
def test_select_where_any_correct_result(t : T?) {
    t |> run("select(x*100) |> where(>50) |> any") @(t : T?) {
        // src = [1,2,3], projected = [100,200,300], filtered >50 = [100,200,300], any = true
        t |> equal(true, target_select_where_any_splices_fold())
    }
}

[test]
def test_select_where_to_array_correct_result(t : T?) {
    t |> run("select(x*2) |> where(>4) |> to_array") @(t : T?) {
        // src = [1..5], projected = [2,4,6,8,10], filtered >4 = [6,8,10]
        let got <- target_select_where_to_array_splices_fold()
        let expected = [6, 8, 10]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
}

// ── Gap 2 — select + where projection dedup (single-eval splice) ───────

[export, marker(no_coverage)]
def target_select_where_sum_dedup() : int {
    // Pure projection with a distinctive constant (7919, prime). After the splice the
    // constant appears EXACTLY ONCE in the body — in the projection-bind statement.
    // Without the dedup the constant would appear twice: once inlined into the predicate
    // via peel-substitution, and once cloned into the sum's valueExpr.
    return _fold(each([1, 2, 3, 4, 5])._select(_ * 7919)._where(_ > 0).sum())
}

[test]
def test_select_where_projection_dedup(t : T?) {
    // Single-eval splice: the projection constant 7919 must appear exactly once in the
    // body — proves the projection is bound to a local once per element rather than
    // inlined twice (once in predicate, once in valueExpr).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_where_sum_dedup)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(1, count_const_int(body_expr, 7919),
            "projection constant should appear exactly once (single-eval dedup)")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "select"), "select should be inlined")
        t |> equal(0, count_call(body_expr, "where_"), "where_ should be inlined")
    }
}

[test]
def test_select_where_dedup_correct_result(t : T?) {
    t |> run("select+where dedup preserves arithmetic") @(t : T?) {
        // [1,2,3,4,5] * 7919 = [7919, 15838, 23757, 31676, 39595], all > 0,
        // sum = 7919 * (1+2+3+4+5) = 7919 * 15 = 118785
        t |> equal(118785, target_select_where_sum_dedup())
    }
}

[export, marker(no_coverage)]
def target_select_where_impure_falls_to_tier2() : int {
    // Impure projection (side-effecting function call) must NOT be moved outside the
    // where if-wrap — that would visibly fire side effects on filter-rejected elements.
    // The splice arm bails to tier 2 cascade in this case.
    return _fold(each([1, 2, 3, 4, 5])._select(side_effect_select_proj(_))._where(_ > 5).count())
}

[test]
def test_select_where_impure_correctness(t : T?) {
    t |> run("impure projection in select+where preserves per-element call count") @(t : T?) {
        // Plain LINQ semantics: select runs the projection for EVERY element before the
        // where filter sees the result. 5 input elements → 5 projection invocations,
        // regardless of how many survive the filter.
        g_select_count_proj_hits = 0
        let n = target_select_where_impure_falls_to_tier2()
        // projected: [2,4,6,8,10]; filtered >5: [6,8,10]; count = 3
        t |> equal(3, n)
        t |> equal(5, g_select_count_proj_hits, "projection must fire once per source element")
    }
}

[export, marker(no_coverage)]
def target_select_distinct_impure() : array<int> {
    // Side-effecting projection before distinct. Splice MUST bind the projected value once
    // per source element (and use the bind for both the seen-key and the buffer push) so
    // the projection fires once per element — matching plain LINQ's iterator-pipeline behavior.
    return <- _fold([1, 2, 3, 4, 5].to_sequence()._select(side_effect_select_proj(_)).distinct().to_array())
}

[test]
def test_select_distinct_impure_correctness(t : T?) {
    t |> run("impure projection in select+distinct fires once per source element") @(t : T?) {
        // Naive emission evaluates `side_effect_select_proj(it)` twice per element (once for
        // the unique_key lookup, once for push_clone into the buffer) → counter would hit
        // 5 * 2 = 10. The per-element bind fix keeps it at 5.
        g_select_count_proj_hits = 0
        let out <- target_select_distinct_impure()
        t |> equal(5, length(out))
        t |> equal(5, g_select_count_proj_hits, "projection must fire once per source element (not twice)")
    }
}

// ── Gap 1 — inlineable key fallback when key is side-effecting ───────────

var g_order_key_hits = 0

def side_effect_order_key(x : int) : int {
    g_order_key_hits ++
    return -x
}

[export, marker(no_coverage)]
def target_order_by_take_side_effecting_key() : array<int> {
    // Side-effecting key — try_make_inline_cmp's has_sideeffects gate must bail; the
    // splice falls back to the keyed top_n_by entry point (not _with_cmp).
    return <- _fold(each([3, 1, 4, 1, 5, 9, 2, 6])._order_by(side_effect_order_key(_)).take(3).to_array())
}

[test]
def test_order_by_take_side_effecting_key_falls_back(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(),
            @@target_order_by_take_side_effecting_key)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> success(count_call(body_expr, "top_n_by") >= 1,
            "side-effecting key should fall back to keyed top_n_by")
        t |> equal(0, count_call(body_expr, "top_n_by_with_cmp"),
            "side-effecting key must NOT use the inline-cmp variant")
    }
}

[test]
def test_order_by_take_side_effecting_key_correct_result(t : T?) {
    t |> run("side-effecting order_by key produces correct top-N by -key") @(t : T?) {
        let got <- target_order_by_take_side_effecting_key()
        // -key sorts ascending by -x ⇒ descending by x; take 3 largest.
        let expected = [9, 6, 5]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
}

// ── Phase 3+ — buffer-required splice arms (plan_reverse) ──────────────

[export, marker(no_coverage)]
def target_reverse_to_array_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5]) |> reverse() |> to_array())
}

[test]
def test_reverse_to_array_emits_single_loop_plus_inplace(t : T?) {
    // R1: bare reverse + to_array splices into a single fused for-loop that pushes the
    // source into a buffer, then calls reverse_inplace. Anti-cascade gate: tier-2 cascade
    // would emit `var pass_0 = ...; delete pass_0; var pass_1 = ...` with multiple outer
    // vars, so the single-buffer assertion proves splice fired.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_call(body_expr, "reverse_inplace") >= 1,
            "must emit reverse_inplace call on the buffer")
        t |> equal(0, count_call(body_expr, "reverse"),
            "reverse runtime helper must NOT be called (splice elides it)")
        t |> equal(1, count_outer_let_vars(body_expr),
            "single outer var (the buffer) — anti-cascade gate")
    }
}

[export, marker(no_coverage)]
def target_where_reverse_to_array_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> to_array())
}

[test]
def test_reverse_with_where_fuses_into_loop(t : T?) {
    // R2: where_ before reverse fuses into the prefilter loop. Buffer accumulates only
    // matching elements; reverse_inplace then mutates that buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_reverse_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_call(body_expr, "reverse_inplace") >= 1,
            "must emit reverse_inplace call")
        t |> equal(0, count_call(body_expr, "where_"),
            "where_ runtime helper must NOT be called (predicate inlined)")
        t |> equal(1, count_outer_let_vars(body_expr),
            "single outer var (the buffer) — anti-cascade gate")
    }
}

[export, marker(no_coverage)]
def target_select_reverse_to_array_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5])._select(_ * 10) |> reverse() |> to_array())
}

[export, marker(no_coverage)]
def target_reverse_take_to_array_fold() : array<int> {
    return <- _fold(each([1, 2, 3, 4, 5, 6, 7, 8]) |> reverse() |> take(3) |> to_array())
}

[test]
def test_reverse_take_emits_backward_index_loop(t : T?) {
    // R6: array-typed source + reverse |> take(N) — backward index loop instead of
    // (push all → reverse_inplace → resize). Visits only the last N indices, skipping
    // the O(length) push and O(length) reverse pass. Closes the m3 lazy-yield regression.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_take_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "reverse_inplace"),
            "must NOT emit reverse_inplace (backward index loop replaces it)")
        t |> equal(0, count_call(body_expr, "resize"),
            "must NOT emit resize (reserve(takeN) replaces it)")
        t |> equal(0, count_call(body_expr, "take"),
            "take runtime helper must NOT be called")
        t |> success(count_call(body_expr, "reserve") >= 1,
            "must emit reserve(takeN) to pre-size the buffer")
        t |> success(count_call(body_expr, "length") >= 1,
            "must call length(src) to bound the backward walk")
        t |> equal(3, count_outer_let_vars(body_expr),
            "three outer vars: source-length, clamped-takeN, buffer")
    }
}

[export, marker(no_coverage)]
def target_reverse_take_iterator_fold() : array<int> {
    // Iterator source (range) — backward index loop is NOT applicable, falls back
    // to R1-R4 path (push all → reverse_inplace → resize).
    return <- _fold(each(range(1, 9)) |> reverse() |> take(3) |> to_array())
}

[test]
def test_reverse_take_iterator_source_uses_reverse_inplace(t : T?) {
    // R6 negative: when source is not indexable (range / iterator), the R1-R4 fallback
    // path stays in place. Pins the full fallback shape (not just reverse_inplace/resize
    // presence) so the test catches both directions of drift: (a) R6 starts mis-firing on
    // iterator sources, (b) the R1-R4 emission shape changes (e.g. cascades to take helper).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_take_iterator_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused prefilter loop (R1-R4)")
        t |> success(count_call(body_expr, "reverse_inplace") >= 1,
            "iterator source must use reverse_inplace fallback")
        t |> success(count_call(body_expr, "resize") >= 1,
            "iterator source must use resize fallback")
        t |> equal(0, count_call(body_expr, "take"),
            "take runtime helper must NOT be called (resize replaces it)")
        t |> equal(1, count_outer_let_vars(body_expr),
            "single outer var (the buffer) — anti-cascade gate")
    }
}

[export, marker(no_coverage)]
def target_reverse_count_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> count())
}

[test]
def test_reverse_count_elides_buffer_and_reverse(t : T?) {
    // R5: reverse |> count — reverse is identity for count. No buffer, no reverse_inplace,
    // just a counter loop over surviving elements.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "reverse_inplace"),
            "reverse_inplace must NOT be emitted — reverse is identity for count")
        t |> equal(0, count_call(body_expr, "push_clone"),
            "no buffer pushes — count elides the buffer entirely")
        t |> success(count_op1(body_expr, "++") >= 1, "counter increment must be present")
    }
}

[export, marker(no_coverage)]
def target_reverse_first_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> first())
}

[test]
def test_reverse_first_uses_last_survivor_tracker(t : T?) {
    // R5: reverse |> first = LAST survivor in source order. Walk forward, overwrite
    // `last` on every match, return `last` (panic if no matches). No buffer, no reverse.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_first_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "reverse_inplace"),
            "reverse_inplace must NOT be emitted — last-survivor walk replaces it")
        t |> equal(0, count_call(body_expr, "push_clone"),
            "no buffer pushes — first elides the buffer entirely")
        t |> success(count_call(body_expr, "panic") >= 1,
            "first must emit panic for the empty-sequence case")
    }
}

[export, marker(no_coverage)]
def target_reverse_first_or_default_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 100) |> reverse() |> first_or_default(-1))
}

[test]
def test_reverse_first_or_default_returns_default_when_empty(t : T?) {
    // R5: reverse |> first_or_default returns the default expression when no element
    // survives the filter. No panic.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_reverse_first_or_default_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "reverse_inplace"),
            "reverse_inplace must NOT be emitted")
        t |> equal(0, count_call(body_expr, "panic"),
            "first_or_default must NOT panic")
    }
}

// ── Phase 3+ — plan_distinct stages D2-D7 ──────────────────────────────

[export, marker(no_coverage)]
def target_where_distinct_to_array_fold() : array<int> {
    return <- _fold([1, 2, 3, 2, 4, 1, 5].to_sequence()._where(_ > 1).distinct().to_array())
}

[test]
def test_distinct_with_where_fuses_into_loop(t : T?) {
    // D2: where_ before distinct fuses into the prefilter loop. Single for, single seen-set + buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_distinct_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "where_"), "where_ runtime helper must NOT be called (predicate inlined)")
        t |> equal(0, count_call(body_expr, "distinct"), "distinct runtime helper must NOT be called")
        t |> equal(2, count_outer_let_vars(body_expr), "exactly seen-set + buffer")
    }
}

[export, marker(no_coverage)]
def target_select_distinct_to_array_fold() : array<int> {
    return <- _fold([1, 2, 3, 4].to_sequence()._select(_ % 3).distinct().to_array())
}

[test]
def test_distinct_select_uses_projection_as_key(t : T?) {
    // D3: select before distinct — projection inlined into both unique_key and push_clone.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_distinct_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "select"), "select runtime helper must NOT be called")
        t |> equal(0, count_call(body_expr, "distinct"), "distinct runtime helper must NOT be called")
        t |> success(count_op2(body_expr, "%") >= 1, "projection modulo must appear inline in body")
    }
}

[export, marker(no_coverage)]
def target_distinct_by_to_array_fold() : array<int> {
    return <- _fold([10, 21, 32, 11, 23, 42].to_sequence()._distinct_by(_ % 10).to_array())
}

[test]
def test_distinct_by_with_key_substitutes_keyfn(t : T?) {
    // D4: distinct_by — key block invoked per element; key drives dedup but the raw
    // (or projected) value is what gets pushed to the buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_distinct_by_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> equal(0, count_call(body_expr, "distinct_by"), "distinct_by runtime helper must NOT be called")
        t |> success(count_op2(body_expr, "%") >= 1, "key modulo operation must appear inline (via let init walk)")
        t |> equal(2, count_outer_let_vars(body_expr), "exactly seen-set + buffer")
    }
}

[export, marker(no_coverage)]
def target_distinct_take_to_array_fold() : array<int> {
    return <- _fold([1, 1, 2, 2, 3, 3, 4, 4, 5, 5].to_sequence().distinct().take(3).to_array())
}

[test]
def test_distinct_take_emits_early_exit(t : T?) {
    // D5: take(N) on distinct early-exits the prefilter loop after N distinct hits.
    // Anti-cascade gate: tier-2 cascade would emit multiple outer vars + delete stmts.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_distinct_take_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_break_continue(body_expr, true) >= 1, "take guard must emit a break")
        t |> equal(0, count_call(body_expr, "take"), "take runtime helper must NOT be called")
    }
}

[export, marker(no_coverage)]
def target_distinct_count_fold() : int {
    return _fold([1, 2, 2, 3, 3, 3].to_sequence().distinct().count())
}

[test]
def test_distinct_count_elides_buffer(t : T?) {
    // D6: distinct → count returns length(seen). No buffer, no push_clone — the
    // seen-set IS the answer source.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_distinct_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "push_clone"),
            "no buffer pushes — count elides the buffer entirely")
        t |> equal(0, count_call(body_expr, "distinct"), "distinct runtime helper must NOT be called")
    }
}

[export, marker(no_coverage)]
def target_distinct_sum_fold() : int {
    return _fold([1, 2, 2, 3, 3, 3, 4].to_sequence().distinct().sum())
}

[test]
def test_distinct_sum_emits_inline_accumulator(t : T?) {
    // D7: distinct → sum maintains seen-set + accumulator; each fresh element contributes
    // to acc via `+=`. No buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_distinct_sum_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> success(count_op2(body_expr, "+=") >= 1, "sum accumulator must emit +=")
        t |> equal(0, count_call(body_expr, "push_clone"),
            "no buffer pushes — sum folds inline")
    }
}

// ── Phase 3+ — plan_group_by stages G2-G3 (count reducer + named-tuple wrap) ───

[export, marker(no_coverage)]
def target_group_by_count_bare_reducer_fold() : array<int> {
    // Bare reducer body — group_proj is just `_._1 |> length`, no named-tuple wrap.
    // Result is array<int> of per-bucket counts. Bucket count is 3 (mod 3 = 0,1,2).
    // _group_by on array → array<tuple<K; array<V>>>; _select then yields array<int>;
    // no trailing .to_array() since the chain already produces an array.
    return <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select(_._1 |> length))
}

[test]
def test_group_by_count_emits_table_no_bucket_arrays(t : T?) {
    // G2: group_by(key) + select(_._1|>length) + to_array splices into a table-driven
    // emission with NO per-bucket array materialization. Table value is `tuple<KeyT; int>`
    // (Acc is int for count) rather than `tuple<KeyT; array<elem>>`.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_count_bare_reducer_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        // No call to the runtime group_by_lazy / group_by_lazy_impl helpers; the
        // splice replaces them with a per-element table update.
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "group_by_lazy_impl"), "must NOT call impl")
        // unique_key must be invoked per element to derive the seen-table key.
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        // Single-hash hot path: key_exists must NOT appear (the `tab?[uk] ?? dummy` +
        // addr-compare pattern avoids the extra membership check). See PR #2721.
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
        // For-loop count: table-update loop + result-build loop = 2.
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_count_named_tuple_fold() : array<tuple<K : int; N : int>> {
    // Named-tuple wrap: select(@(g) => (K=g._0, N=g._1|>length)) preserves both the key
    // AND the aggregate. Splice uses the user's group_proj body type as the table value
    // type, so the K/N field names survive end-to-end (see test below).
    return <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)))
}

[test]
def test_group_by_count_named_tuple_preserves_keys(t : T?) {
    // G3: named-tuple wrap recognized; output type preserves user shape.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_count_named_tuple_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        // For-loops: table-update + result-build = 2 (incremental-aggregate's tell).
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
        // unique_key per element — splice's per-element hash. (Note: not asserting
        // absence of `length` here — `length(tab)` legitimately appears in the
        // reserve hint and would false-positive.)
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        // Single-hash hot path: no key_exists. See PR #2721.
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_count_terminator_fold() : int {
    // G8: count terminator — total number of buckets. Should return length(tab) directly.
    return _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)).count())
}

[test]
def test_group_by_count_terminator_returns_table_length(t : T?) {
    // G8: count terminator on group_by + select — return length(tab) without
    // materializing the result buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_count_terminator_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy")
        // Only ONE for-loop (the table-update). Result-build loop is elided when
        // terminator is count (we just return length(tab)).
        t |> equal(1, count_inner_for_loops(body_expr), "single table-update loop, no result loop")
        // Single-hash hot path: no key_exists. See PR #2721.
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_inner_select_sum_bare_fold() : array<int> {
    // G4 (PR-A1): bare inner-select-sum body. is_bucket_reducer_call's new arm matches
    // `sum(select(<bind>._1, <lambda>))`; the inner lambda body is peeled by fold_linq_cond
    // and inlined into the per-element `entry._1 += <body>` site.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> select(@(x : int) => x * 2) |> sum))
}

[test]
def test_group_by_inner_select_sum_emits_table_no_inner_select_call(t : T?) {
    // G4: bare inner-select-sum splices through the new arm. Body must NOT contain
    // a runtime call to `select` (the inner select is peeled + inlined) and must
    // preserve the single-hash hot-path fingerprint from PR #2721.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_inner_select_sum_bare_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "select"), "inner select must be inlined, not called")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_inner_select_sum_named_fold() : array<tuple<K : int; T : int>> {
    // G4b: named-tuple-wrap form `(K=_._0, T=_._1|>select(<inner>)|>sum)`. Splice fires
    // via Case B (named-tuple recognizer); table value type = user's named tuple,
    // accumulator slot = inner projection result type.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, T = _._1 |> select(@(x : int) => x * 2) |> sum)))
}

[test]
def test_group_by_inner_select_sum_named_preserves_key(t : T?) {
    // G4b: named-tuple wrap with inner-select-sum splices and keeps the user's field names.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_inner_select_sum_named_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "select"), "inner select must be inlined, not called")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_min_workhorse_fold() : array<int> {
    // G5a (PR-A2): bare min on workhorse int → direct `<` compare, no `_::less`.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> min))
}

[test]
def test_group_by_min_workhorse_emits_direct_compare(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_min_workhorse_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "less"), "workhorse min must use direct `<`, not _::less")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_min_non_workhorse_fold() : array<tuple<int; int>> {
    // G5b (PR-A2): bare min on `tuple<int;int>` (non-workhorse) → `_::less` dispatch.
    return <- _fold([(1, 2), (0, 5), (2, 3), (0, 1), (1, 4)]._group_by(_._0 % 2)._select(_._1 |> min))
}

[test]
def test_group_by_min_non_workhorse_uses_less(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_min_non_workhorse_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> success(count_call(body_expr, "less") >= 1, "non-workhorse min: at least one _::less call")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_first_fold() : array<int> {
    // G6 (PR-A2): bare first → miss-branch captures, hit-branch is empty (no compare).
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> first))
}

[test]
def test_group_by_first_no_hit_compare(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_first_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "less"), "first hit branch is empty: no _::less call")
        t |> equal(0, count_call(body_expr, "first"), "must NOT call runtime first()")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_multi_reducer_fold() : array<tuple<K : int; N : int; S : int; MX : int>> {
    // G7 (PR-A2): 4-slot named tuple → multi-reducer specs array fuses count+sum+max into one pass.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
        (K = _._0, N = _._1 |> length, S = _._1 |> sum, MX = _._1 |> max)))
}

[test]
def test_group_by_multi_reducer_emits_fused_pass(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_multi_reducer_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "sum"), "must NOT call runtime sum()")
        t |> equal(0, count_call(body_expr, "max"), "must NOT call runtime max()")
        // Note: 1 `length(tab)` survives for reserve() — that's not the reducer.
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(2, count_inner_for_loops(body_expr), "single fused table-update loop + result-build")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
    }
}

[export, marker(no_coverage)]
def target_group_by_first_or_default_falls_through() : array<int> {
    // G10 (PR-A2): `first_or_default(_._1, def)` has 2 args → recognizer rejects (only 1-arg
    // bucket reducers are spliced). Cascades to tier 2.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> first_or_default(-1)))
}

[test]
def test_group_by_first_or_default_cascades_to_tier2(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_first_or_default_falls_through)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        // Cascade emits pass_0/pass_1/... — splice would have exactly 1 outer let (the table).
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_upstream_where_fold() : array<int> {
    // G11 (PR-B): upstream where_ fuses into the per-element loop. No runtime where_ call;
    // body contains `if (predicate) { ... }` guarding the key + table-update splice.
    return <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ % 2 == 0)._group_by(_ % 3)._select(_._1 |> length))
}

[test]
def test_group_by_upstream_where_no_runtime_where_call(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_upstream_where_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "where_"), "upstream where_ must be inlined, not called")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_upstream_select_fold() : array<tuple<K : int; S : int>> {
    // G12 (PR-B): upstream select_ fuses; the key references the projected value via an
    // intermediate var bind inside the per-element loop.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._select(_ * 2)._group_by(_ % 3)._select((K = _._0, S = _._1 |> sum)))
}

[test]
def test_group_by_upstream_select_no_runtime_select_call(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_upstream_select_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "select"), "upstream select must be inlined, not called")
        t |> success(count_call(body_expr, "unique_key") >= 1, "must call _::unique_key per element")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_upstream_where_select_fold() : array<tuple<K : int; S : int>> {
    // G13 (PR-B): chained upstream where_ + select_ fuse together. Where guards, select binds.
    return <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ > 3)._select(_ * 10)._group_by(_ % 3)._select((K = _._0, S = _._1 |> sum)))
}

[test]
def test_group_by_upstream_where_select_combined(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_upstream_where_select_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy runtime")
        t |> equal(0, count_call(body_expr, "where_"), "upstream where_ must be inlined")
        t |> equal(0, count_call(body_expr, "select"), "upstream select must be inlined")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path: no key_exists")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_upstream_distinct_falls_through() : array<int> {
    // BAIL (PR-B): distinct upstream is NOT where_/select* → plan_group_by rejects; cascades to tier 2.
    return <- _fold([1, 2, 3, 4, 5, 6, 7].to_sequence().distinct()._group_by(_ % 3)._select(_._1 |> length).to_array())
}

[test]
def test_group_by_upstream_distinct_cascades_to_tier2(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_upstream_distinct_falls_through)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_having_falls_through() : array<int> {
    // BAIL: where_ between group_by and select (HAVING clause shape) → cascade to tier 2.
    return <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._where(_._1 |> length >= 2)._select(_._1 |> length))
}

[test]
def test_group_by_having_cascades_to_tier2(t : T?) {
    // Bail-trigger: where_ between group_by and select — plan_group_by must reject;
    // chain cascades to tier 2 (which emits the array-shape pipeline with pass_N
    // vars + delete intermediates).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_falls_through)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        // Tier 2 cascade emits pass_0/pass_1/... let-vars. Splice would have exactly 1
        // (the table). So having_ → tier 2 produces 3+ outer let-vars (where, group, select).
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_having_length_match_fold() : array<tuple<K : int; N : int>> {
    // PR-D splice: `_having(_._1|>length>=N)` between group_by_lazy and select where the select
    // has a matching length slot — predicate rewrites to `kv._{slot}>=N` and wraps push_clone.
    return <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)._select((K = _._0, N = _._1 |> length)))
}

[test]
def test_group_by_having_length_match_splices(t : T?) {
    // Single-hash hot path (no key_exists) + no runtime having_/select call in the splice body.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_length_match_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "expected splice wrapper")
        t |> equal(0, count_call(body_expr, "key_exists"))
        t |> success(count_call(body_expr, "unique_key") >= 1, "table-update uses unique_key")
        t |> equal(0, count_call(body_expr, "having_"))
        t |> equal(0, count_call(body_expr, "group_by_lazy"))
    }
}

[export, marker(no_coverage)]
def target_group_by_having_count_terminator_fold() : int {
    // PR-D count terminator: result-build counts only the buckets passing the rewritten predicate.
    return _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)._select((K = _._0, N = _._1 |> length)) |> count())
}

[test]
def test_group_by_having_count_terminator_splices(t : T?) {
    // count + having: no length(tab) shortcut (we have to inspect each entry); the splice emits
    // a counter loop instead — `having_` and `group_by_lazy` should still be absent.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_count_terminator_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "expected splice wrapper")
        t |> equal(0, count_call(body_expr, "having_"))
        t |> equal(0, count_call(body_expr, "group_by_lazy"))
        t |> equal(0, count_call(body_expr, "key_exists"))
    }
}

[export, marker(no_coverage)]
def target_group_by_having_hidden_sum_fold() : array<tuple<K : int; N : int>> {
    // PR-E: having uses sum but select has only length — planner synthesizes a hidden sum slot
    // in the internal table value type; result-build projects only the user-visible (K, N).
    return <- _fold([1, 2, 3, 4, 5, 6]._group_by_lazy(_ % 2)._having(_._1 |> sum > 10)._select((K = _._0, N = _._1 |> length)))
}

[test]
def test_group_by_having_hidden_sum_splices(t : T?) {
    // PR-E splice fingerprint: no group_by_lazy or having_ runtime calls; single-hash hot path.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_hidden_sum_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "expected splice wrapper")
        t |> equal(0, count_call(body_expr, "group_by_lazy"))
        t |> equal(0, count_call(body_expr, "having_"))
        t |> equal(0, count_call(body_expr, "sum"), "hidden sum inlined into += hit branch")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path")
        t |> success(count_call(body_expr, "unique_key") >= 1, "per-element unique_key")
    }
}

[export, marker(no_coverage)]
def target_group_by_having_two_same_named_inner_select_cascades() : array<tuple<K : int; N : int>> {
    // PR-E bail: two same-named inner-select reducers in the same having clause can't be
    // distinguished by name alone — splicing would silently conflate the two lambdas onto
    // one hidden slot. Bail to tier 2 so each select() is evaluated separately.
    return <- _fold([1, 2, 3, 4, 5, 6, 7, 8]._group_by_lazy(_ % 3)
                       ._having((_._1 |> select($(x : int) => x * 2) |> sum)
                           + (_._1 |> select($(x : int) => x * 3) |> sum) > 50)
                       ._select((K = _._0, N = _._1 |> length)))
}

[test]
def test_group_by_having_two_same_named_inner_select_cascades_to_tier2(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_two_same_named_inner_select_cascades)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_having_hidden_bare_form_cascades() : array<int> {
    // PR-E bail: bare-form group_proj (`_._1 |> length`, not a named tuple) combined with a
    // having reducer that needs a hidden slot — the bare table layout uses `tuple<KeyT; AccT>`
    // synthesized inside a qmacro with embedded `typedecl(invoke(...))`, which can't be
    // extended with a dynamic count of additional acc slots. Falls back to tier 2.
    return <- _fold([1, 2, 3, 4, 5, 6]._group_by_lazy(_ % 2)._having(_._1 |> sum > 10)._select(_._1 |> length))
}

[test]
def test_group_by_having_hidden_bare_form_cascades_to_tier2(t : T?) {
    // Cascade fingerprint: multiple pass_N let-vars (group, having, select all separate).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_hidden_bare_form_cascades)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_having_raw_bucket_cascades() : array<tuple<K : int; N : int>> {
    // BAIL: predicate touches _._1 outside any matched reducer (raw bucket-array length probe).
    // is_bind_field_access on _._1 alone (used as condition value) is not in the recognized set.
    return <- _fold([1, 2, 3, 4, 5, 6]._group_by_lazy(_ % 2)._having(empty(_._1))._select((K = _._0, N = _._1 |> length)))
}

[test]
def test_group_by_having_raw_bucket_cascades_to_tier2(t : T?) {
    // Cascade fingerprint: multiple pass_N let-vars.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_having_raw_bucket_cascades)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper from cascade")
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "cascade fingerprint: multiple pass_N outer vars")
    }
}

[export, marker(no_coverage)]
def target_group_by_average_bare_fold() : array<double> {
    // PR-A2 follow-up: bare `_._1 |> average` splices via tuple<double;uint64> per-key acc.
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> average))
}

[test]
def test_group_by_average_bare_emits_avg_acc(t : T?) {
    // Average splice fingerprint: no runtime average call (inlined into += / count loop),
    // single-hash hot path (no key_exists), per-element unique_key.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_average_bare_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy")
        t |> equal(0, count_call(body_expr, "average"), "reducer inlined; no runtime average call")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path")
        t |> success(count_call(body_expr, "unique_key") >= 1, "per-element unique_key")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_average_named_fold() : array<tuple<K : int; Avg : double>> {
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Avg = _._1 |> average)))
}

[test]
def test_group_by_average_named_preserves_field_name(t : T?) {
    // Named form: table stores tuple<K:int;Avg:tuple<double;uint64>> internally; result-build
    // synthesizes user-facing tuple<K;Avg:double> with field names preserved (compiler-checked
    // by the function's declared return type — would fail compilation if shape diverged).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_average_named_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy")
        t |> equal(0, count_call(body_expr, "average"), "reducer inlined; no runtime average call")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[export, marker(no_coverage)]
def target_group_by_average_inner_select_fold() : array<tuple<K : int; Avg : double>> {
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
        (K = _._0, Avg = _._1 |> select(@(x : int) => x * 2) |> average)))
}

[test]
def test_group_by_average_inner_select_inlines_projection(t : T?) {
    // Inner select inlined — neither inner `select` call nor the reducer survives.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_average_inner_select_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy")
        t |> equal(0, count_call(body_expr, "average"), "reducer inlined")
        t |> equal(0, count_call(body_expr, "select"), "inner select inlined")
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path")
    }
}

[export, marker(no_coverage)]
def target_group_by_multi_reducer_with_avg_fold() : array<tuple<K : int; N : int; S : int; Avg : double>> {
    return <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
        (K = _._0, N = _._1 |> length, S = _._1 |> sum, Avg = _._1 |> average)))
}

[test]
def test_group_by_multi_reducer_with_avg_splices(t : T?) {
    // 4-slot named tuple combining length + sum + average — single fused pass, no runtime reducer calls.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_group_by_multi_reducer_with_avg_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(0, count_call(body_expr, "group_by_lazy"), "must NOT call group_by_lazy")
        t |> equal(0, count_call(body_expr, "average"), "reducer inlined")
        t |> equal(0, count_call(body_expr, "sum"), "reducer inlined")
        // length(tab) legitimately appears in the reserve hint — don't assert absence.
        t |> equal(0, count_call(body_expr, "key_exists"), "single-hash hot path")
        t |> equal(2, count_inner_for_loops(body_expr), "table-update + result-build loops")
    }
}

[test]
def test_reverse_select_pushes_projection_not_source(t : T?) {
    // R3: select before reverse — the loop pushes the projected value, not the
    // iteration variable. ExprMul / ExprOp2 "*" presence in body indicates the
    // projection got inlined into push_clone.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_reverse_to_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper for splice")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused for-loop")
        t |> success(count_call(body_expr, "reverse_inplace") >= 1,
            "must emit reverse_inplace call")
        t |> equal(0, count_call(body_expr, "select"),
            "select runtime helper must NOT be called (projection inlined)")
        t |> success(count_op2(body_expr, "*") >= 1,
            "projection multiplication must appear inline in push_clone")
        t |> equal(1, count_outer_let_vars(body_expr),
            "single outer var (the buffer) — anti-cascade gate")
    }
}

// ── Phase 3+ terminal-walk lane: last / single / element_at / aggregate ─────

[export, marker(no_coverage)]
def target_last_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ < 4).last())
}

[export, marker(no_coverage)]
def target_last_or_default_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 99).last_or_default(-1))
}

[export, marker(no_coverage)]
def target_single_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ == 3).single())
}

[export, marker(no_coverage)]
def target_single_or_default_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ > 99).single_or_default(-1))
}

[export, marker(no_coverage)]
def target_element_at_fold() : int {
    return _fold(each([10, 20, 30, 40, 50]).element_at(3))
}

[export, marker(no_coverage)]
def target_element_at_or_default_fold() : int {
    return _fold(each([10, 20, 30]).element_at_or_default(100))
}

[export, marker(no_coverage)]
def target_aggregate_fold() : int {
    return _fold(each([1, 2, 3, 4, 5]).aggregate(0, $(acc : int, x : int) => acc + x * 2))
}

[export, marker(no_coverage)]
def target_aggregate_with_where_fold() : int {
    return _fold(each([1, 2, 3, 4, 5])._where(_ % 2 == 1).aggregate(0, $(acc : int, x : int) => acc + x * x))
}

[test]
def test_last_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_last_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "last: one for-loop (full walk)")
        // `last` tail panics on empty source; verify the splice emitted it.
        t |> success(count_call(body_expr, "panic") >= 1, "last: panic call in tail for empty case")
    }
}

[test]
def test_last_or_default_no_panic(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_last_or_default_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "last_or_default: one for-loop")
        // `last_or_default` returns the bound default; never panics.
        t |> equal(0, count_call(body_expr, "panic"), "last_or_default: no panic call")
    }
}

[test]
def test_single_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_single_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "single: one for-loop")
        // `single` panics twice in the splice: once for >1 match (in-loop), once for empty (tail).
        t |> success(count_call(body_expr, "panic") >= 2, "single: two panic call sites")
    }
}

[test]
def test_single_or_default_no_panic(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_single_or_default_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "single_or_default: one for-loop")
        t |> equal(0, count_call(body_expr, "panic"), "single_or_default: no panic")
    }
}

[test]
def test_element_at_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_element_at_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "element_at: one for-loop")
        // `element_at` emits two panic call sites in the splice (pre-loop for `idx < 0`,
        // tail for "index out of range"); const-fold may eliminate the pre-loop branch
        // when the literal index is non-negative, so >= 1 is the guarantee.
        t |> success(count_call(body_expr, "panic") >= 1, "element_at: at least the tail panic survives")
        // Counter `++` increments the in-flight match counter.
        t |> success(count_op1(body_expr, "++") >= 1, "element_at: counter increment present")
    }
}

[test]
def test_element_at_or_default_no_panic(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_element_at_or_default_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "element_at_or_default: one for-loop")
        t |> equal(0, count_call(body_expr, "panic"), "element_at_or_default: no panic")
    }
}

// Count ExprInvoke nodes anywhere in the subtree. The outer `_fold` splice wrapper
// is an ExprInvoke; per-element block calls (when peel fails) also emit ExprInvoke.
// Peeled aggregate bodies inline the block expression and emit zero inner invokes.
def count_invoke_nodes(expr : Expression?) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprInvoke) {
        n ++
    }
    if (expr is ExprBlock) {
        let b = expr as ExprBlock
        for (s in b.list) {
            n += count_invoke_nodes(s)
        }
        for (s in b.finalList) {
            n += count_invoke_nodes(s)
        }
    } elif (expr is ExprFor) {
        let f = expr as ExprFor
        for (s in f.sources) {
            n += count_invoke_nodes(s)
        }
        n += count_invoke_nodes(f.body)
    } elif (expr is ExprIfThenElse) {
        let i = expr as ExprIfThenElse
        n += count_invoke_nodes(i.cond)
        n += count_invoke_nodes(i.if_true)
        n += count_invoke_nodes(i.if_false)
    } elif (expr is ExprOp2) {
        let o = expr as ExprOp2
        n += count_invoke_nodes(o.left)
        n += count_invoke_nodes(o.right)
    } elif (expr is ExprCall) {
        let c = expr as ExprCall
        for (a in c.arguments) {
            n += count_invoke_nodes(a)
        }
    } elif (expr is ExprMakeBlock) {
        let mb = expr as ExprMakeBlock
        n += count_invoke_nodes(mb._block)
    } elif (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        for (a in inv.arguments) {
            n += count_invoke_nodes(a)
        }
    } elif (expr is ExprReturn) {
        let r = expr as ExprReturn
        n += count_invoke_nodes(r.subexpr)
    } elif (expr is ExprOp1) {
        let o = expr as ExprOp1
        n += count_invoke_nodes(o.subexpr)
    } elif (expr is ExprLet) {
        let l = expr as ExprLet
        for (v in l.variables) {
            if (v != null && v.init != null) {
                n += count_invoke_nodes(v.init)
            }
        }
    }
    return n
}

[test]
def test_aggregate_splices_peeled(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_aggregate_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "aggregate: one for-loop")
        // Single-return block body `acc + x * 2` peeled into `+` and `*` ops; the only
        // ExprInvoke is the outer `_fold` splice wrapper. A non-peeled splice would emit
        // a per-element ExprInvoke against the user's block.
        t |> equal(1, count_invoke_nodes(body_expr),
            "aggregate: only the outer invoke wrapper, NOT a per-element block invoke")
        t |> success(count_op2(body_expr, "+") >= 1, "aggregate: `+` from peeled body")
        t |> success(count_op2(body_expr, "*") >= 1, "aggregate: `*` from peeled body")
    }
}

[test]
def test_aggregate_with_where_fuses(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_aggregate_with_where_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "aggregate+where: one for-loop")
        // Where predicate (`_ % 2 == 1`) inlined alongside peeled aggregate body. The
        // `where_` library helper must NOT appear as a runtime call.
        t |> equal(0, count_call(body_expr, "where_"), "aggregate+where: where_ helper inlined")
        t |> equal(1, count_invoke_nodes(body_expr), "aggregate+where: only outer invoke wrapper")
    }
}

// ── take_while / skip_while splice targets (PR-G) ──────────────────────

[export, marker(no_coverage)]
def target_take_while_array_fold() : array<int> {
    return <- _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).to_array())
}

[export, marker(no_coverage)]
def target_take_while_count_fold() : int {
    return _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).count())
}

[export, marker(no_coverage)]
def target_take_while_sum_fold() : int {
    return _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).sum())
}

[export, marker(no_coverage)]
def target_take_while_first_fold() : int {
    return _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).first())
}

[export, marker(no_coverage)]
def target_skip_while_array_fold() : array<int> {
    return <- _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3).to_array())
}

[export, marker(no_coverage)]
def target_skip_while_count_fold() : int {
    return _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3).count())
}

[export, marker(no_coverage)]
def target_skip_while_then_take_while_fold() : int {
    return _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3)._take_while(_ < 8).count())
}

[export, marker(no_coverage)]
def target_select_then_take_while_cascades_fold() : array<int> {
    // Cascade fingerprint: select precedes take_while → splice bails, tier-2 cascade runs.
    return <- _fold(each([5, 3, 8, 1])._select(_ * 2)._take_while(_ < 16).to_array())
}

[test]
def test_take_while_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_take_while_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "take_while: expected invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "take_while: one for-loop")
        // take_while is spliced inline as `if (!pred) break` — the library `take_while_impl`
        // helper must NOT appear as a runtime call.
        t |> equal(0, count_call(body_expr, "take_while"), "take_while: helper inlined")
        t |> equal(0, count_call(body_expr, "take_while_impl"), "take_while: impl inlined")
        t |> equal(0, count_call(body_expr, "take_while_impl_const"), "take_while: impl_const inlined")
    }
}

[test]
def test_take_while_count_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_take_while_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "take_while count: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "take_while count: one for-loop")
        t |> equal(0, count_call(body_expr, "take_while"), "take_while count: helper inlined")
    }
}

[test]
def test_take_while_sum_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_take_while_sum_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "take_while sum: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "take_while sum: one for-loop (accumulator lane)")
        t |> equal(0, count_call(body_expr, "take_while"), "take_while sum: helper inlined")
        t |> equal(0, count_call(body_expr, "sum"), "take_while sum: sum_impl inlined")
    }
}

[test]
def test_take_while_first_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_take_while_first_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "take_while first: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "take_while first: one for-loop (early-exit lane)")
        t |> equal(0, count_call(body_expr, "take_while"), "take_while first: helper inlined")
        t |> equal(0, count_call(body_expr, "first"), "take_while first: first inlined")
    }
}

[test]
def test_skip_while_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_skip_while_array_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "skip_while: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "skip_while: one for-loop")
        t |> equal(0, count_call(body_expr, "skip_while"), "skip_while: helper inlined")
        t |> equal(0, count_call(body_expr, "skip_while_impl"), "skip_while: impl inlined")
        t |> equal(0, count_call(body_expr, "skip_while_impl_const"), "skip_while: impl_const inlined")
    }
}

[test]
def test_skip_while_count_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_skip_while_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "skip_while count: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "skip_while count: one for-loop")
        t |> equal(0, count_call(body_expr, "skip_while"), "skip_while count: helper inlined")
    }
}

[test]
def test_skip_while_then_take_while_compose_splices(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_skip_while_then_take_while_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "skip_while+take_while: invoke wrapper")
        t |> equal(1, count_inner_for_loops(body_expr), "skip_while+take_while: one for-loop")
        t |> equal(0, count_call(body_expr, "skip_while"), "skip_while+take_while: skip_while inlined")
        t |> equal(0, count_call(body_expr, "take_while"), "skip_while+take_while: take_while inlined")
    }
}

[test]
def test_select_then_take_while_cascades_to_tier2(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_select_then_take_while_cascades_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "select-then-take_while: target found")
        // Cascade tier-2 fingerprint: multiple outer let-vars from fold_linq_default's
        // intermediate-pass pipeline. Distinct from the single-invoke splice shape.
        t |> success(count_outer_let_vars(body_expr) >= 2,
            "select-then-take_while: tier-2 cascade fingerprint (>=2 outer lets)")
    }
}

// ── order_by + first → min_by / max_by splice arm ───────────────────────

[export, marker(no_coverage)]
def target_order_by_first_splices_min_by() : int {
    return _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by(_).first())
}

[export, marker(no_coverage)]
def target_order_by_descending_first_splices_max_by() : int {
    return _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by_descending(_).first())
}

[export, marker(no_coverage)]
def target_where_order_by_first_splices_min_by_on_buf() : int {
    return _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._where(_ > 5)._order_by(_).first())
}

[export, marker(no_coverage)]
def target_order_by_first_or_default_splices_top_n_by() : int {
    return _fold(each([10, 20, 5, 8, 30])._order_by(_).first_or_default(-1))
}

[export, marker(no_coverage)]
def target_order_with_cmp_first_bails_to_tier2() : int {
    // order(arr, cmp) with a custom comparator — splice must bail (min/max can't honor arbitrary cmp).
    return _fold(each([10, 5, 20, 3]) |> order($(a : int, b : int) => a > b) |> first())
}

[test]
def test_order_by_first_emits_min_by(t : T?) {
    // order_by + first with an inlineable key now splices to the streaming-min walk —
    // single `var order_best` + `var order_seen` state, per-element `_::less(it, order_best)`
    // direct call. No min_by lambda dispatch, no full materialization.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_by_first_splices_min_by)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(0, count_call(body_expr, "min_by"), "streaming-min supersedes min_by")
        t |> equal(0, count_call(body_expr, "max_by"), "should NOT emit max_by (ascending)")
        t |> equal(0, count_call(body_expr, "order_by"), "should NOT emit order_by")
        t |> equal(0, count_call(body_expr, "first"), "should NOT emit first (splice absorbs)")
        t |> success(count_call(body_expr, "less") >= 1, "streaming-min uses inline less-test")
    }
}

[test]
def test_order_by_descending_first_emits_max_by(t : T?) {
    // Streaming-min handles descending by flipping the inline less-test operand order
    // (`_::less(order_best, it)`); same shape as ascending.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_by_descending_first_splices_max_by)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(0, count_call(body_expr, "max_by"), "streaming-min supersedes max_by")
        t |> equal(0, count_call(body_expr, "min_by"), "should NOT emit min_by (descending)")
        t |> equal(0, count_call(body_expr, "order_by_descending"), "should NOT emit order_by_descending")
        t |> equal(0, count_call(body_expr, "first"), "should NOT emit first (splice absorbs)")
        t |> success(count_call(body_expr, "less") >= 1, "streaming-min uses inline less-test (descending flips args)")
    }
}

[test]
def test_where_order_by_first_emits_min_by_on_prefilter(t : T?) {
    // where + order_by + first with an inlineable key splices to streaming-min with the
    // per-element body wrapped in `if (whereCond)`. Single fused loop — no prefilter buffer.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_where_order_by_first_splices_min_by_on_buf)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(0, count_call(body_expr, "min_by"), "streaming-min supersedes min_by")
        t |> equal(0, count_call(body_expr, "order_by"), "should NOT emit order_by")
        t |> equal(0, count_call(body_expr, "first"), "should NOT emit first (splice absorbs)")
        t |> equal(1, count_inner_for_loops(body_expr), "single fused walk over source (no prefilter buf)")
        t |> success(count_call(body_expr, "less") >= 1, "streaming-min uses inline less-test")
    }
}

[test]
def test_order_by_first_or_default_emits_top_n_by_first_or_default(t : T?) {
    // order_by + first_or_default with inlineable key splices to streaming-min + return-default
    // when !seen. No top_n_by(_, 1, _) materialization, no min_by_or_default helper.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_by_first_or_default_splices_top_n_by)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(0, count_call(body_expr, "top_n_by_with_cmp"), "streaming-min supersedes top_n_by_with_cmp")
        t |> equal(0, count_call(body_expr, "top_n_by"), "streaming-min supersedes top_n_by")
        t |> equal(0, count_call(body_expr, "first_or_default"), "streaming-min absorbs first_or_default")
        t |> equal(0, count_call(body_expr, "order_by"), "should NOT emit order_by")
        t |> success(count_call(body_expr, "less") >= 1, "streaming-min uses inline less-test")
    }
}

[test]
def test_order_with_cmp_first_bails_no_min_max(t : T?) {
    // `order(arr, cmp)` with a custom comparator block: splice helpers can't honor an arbitrary cmp,
    // so plan_order_family must bail. fold_linq_default takes over and rewrites to `order_to_array` + `first`.
    // The key invariant: NO min/max splice (which would silently drop the cmp).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_order_with_cmp_first_bails_to_tier2)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched, "should have return expression")
        t |> equal(0, count_call(body_expr, "min_by"), "must NOT emit min_by (cmp can't be honored)")
        t |> equal(0, count_call(body_expr, "max_by"), "must NOT emit max_by")
        t |> equal(0, count_call(body_expr, "min"), "must NOT emit min")
        t |> equal(0, count_call(body_expr, "max"), "must NOT emit max")
        t |> equal(0, count_call(body_expr, "top_n"), "must NOT emit top_n (bare top_n would drop cmp)")
        t |> equal(0, count_call(body_expr, "top_n_by"), "must NOT emit top_n_by")
        // fold_linq_default rewrites the bailed chain to order_to_array + first. Confirm a sort step survives.
        t |> success(count_call(body_expr, "order_to_array") + count_call(body_expr, "order") >= 1,
            "sort step must survive (order_to_array or order)")
        t |> success(count_call(body_expr, "first") >= 1, "first call must survive")
    }
}

