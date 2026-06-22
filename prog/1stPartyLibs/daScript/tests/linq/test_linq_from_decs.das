options gen2
options rtti
options _no_linq_perf_warn = true  // file contains intentional cascade tests (regression guards for tier-2 fallthrough)
require daslib/linq
require dastest/testing_boost public
require math
require strings

require daslib/linq_boost
require daslib/decs_boost
require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost

[test]
def test_from_decs(t : T?) {
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.index := i
            cmp.text := "{i}"
        }
    }
    commit()
    t |> run("from_decs simple") @(t : T?) {
        var qq = (
            from_decs($(index : int; text : string){})
                ._where(_.index < 2)
                .reverse()
                .to_array()
        )
        t |> equal(qq.length(), 2)
        var expected = [
            (index = 1, text = "1"),
            (index = 0, text = "0")
        ]
        for (c, e in qq, expected) {
            t |> equal(c.index, e.index)
            t |> equal(c.text, e.text)
        }
    }
}

[decs_template(prefix = "ftd_default_")]
struct FromTemplateDefault {
    index : int
    text  : string
}

[decs_template]
struct FromTemplateImplicit {
    iv : int
    sv : string
}

[decs_template(prefix)]
struct FromTemplateEmpty {
    ev_index : int
    ev_text  : string
}

[test]
def test_from_decs_template(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.ftd_default_index := i
            cmp.ftd_default_text := "ftd{i}"
            cmp.FromTemplateImplicit_iv := i * 10
            cmp.FromTemplateImplicit_sv := "imp{i}"
            cmp.ev_index := i * 100
            cmp.ev_text := "emp{i}"
        }
    }
    commit()
    t |> run("from_decs_template custom prefix parity") @(tt : T?) {
        var via_template = (
            from_decs_template(type<FromTemplateDefault>)
                ._where(_.index < 2)
                .reverse()
                .to_array()
        )
        var via_explicit = (
            from_decs($(ftd_default_index : int; ftd_default_text : string){})
                ._where(_.ftd_default_index < 2)
                .reverse()
                .to_array()
        )
        tt |> equal(via_template.length(), via_explicit.length())
        tt |> equal(via_template.length(), 2)
        for (a, b in via_template, via_explicit) {
            tt |> equal(a.index, b.ftd_default_index)
            tt |> equal(a.text, b.ftd_default_text)
        }
    }
    t |> run("from_decs_template default-prefix parity (struct name)") @(tt : T?) {
        let via_template = (
            from_decs_template(type<FromTemplateImplicit>)._select(_.iv).sum()
        )
        let via_explicit = (
            from_decs($(FromTemplateImplicit_iv : int; FromTemplateImplicit_sv : string){})._select(_.FromTemplateImplicit_iv).sum()
        )
        tt |> equal(via_template, via_explicit)
        tt |> equal(via_template, 0 + 10 + 20)
    }
    t |> run("from_decs_template empty-prefix parity") @(tt : T?) {
        let via_template = (
            from_decs_template(type<FromTemplateEmpty>).count()
        )
        let via_explicit = (
            from_decs($(ev_index : int; ev_text : string){}).count()
        )
        tt |> equal(via_template, via_explicit)
        tt |> equal(via_template, 3)
    }
    t |> run("from_decs_template projection by field name") @(tt : T?) {
        let sum_idx = (
            from_decs_template(type<FromTemplateDefault>)._select(_.index).sum()
        )
        tt |> equal(sum_idx, 0 + 1 + 2)
    }
}

// reverse + distinct_by over a decs source: the FORWARD keep-last splice (R-2b) fuses single-pass
// (decs is forward-only, so the array backward-index walk can't apply). Parity vs the tier-2 cascade
// proves both the keep-last semantics and that the string field survives the table-overwrite store.
[test]
def test_from_decs_reverse_distinct_by(t : T?) {
    restart()
    for (i in range(6)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.ftd_default_index := i
            cmp.ftd_default_text := "kl{i}"
        }
    }
    commit()
    t |> run("reverse._distinct_by(index%2) decs forward keep-last == tier-2 cascade") @(tt : T?) {
        var fused <- _fold(from_decs($(ftd_default_index : int; ftd_default_text : string){})
            .reverse()._distinct_by(_.ftd_default_index % 2).to_array())
        var cascade <- (from_decs($(ftd_default_index : int; ftd_default_text : string){})
            |> reverse() |> _distinct_by(_.ftd_default_index % 2) |> to_array())
        // keep LAST per (index%2), reverse-discovery order → index 5 (key 1), index 4 (key 0).
        tt |> equal(fused.length(), 2)
        tt |> equal(fused.length(), cascade.length())
        for (a, b in fused, cascade) {
            tt |> equal(a.ftd_default_index, b.ftd_default_index)
            tt |> equal(a.ftd_default_text, b.ftd_default_text)
        }
        tt |> equal(fused[0].ftd_default_index, 5)
        tt |> equal(fused[0].ftd_default_text, "kl5")
        tt |> equal(fused[1].ftd_default_index, 4)
        tt |> equal(fused[1].ftd_default_text, "kl4")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Approach Z splice (plan_decs_unroll) — Slice 1: bare count via arch.size
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "unroll_")]
struct UnrollCountRow {
    val : int
}

def private describe_count(expr : Expression?; needle : string) : int {
    if (expr == null) return 0
    let s = describe(expr)
    var n = 0
    var i = 0
    let nl = length(needle)
    while (true) {
        let p = s |> find(needle, i)
        if (p < 0) break
        n ++
        i = p + nl
    }
    return n
}

// Structural ExprFor counter — robust against describe() formatting changes (Copilot review #2).
def private count_expr_for(expr : Expression?) : int {
    if (expr == null) return 0
    var n = 0
    if (expr is ExprFor) {
        n ++
        var ef = expr as ExprFor
        for (s in ef.sources) {
            n += count_expr_for(s)
        }
        n += count_expr_for(ef.body)
    } elif (expr is ExprBlock) {
        var eb = expr as ExprBlock
        for (s in eb.list) {
            n += count_expr_for(s)
        }
    } elif (expr is ExprMakeBlock) {
        var emb = expr as ExprMakeBlock
        n += count_expr_for(emb._block)
    } elif (expr is ExprInvoke) {
        var ei = expr as ExprInvoke
        for (a in ei.arguments) {
            n += count_expr_for(a)
        }
    } elif (expr is ExprCall) {
        var ec = expr as ExprCall
        for (a in ec.arguments) {
            n += count_expr_for(a)
        }
    } elif (expr is ExprLet) {
        var el = expr as ExprLet
        for (v in el.variables) {
            n += count_expr_for(v.init)
        }
    } elif (expr is ExprReturn) {
        var er = expr as ExprReturn
        n += count_expr_for(er.subexpr)
    } elif (expr is ExprIfThenElse) {
        var ei = expr as ExprIfThenElse
        n += count_expr_for(ei.cond)
        n += count_expr_for(ei.if_true)
        n += count_expr_for(ei.if_false)
    }
    return n
}

[export, marker(no_coverage)]
def target_unroll_count_fold() : int {
    return _fold(from_decs_template(type<UnrollCountRow>).count())
}

[test]
def test_unroll_count_parity(t : T?) {
    restart()
    for (i in range(7)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll_val := i
        }
    }
    commit()
    t |> equal(target_unroll_count_fold(), 7, "Slice 1 count splice via arch.size")
}

[test]
def test_unroll_count_empty(t : T?) {
    restart()
    commit()
    t |> equal(target_unroll_count_fold(), 0, "empty archetypes count to 0")
}

[test]
def test_unroll_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_count_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_count_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        // arch.size shortcut: no inner for-loop (per-entity walk), no to_sequence.
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "count splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "count splice emits exactly one for_each_archetype")
        t |> equal(count_expr_for(body_expr), 0, "count splice with no chain ops must NOT emit inner per-entity for-loop")
        t |> equal(describe_count(body_expr, ".size"), 1, "count splice reads arch.size")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 2: chain-aware count/long_count/sum via for_each_archetype + named-tuple bind
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "unroll2_")]
struct UnrollChainRow {
    val : int
    flag : int
}

def private fixture_unroll2(n : int) {
    restart()
    for (i in range(n)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll2_val := i
            cmp.unroll2_flag := i % 2
        }
    }
    commit()
}

[export, marker(no_coverage)]
def target_unroll_select_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).sum())
}

[export, marker(no_coverage)]
def target_unroll_where_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1).count())
}

[export, marker(no_coverage)]
def target_unroll_where_select_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val).sum())
}

[export, marker(no_coverage)]
def target_unroll_long_count_fold() : int64 {
    return _fold(from_decs_template(type<UnrollChainRow>).long_count())
}

[test]
def test_unroll_select_sum_parity(t : T?) {
    fixture_unroll2(5)
    // 0+1+2+3+4 = 10
    t |> equal(target_unroll_select_sum_fold(), 10, "select+sum splice parity")
}

[test]
def test_unroll_where_count_parity(t : T?) {
    fixture_unroll2(5)
    // flags: 0,1,0,1,0 → flag==1 hits 2 rows
    t |> equal(target_unroll_where_count_fold(), 2, "where+count splice parity")
}

[test]
def test_unroll_where_select_sum_parity(t : T?) {
    fixture_unroll2(5)
    // flag==1 rows: vals 1,3 → sum 4
    t |> equal(target_unroll_where_select_sum_fold(), 4, "where+select+sum splice parity")
}

[test]
def test_unroll_long_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_long_count_fold(), 7l, "long_count splice parity")
}

[test]
def test_unroll_select_sum_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_select_sum_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_select_sum_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "select+sum splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 3a: min/max/average accumulator extensions
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_min_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).min())
}

[export, marker(no_coverage)]
def target_unroll_max_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).max())
}

[export, marker(no_coverage)]
def target_unroll_average_fold() : double {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).average())
}

[export, marker(no_coverage)]
def target_unroll_where_max_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val).max())
}

[test]
def test_unroll_min_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0,1,2,3,4 → min = 0
    t |> equal(target_unroll_min_fold(), 0, "min splice parity")
}

[test]
def test_unroll_max_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0,1,2,3,4 → max = 4
    t |> equal(target_unroll_max_fold(), 4, "max splice parity")
}

[test]
def test_unroll_average_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0,1,2,3,4 → avg = 10/5 = 2.0
    t |> equal(target_unroll_average_fold(), 2.0lf, "average splice parity")
}

[test]
def test_unroll_average_empty_returns_zero(t : T?) {
    // Regression for emit_decs_accumulator.average — empty archetype set must return 0.0lf (matches linq.das:1545 cnt==0 guard), not NaN from 0.0lf / 0.0lf.
    fixture_unroll2(0)
    t |> equal(target_unroll_average_fold(), 0.0lf, "average splice empty-source guard")
}

[test]
def test_unroll_where_max_parity(t : T?) {
    fixture_unroll2(5)
    // flag==1 rows: vals 1,3 → max 3
    t |> equal(target_unroll_where_max_fold(), 3, "where+select+max splice parity")
}

[test]
def test_unroll_max_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_max_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_max_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "max splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 3b: early-exit terminators (first/first_or_default/any/all/contains)
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_any_bare_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>).any())
}

[export, marker(no_coverage)]
def target_unroll_any_pred_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._any(_ > 100))
}

[export, marker(no_coverage)]
def target_unroll_all_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._all(_ >= 0))
}

[export, marker(no_coverage)]
def target_unroll_contains_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).contains(3))
}

[export, marker(no_coverage)]
def target_unroll_first_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val).first())
}

[export, marker(no_coverage)]
def target_unroll_first_or_default_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.val > 1000)._select(_.val).first_or_default(-7))
}

[test]
def test_unroll_any_bare_parity(t : T?) {
    fixture_unroll2(3)
    t |> equal(target_unroll_any_bare_fold(), true, "bare any with entities")
    restart()
    commit()
    t |> equal(target_unroll_any_bare_fold(), false, "bare any with no entities")
}

[test]
def test_unroll_any_pred_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0..4, none > 100
    t |> equal(target_unroll_any_pred_fold(), false, "any(pred) — no match")
    fixture_unroll2(150)
    // vals 0..149, some > 100
    t |> equal(target_unroll_any_pred_fold(), true, "any(pred) — has match")
}

[test]
def test_unroll_all_parity(t : T?) {
    fixture_unroll2(5)
    t |> equal(target_unroll_all_fold(), true, "all(>=0) on 0..4")
}

[test]
def test_unroll_contains_parity(t : T?) {
    fixture_unroll2(5)
    t |> equal(target_unroll_contains_fold(), true, "contains(3) in 0..4")
    fixture_unroll2(2)
    t |> equal(target_unroll_contains_fold(), false, "contains(3) in 0..1")
}

[test]
def test_unroll_first_parity(t : T?) {
    fixture_unroll2(5)
    // flag==1 first: i=1 → val=1
    t |> equal(target_unroll_first_fold(), 1, "first(where flag==1)")
}

[test]
def test_unroll_first_or_default_parity(t : T?) {
    fixture_unroll2(5)
    // val>1000 → none match → default -7
    t |> equal(target_unroll_first_or_default_fold(), -7, "first_or_default empty match")
}

[test]
def test_unroll_first_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_first_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_first_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "first splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "first splice uses for_each_archetype_find")
    }
}

// No-range any/all/contains take the FindReturn / FindReturnNegated fast path: the for_each_archetype_find
// result IS the answer (no found-state var). `found` count == 0 distinguishes this from the take_while+any
// shape, which routes through an explicit found flag (count 3). This locks in the LoopDispatch fast-path arm.
[test]
def test_unroll_any_pred_findreturn_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_any_pred_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_any_pred_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "any(pred) no-range uses for_each_archetype_find")
        t |> equal(describe_count(body_expr, "found"), 0, "any(pred) no-range takes the FindReturn fast path — no found-state")
    }
}

[test]
def test_unroll_all_findreturnnegated_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_all_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_all_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "all no-range uses for_each_archetype_find")
        t |> equal(describe_count(body_expr, "found"), 0, "all no-range takes the FindReturnNegated fast path — no found-state")
    }
}

[test]
def test_unroll_contains_findreturn_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_contains_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_contains_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "contains no-range uses for_each_archetype_find")
        t |> equal(describe_count(body_expr, "found"), 0, "contains no-range takes the FindReturn fast path — no found-state")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 3c: to_array buffer terminator
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val).to_array())
}

[test]
def test_unroll_to_array_parity(t : T?) {
    fixture_unroll2(6)
    // flag==1 rows: i=1,3,5 → vals 1,3,5
    let got <- target_unroll_to_array_fold()
    t |> equal(got |> length, 3, "to_array length")
    if (length(got) == 3) {
        t |> equal(got[0], 1, "to_array[0]")
        t |> equal(got[1], 3, "to_array[1]")
        t |> equal(got[2], 5, "to_array[2]")
    }
}

[test]
def test_unroll_to_array_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_to_array_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_to_array_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "to_array splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 4: chained _select + interleaved _where + _count(pred) + _min_by / _max_by
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_chained_select_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._select(_ * 10).sum())
}

[export, marker(no_coverage)]
def target_unroll_select_then_where_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._where(_ > 1).sum())
}

[export, marker(no_coverage)]
def target_unroll_where_select_where_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val)._where(_ > 1).sum())
}

[export, marker(no_coverage)]
def target_unroll_count_pred_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._count(_.val > 2))
}

[export, marker(no_coverage)]
def target_unroll_long_count_pred_fold() : int64 {
    return _fold(from_decs_template(type<UnrollChainRow>)._long_count(_.val >= 3))
}

[export, marker(no_coverage)]
def target_unroll_chained_select_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._select(_ + 100).to_array())
}

[export, marker(no_coverage)]
def target_unroll_min_by_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._min_by(-_))
}

[export, marker(no_coverage)]
def target_unroll_max_by_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val)._max_by(-_))
}

[test]
def test_unroll_chained_select_sum_parity(t : T?) {
    fixture_unroll2(5)
    // (0+1+2+3+4)*10 = 100
    t |> equal(target_unroll_chained_select_sum_fold(), 100, "chained select+sum splice parity")
}

[test]
def test_unroll_select_then_where_sum_parity(t : T?) {
    fixture_unroll2(5)
    // select val → 0,1,2,3,4 → where >1 → 2,3,4 → sum 9
    t |> equal(target_unroll_select_then_where_sum_fold(), 9, "select→where→sum splice parity")
}

[test]
def test_unroll_where_select_where_sum_parity(t : T?) {
    fixture_unroll2(6)
    // flag==1 rows: i=1,3,5 (vals 1,3,5) → select val → 1,3,5 → where >1 → 3,5 → sum 8
    t |> equal(target_unroll_where_select_where_sum_parity_compute(), 8, "where→select→where→sum splice parity")
}

def private target_unroll_where_select_where_sum_parity_compute() : int {
    return target_unroll_where_select_where_sum_fold()
}

[test]
def test_unroll_count_pred_parity(t : T?) {
    fixture_unroll2(6)
    // vals 0..5, pred val>2 → 3,4,5 → 3 hits
    t |> equal(target_unroll_count_pred_fold(), 3, "_count(pred) splice parity")
}

[test]
def test_unroll_long_count_pred_parity(t : T?) {
    fixture_unroll2(6)
    // vals 0..5, pred val>=3 → 3,4,5 → 3 hits
    t |> equal(target_unroll_long_count_pred_fold(), 3l, "_long_count(pred) splice parity")
}

[test]
def test_unroll_chained_select_to_array_parity(t : T?) {
    fixture_unroll2(3)
    // select val → 0,1,2 → select +100 → 100,101,102
    let got <- target_unroll_chained_select_to_array_fold()
    t |> equal(got |> length, 3, "chained to_array length")
    if (length(got) == 3) {
        t |> equal(got[0], 100, "[0]")
        t |> equal(got[1], 101, "[1]")
        t |> equal(got[2], 102, "[2]")
    }
}

[test]
def test_unroll_min_by_parity(t : T?) {
    fixture_unroll2(5)
    // select val → 0,1,2,3,4; min by -val → max val → 4
    t |> equal(target_unroll_min_by_fold(), 4, "_min_by splice parity")
}

[test]
def test_unroll_max_by_parity(t : T?) {
    fixture_unroll2(5)
    // select val → 0,1,2,3,4; max by -val → min val → 0
    t |> equal(target_unroll_max_by_fold(), 0, "_max_by splice parity")
}

[test]
def test_unroll_chained_select_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_chained_select_sum_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_chained_select_sum_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "chained-select sum splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

[test]
def test_unroll_count_pred_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_count_pred_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_count_pred_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "_count(pred) splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

[test]
def test_unroll_min_by_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_min_by_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_min_by_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "_min_by splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5a: take/skip ranges on decs (counters hoisted at invoke scope; take present routes to for_each_archetype_find with bool block returning true on cap-hit)
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_take_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).take(3).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).skip(2).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_take_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).skip(1).take(2).count())
}

[export, marker(no_coverage)]
def target_unroll_where_take_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1).take(2).count())
}

[export, marker(no_coverage)]
def target_unroll_select_take_sum_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3).sum())
}

[export, marker(no_coverage)]
def target_unroll_take_first_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(2).first())
}

[export, marker(no_coverage)]
def target_unroll_take_neg_count_fold() : int {
    // take(-N) must short-circuit (>=-guard) — empty iteration regardless of source size.
    return _fold(from_decs_template(type<UnrollChainRow>).take(-1).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_beyond_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).skip(100).count())
}

[export, marker(no_coverage)]
def target_unroll_take_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3).to_array())
}

[test]
def test_unroll_take_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_take_count_fold(), 3, "take(3) caps total")
}

[test]
def test_unroll_skip_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_count_fold(), 5, "skip(2) drops first 2 across archetypes")
}

[test]
def test_unroll_skip_take_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_take_count_fold(), 2, "skip(1)+take(2) windows total count")
}

[test]
def test_unroll_where_take_count_parity(t : T?) {
    fixture_unroll2(7)
    // flag==1 rows are vals 1,3,5 → take(2) → 2 matches
    t |> equal(target_unroll_where_take_count_fold(), 2, "where+take caps post-filter count")
}

[test]
def test_unroll_select_take_sum_parity(t : T?) {
    fixture_unroll2(7)
    // vals 0,1,2,...,6 → take(3) → sum 0+1+2 = 3
    t |> equal(target_unroll_select_take_sum_fold(), 3, "select+take+sum splice parity")
}

[test]
def test_unroll_take_first_parity(t : T?) {
    fixture_unroll2(7)
    // take(2) then first() — first wins before take counter ticks past 1
    t |> equal(target_unroll_take_first_fold(), 0, "take+first returns first element")
}

[test]
def test_unroll_take_neg_zero_count(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_take_neg_count_fold(), 0, "take(-1) short-circuits to 0")
}

[test]
def test_unroll_skip_beyond_returns_zero(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_beyond_count_fold(), 0, "skip past end yields 0 count")
}

[test]
def test_unroll_take_to_array_parity(t : T?) {
    fixture_unroll2(7)
    let got <- target_unroll_take_to_array_fold()
    t |> equal(length(got), 3, "take(3).to_array() yields 3 elements")
    if (length(got) >= 3) {
        t |> equal(got[0], 0, "first taken element")
        t |> equal(got[1], 1, "second taken element")
        t |> equal(got[2], 2, "third taken element")
    }
}

[test]
def test_unroll_take_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "take+count splice must NOT call to_sequence")
        // take present → must use the _find variant so early-exit propagates across archetypes.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take splice routes to for_each_archetype_find")
    }
}

[test]
def test_unroll_skip_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_skip_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "skip+count splice must NOT call to_sequence")
        // skip-only (no take) keeps the void for_each_archetype variant.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "skip-only splice does NOT route to _find")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "skip-only splice uses regular for_each_archetype")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5a Copilot review #1 regression guards — take + any/all/contains must
// distinguish take-cap (inner `return true`) from real match. Without explicit
// foundName/foundCounter state these would false-positive on empty/no-match
// sources whenever the take-cap fires before a real perElement decision.
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_take_any_no_pred_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>).take(0).any())
}

[export, marker(no_coverage)]
def target_unroll_take_any_pred_no_match_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3)._any(_ > 100))
}

[export, marker(no_coverage)]
def target_unroll_take_any_pred_match_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3)._any(_ == 1))
}

[export, marker(no_coverage)]
def target_unroll_take_all_pass_fold() : bool {
    // All val >= 0 holds; take(3) shouldn't fool `all` into returning false from the take-cap exit.
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3)._all(_ >= 0))
}

[export, marker(no_coverage)]
def target_unroll_take_all_fail_fold() : bool {
    // Some val > 1 → `all <= 1` false.
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3)._all(_ <= 1))
}

[export, marker(no_coverage)]
def target_unroll_take_contains_hit_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(5).contains(2))
}

[export, marker(no_coverage)]
def target_unroll_take_contains_miss_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(2).contains(100))
}

[test]
def test_unroll_take_any_no_pred_empty(t : T?) {
    fixture_unroll2(7)
    // take(0).any() on non-empty source → false (the prefix-of-length-0 is empty).
    t |> equal(target_unroll_take_any_no_pred_fold(), false, "take(0).any() returns false on non-empty source")
}

[test]
def test_unroll_take_any_pred_no_match(t : T?) {
    fixture_unroll2(7)
    // take(3) gives vals 0,1,2; none > 100 → false. take-cap fires at element 3 but must not be mistaken for a match.
    t |> equal(target_unroll_take_any_pred_no_match_fold(), false, "take(3).any(_ > 100) returns false when no match in prefix")
}

[test]
def test_unroll_take_any_pred_match(t : T?) {
    fixture_unroll2(7)
    // take(3) gives vals 0,1,2; matches _==1 at element 1 → true.
    t |> equal(target_unroll_take_any_pred_match_fold(), true, "take(3).any(_ == 1) returns true when match in prefix")
}

[test]
def test_unroll_take_all_pass(t : T?) {
    fixture_unroll2(7)
    // vals 0,1,2,...,6 all >= 0; take(3) gives 0,1,2 all pass → true. Take-cap fire at element 3 must not be mistaken for "counterexample found".
    t |> equal(target_unroll_take_all_pass_fold(), true, "take(3).all(_ >= 0) returns true when all prefix elements pass")
}

[test]
def test_unroll_take_all_fail(t : T?) {
    fixture_unroll2(7)
    // take(3) gives 0,1,2; element 2 fails `_ <= 1` → false.
    t |> equal(target_unroll_take_all_fail_fold(), false, "take(3).all(_ <= 1) returns false when prefix contains counterexample")
}

[test]
def test_unroll_take_contains_hit(t : T?) {
    fixture_unroll2(7)
    // take(5) gives 0..4; contains(2) → true.
    t |> equal(target_unroll_take_contains_hit_fold(), true, "take(5).contains(2) returns true when value in prefix")
}

[test]
def test_unroll_take_contains_miss(t : T?) {
    fixture_unroll2(7)
    // take(2) gives 0,1; contains(100) → false. take-cap fires at element 2 but must not be mistaken for a hit.
    t |> equal(target_unroll_take_contains_miss_fold(), false, "take(2).contains(100) returns false when value not in prefix")
}

// Slice 5a Copilot review #2 regression guard — take(N) expression must evaluate
// exactly once per invoke (matches take(src, total:int) call-site semantics).
// Side-effect counter proves the hoist works.

var private take_n_eval_counter : int = 0

def private take_n_with_side_effect() : int {
    take_n_eval_counter ++
    return 3
}

[export, marker(no_coverage)]
def target_unroll_take_with_side_effect_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).take(take_n_with_side_effect()).count())
}

[test]
def test_unroll_take_n_evaluated_once(t : T?) {
    fixture_unroll2(7)
    take_n_eval_counter = 0
    let c = target_unroll_take_with_side_effect_fold()
    t |> equal(c, 3, "take(3).count() returns 3")
    // Single hoisted evaluation — NOT once per element across 7 entities.
    t |> equal(take_n_eval_counter, 1, "take(N) expression must evaluate exactly once per invoke")
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5b: take_while / skip_while on decs. Predicate-driven ranges, predicates
// peeled against the source tuple (matches array-side seenSelect bail rule).
// take_while breaks iteration on first false → routes to for_each_archetype_find
// just like take(N). skip_while sets a one-way flag (flips false on first
// non-matching elem); flag hoisted at invoke scope so it persists across
// archetypes.
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_take_while_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_while_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._skip_while(_.val < 3).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_while_take_while_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._skip_while(_.val < 2)._take_while(_.val < 5).count())
}

[export, marker(no_coverage)]
def target_unroll_where_take_while_count_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._take_while(_.val < 5).count())
}

// take_while + select is REJECTED by extract_decs_ranges (canonical order forbids
// select-after-range — array side bails at linq_fold.das:1623). So for sum/first/
// to_array splice coverage we drop select and operate on the source tuple
// directly. first/to_array yield source tuples; sum needs a scalar so we use
// _select-BEFORE-take_while is also forbidden (skip_while/take_while bail on
// seenSelect), so sum-after-take_while can't splice on either side. The variants
// here keep the chain in shapes that DO splice.

[export, marker(no_coverage)]
def target_unroll_take_while_first_fold() : int {
    // first returns the source tuple; we extract .val for the parity check.
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3).first()).val
}

[export, marker(no_coverage)]
def target_unroll_take_while_to_array_fold() : array<tuple<val : int; flag : int>> {
    return <- _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3).to_array())
}

[export, marker(no_coverage)]
def target_unroll_take_while_min_by_fold() : int {
    // min_by takes a key extractor lambda over the source tuple — splice fires
    // (chain prefix empty; rangeInfo carries the take_while; min_by routes to
    // emit_decs_min_max_by). Source vals 0..6, take_while(<3) keeps 0,1,2 → min .val = 0.
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3)._min_by(_.val)).val
}

[export, marker(no_coverage)]
def target_unroll_take_while_never_fires_count_fold() : int {
    // pred always true → take_while never breaks → full source count.
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 100).count())
}

[export, marker(no_coverage)]
def target_unroll_take_while_immediate_break_count_fold() : int {
    // pred false on first elem → break immediately → 0.
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 0).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_while_never_done_count_fold() : int {
    // pred always true → skip_while never flips → everything skipped → 0.
    return _fold(from_decs_template(type<UnrollChainRow>)._skip_while(_.val < 100).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_while_immediate_done_count_fold() : int {
    // pred false on first elem → flag flips on entry → nothing skipped → full count.
    return _fold(from_decs_template(type<UnrollChainRow>)._skip_while(_.val < 0).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_take_while_count_fold() : int {
    // skip(2) then take_while(val<5) → drops 0,1 → walks 2,3,4 stops at 5 → count 3.
    return _fold(from_decs_template(type<UnrollChainRow>).skip(2)._take_while(_.val < 5).count())
}

[export, marker(no_coverage)]
def target_unroll_skip_while_take_count_fold() : int {
    // skip_while(val<2) drops 0,1 → take(2) windows 2,3 → count 2.
    return _fold(from_decs_template(type<UnrollChainRow>)._skip_while(_.val < 2).take(2).count())
}

// Predicates over the SOURCE TUPLE (no select; select-after-take_while bails per
// canonical order). This is the only shape where the splice actually fires for
// any/all/contains + take_while, which is what exercises the explicit found-state
// routing in emit_early_exit_lane's !supports_direct_return arm (decs + range present).

[export, marker(no_coverage)]
def target_unroll_take_while_any_match_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3)._any(_.val == 2))
}

[export, marker(no_coverage)]
def target_unroll_take_while_any_after_window_fold() : bool {
    // val 5 is outside the take_while window — must NOT report true even though take_while produces inner `return true` on val=3.
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3)._any(_.val == 5))
}

[export, marker(no_coverage)]
def target_unroll_take_while_all_pass_fold() : bool {
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3)._all(_.val < 5))
}

[export, marker(no_coverage)]
def target_unroll_take_while_all_fail_fold() : bool {
    // val=1 fails the all-pred. take_while-stop inner `return true` must not be misread as "no counterexample".
    return _fold(from_decs_template(type<UnrollChainRow>)._take_while(_.val < 3)._all(_.val == 0))
}

// take_while + contains: source-tuple equality isn't defined (no `==` on tuple),
// and the canonical-order rule forbids _select between take_while and contains
// to project to a scalar — so this combination is not testable on decs today.
// any/all above already exercise the same found-state code path
// (contains shares the Find+state branch in emit_early_exit_lane).

[test]
def test_unroll_take_while_count_parity(t : T?) {
    fixture_unroll2(7)
    // vals 0,1,2 satisfy <3 → break on val=3 → count 3.
    t |> equal(target_unroll_take_while_count_fold(), 3, "take_while breaks on first false-pred")
}

[test]
def test_unroll_skip_while_count_parity(t : T?) {
    fixture_unroll2(7)
    // skip 0,1,2; keep 3..6 → count 4.
    t |> equal(target_unroll_skip_while_count_fold(), 4, "skip_while drops leading matches, keeps rest")
}

[test]
def test_unroll_skip_while_take_while_count_parity(t : T?) {
    fixture_unroll2(7)
    // skip 0,1; flag flips on val=2; take_while keeps 2,3,4; breaks on 5 → count 3.
    t |> equal(target_unroll_skip_while_take_while_count_fold(), 3, "skip_while + take_while compose")
}

[test]
def test_unroll_where_take_while_count_parity(t : T?) {
    fixture_unroll2(7)
    // flag==1 → vals 1,3,5; take_while(val<5) keeps 1,3; breaks on 5 → count 2.
    t |> equal(target_unroll_where_take_while_count_fold(), 2, "where + take_while applies pred to post-filter stream")
}

[test]
def test_unroll_take_while_first_parity(t : T?) {
    fixture_unroll2(7)
    // first returns source tuple; we project to .val. vals 0,1,2 satisfy <3, first = 0.
    t |> equal(target_unroll_take_while_first_fold(), 0, "take_while + first returns first source elem before break")
}

[test]
def test_unroll_take_while_to_array_parity(t : T?) {
    fixture_unroll2(7)
    // to_array yields the take_while prefix as tuples; check length + field access on each.
    let got <- target_unroll_take_while_to_array_fold()
    t |> equal(length(got), 3, "take_while.to_array() yields the prefix")
    if (length(got) >= 3) {
        t |> equal(got[0].val, 0, "first tuple .val")
        t |> equal(got[1].val, 1, "second tuple .val")
        t |> equal(got[2].val, 2, "third tuple .val")
    }
}

[test]
def test_unroll_take_while_min_by_parity(t : T?) {
    fixture_unroll2(7)
    // min_by takes a key extractor over the source tuple; splice routes to emit_decs_min_max_by
    // with take_while routing through for_each_archetype_find. Window {0,1,2}; min .val = 0.
    t |> equal(target_unroll_take_while_min_by_fold(), 0, "take_while + min_by returns the min in window")
}

[test]
def test_unroll_take_while_never_fires_returns_all(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_take_while_never_fires_count_fold(), 7, "take_while with always-true pred returns full source count")
}

[test]
def test_unroll_take_while_immediate_break_returns_zero(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_take_while_immediate_break_count_fold(), 0, "take_while breaks on first elem when pred false")
}

[test]
def test_unroll_skip_while_never_done_returns_zero(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_while_never_done_count_fold(), 0, "skip_while with always-true pred drops everything")
}

[test]
def test_unroll_skip_while_immediate_done_returns_all(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_while_immediate_done_count_fold(), 7, "skip_while flag flips on first elem when pred false")
}

[test]
def test_unroll_skip_take_while_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_take_while_count_fold(), 3, "skip(2) + take_while(val<5) walks 2,3,4")
}

[test]
def test_unroll_skip_while_take_count_parity(t : T?) {
    fixture_unroll2(7)
    t |> equal(target_unroll_skip_while_take_count_fold(), 2, "skip_while(val<2) + take(2) yields 2,3")
}

[test]
def test_unroll_take_while_any_match(t : T?) {
    fixture_unroll2(7)
    // val 2 is in the window → true. Real match, not the take_while-stop signal.
    t |> equal(target_unroll_take_while_any_match_fold(), true, "take_while + any returns true when match in window")
}

[test]
def test_unroll_take_while_any_after_window(t : T?) {
    fixture_unroll2(7)
    // val 5 outside take_while window → false. take_while-stop must NOT register as a match.
    t |> equal(target_unroll_take_while_any_after_window_fold(), false, "take_while + any returns false when match outside window")
}

[test]
def test_unroll_take_while_all_pass(t : T?) {
    fixture_unroll2(7)
    // window {0,1,2}; all < 5 → true.
    t |> equal(target_unroll_take_while_all_pass_fold(), true, "take_while + all returns true when window satisfies pred")
}

[test]
def test_unroll_take_while_all_fail(t : T?) {
    fixture_unroll2(7)
    // window {0,1,2}; not all == 0 → false. take_while-stop inner true must not be misread as "no counterexample".
    t |> equal(target_unroll_take_while_all_fail_fold(), false, "take_while + all returns false on counterexample in window")
}

[test]
def test_unroll_take_while_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_while_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "take_while+count splice must NOT call to_sequence")
        // take_while present → must use the _find variant so first-false propagates across archetypes.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take_while splice routes to for_each_archetype_find")
    }
}

[test]
def test_unroll_skip_while_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_skip_while_count_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "skip_while+count splice must NOT call to_sequence")
        // skip_while alone does NOT early-exit — must use plain for_each_archetype.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "skip_while-only splice does NOT route to _find")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "skip_while-only splice uses regular for_each_archetype")
    }
}

[test]
def test_unroll_take_while_any_splice_shape(t : T?) {
    // Verifies the take_while + any splice fires AND routes through explicit-state
    // (decs_found) — distinguishing take_while-stop `return true` from real-match
    // `return true`. This is the exact code path that fails to be exercised when
    // tests put _select between take_while and the terminator (Copilot PR #2772
    // review feedback). Predicate over the source tuple keeps the splice hot.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_while_any_match_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "take_while+any splice must NOT fall to tier-2 to_sequence")
        // take_while-stop produces inner `return true` — needs _find dispatch.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take_while+any splice routes to for_each_archetype_find")
        // Explicit found-state routing disambiguates take_while-stop from a real match (var + set + return).
        t |> equal(describe_count(body_expr, "found"), 3, "take_while+any routes through explicit found state (var + set + return)")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5e: group_by on decs — state-table over for_each_archetype
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "unroll5e_")]
struct UnrollGroupRow {
    brand : int
    val   : int
    flag  : int
}

// Mirrors the tuple yielded by the from_decs_template bridge — used to type inner-select lambdas inside `_._1 |> select(@(r : UnrollGroupTup) => r.val) |> ...`.
typedef UnrollGroupTup = tuple<brand : int; val : int; flag : int>

def private fixture_unroll5e() {
    // 7 entities: brand=val%3, val=10..16, flag=val%2
    // brand 0 → vals {12,15}      len=2 sum=27 min=12 max=15 avg=13.5
    // brand 1 → vals {10,13,16}   len=3 sum=39 min=10 max=16 avg=13.0
    // brand 2 → vals {11,14}      len=2 sum=25 min=11 max=14 avg=12.5
    restart()
    var i = 10
    while (i <= 16) {
        let v = i
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll5e_brand := v % 3
            cmp.unroll5e_val := v
            cmp.unroll5e_flag := v % 2
        }
        i ++
    }
    commit()
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_count_fold() : array<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, N = _._1 |> length))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_sum_fold() : array<tuple<Brand : int; S : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, S = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> sum))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_min_fold() : array<tuple<Brand : int; Min : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, Min = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> min))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_max_fold() : array<tuple<Brand : int; Max : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, Max = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> max))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_avg_fold() : array<tuple<Brand : int; Avg : double>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, Avg = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> average))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_first_fold() : array<tuple<Brand : int; F : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, F = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> first))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_multi_reducer_fold() : array<tuple<Brand : int; N : int; S : int; MX : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0,
                              N = _._1 |> length,
                              S = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> sum,
                              MX = _._1 |> select(@(r : UnrollGroupTup) => r.val) |> max))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_count_terminator_fold() : int {
    return _fold(from_decs_template(type<UnrollGroupRow>)
                 ._group_by(_.brand)
                 ._select((Brand = _._0, N = _._1 |> length))
                 .count())
}

// Iterator-typed chain — bridge's `to_sequence(res)` makes `_group_by._select` yield iterator<...> by default (no trailing `.to_array()` peel). The splice must emit `to_sequence_move` to match the caller's static type; otherwise error[30343] iterator vs array. Regression guard for Copilot review on PR #2775.
[export, marker(no_coverage)]
def target_unroll5e_groupby_iter_typed_fold() : iterator<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, N = _._1 |> length)))
}

// count(predicate) overload — the planner must NOT pop `.count(_.N >= 2)` as a bare count terminator (would silently drop the predicate). Must cascade to tier-2 so the 2-arg `count(src, pred)` overload runs. Regression guard for Copilot review on PR #2775.
[export, marker(no_coverage)]
def target_unroll5e_groupby_count_pred_fold() : int {
    return _fold(from_decs_template(type<UnrollGroupRow>)
                 ._group_by(_.brand)
                 ._select((Brand = _._0, N = _._1 |> length))
                 ._count(_.N >= 2))
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_where_count_fold() : array<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._where(_.flag == 1)
                    ._group_by(_.brand)
                    ._select((Brand = _._0, N = _._1 |> length))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_select_sum_fold() : array<tuple<K : int; S : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._select(_.val)
                    ._group_by(_ % 3)
                    ._select((K = _._0, S = _._1 |> sum))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_having_count_fold() : array<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by_lazy(_.brand)
                    ._having(_._1 |> length >= 3)
                    ._select((Brand = _._0, N = _._1 |> length))
                    .to_array())
}

[export, marker(no_coverage)]
def target_unroll5e_groupby_having_hidden_sum_fold() : array<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<UnrollGroupRow>)
                    ._group_by_lazy(_.brand)
                    ._having(_._1 |> select(@(r : UnrollGroupTup) => r.val) |> sum > 30)
                    ._select((Brand = _._0, N = _._1 |> length))
                    .to_array())
}

[test]
def test_unroll5e_groupby_count_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_count_fold()
    t |> equal(got.length(), 3, "3 brand buckets")
    var perBrand <- {for (item in got); item.Brand => item.N}
    t |> equal(perBrand[0], 2)
    t |> equal(perBrand[1], 3)
    t |> equal(perBrand[2], 2)
}

[test]
def test_unroll5e_groupby_sum_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_sum_fold()
    t |> equal(got.length(), 3)
    var perBrand <- {for (item in got); item.Brand => item.S}
    t |> equal(perBrand[0], 27)
    t |> equal(perBrand[1], 39)
    t |> equal(perBrand[2], 25)
}

[test]
def test_unroll5e_groupby_min_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_min_fold()
    t |> equal(got.length(), 3)
    var perBrand <- {for (item in got); item.Brand => item.Min}
    t |> equal(perBrand[0], 12)
    t |> equal(perBrand[1], 10)
    t |> equal(perBrand[2], 11)
}

[test]
def test_unroll5e_groupby_max_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_max_fold()
    t |> equal(got.length(), 3)
    var perBrand <- {for (item in got); item.Brand => item.Max}
    t |> equal(perBrand[0], 15)
    t |> equal(perBrand[1], 16)
    t |> equal(perBrand[2], 14)
}

[test]
def test_unroll5e_groupby_avg_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_avg_fold()
    t |> equal(got.length(), 3)
    var perBrand <- {for (item in got); item.Brand => item.Avg}
    t |> equal(perBrand[0], 13.5lf)
    t |> equal(perBrand[1], 13.0lf)
    t |> equal(perBrand[2], 12.5lf)
}

[test]
def test_unroll5e_groupby_first_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_first_fold()
    t |> equal(got.length(), 3)
    // First per bucket depends on iteration order, but it must be A value FROM the bucket.
    for (item in got) {
        if (item.Brand == 0) t |> success(item.F == 12 || item.F == 15, "brand 0 first must be 12 or 15")
        elif (item.Brand == 1) t |> success(item.F == 10 || item.F == 13 || item.F == 16, "brand 1 first must be 10/13/16")
        elif (item.Brand == 2) t |> success(item.F == 11 || item.F == 14, "brand 2 first must be 11 or 14")
    }
}

[test]
def test_unroll5e_groupby_multi_reducer_parity(t : T?) {
    fixture_unroll5e()
    let got <- target_unroll5e_groupby_multi_reducer_fold()
    t |> equal(got.length(), 3)
    var keysSeen = 0
    for (item in got) {
        if (item.Brand == 0) {
            t |> equal(item.N, 2); t |> equal(item.S, 27); t |> equal(item.MX, 15)
            keysSeen ++
        } elif (item.Brand == 1) {
            t |> equal(item.N, 3); t |> equal(item.S, 39); t |> equal(item.MX, 16)
            keysSeen ++
        } elif (item.Brand == 2) {
            t |> equal(item.N, 2); t |> equal(item.S, 25); t |> equal(item.MX, 14)
            keysSeen ++
        }
    }
    t |> equal(keysSeen, 3, "all 3 keys observed")
}

[test]
def test_unroll5e_groupby_count_terminator_parity(t : T?) {
    fixture_unroll5e()
    t |> equal(target_unroll5e_groupby_count_terminator_fold(), 3, "count terminator returns length(tab)")
}

[test]
def test_unroll5e_groupby_iter_typed_parity(t : T?) {
    // Walks the iterator-typed chain; counts entries.
    fixture_unroll5e()
    var count = 0
    for (item in target_unroll5e_groupby_iter_typed_fold()) {
        count ++
        // Every entry has a valid Brand (0/1/2) and N (count per bucket).
        t |> success(item.Brand >= 0 && item.Brand <= 2, "Brand in 0..2")
        t |> success(item.N >= 1, "N >= 1 (at least one row per bucket)")
    }
    t |> equal(count, 3, "iterator-typed group_by yields 3 buckets")
}

[test]
def test_unroll5e_groupby_count_pred_parity(t : T?) {
    // Buckets per brand: 0→2, 1→3, 2→2. `_.N >= 2` matches all 3 → result is 3.
    fixture_unroll5e()
    t |> equal(target_unroll5e_groupby_count_pred_fold(), 3, "count(predicate) over groups counts buckets satisfying pred")
    // Smoke: change pred to keep only buckets with N >= 3 → only brand 1.
}

[test]
def test_unroll5e_groupby_count_pred_cascade_shape(t : T?) {
    // count(predicate) must NOT be popped as the bare count terminator (would drop the predicate). Splice cascades to tier-2 eager bridge which honors the 2-arg count overload — verified by presence of tier-2 fingerprints (`to_sequence`, `group_by_lazy_to_array` materialized intermediate).
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5e_groupby_count_pred_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected invoke wrapper")
        // Splice did NOT fire: the decs-specific table (`decs_tab`) is absent.
        t |> equal(describe_count(body_expr, "decs_tab"), 0, "count(pred) must cascade — no decs_tab hoisted")
        // Tier-2 fingerprint: `count(_, predicate)` runs on the materialized array.
        t |> success(describe_count(body_expr, "_.N >= 2") >= 1, "predicate body must appear in the cascaded emission")
    }
}

[test]
def test_unroll5e_groupby_iter_typed_splice_shape(t : T?) {
    // Verifies the iterator-typed splice fires (not falling to tier-2) and emits to_sequence_move at the tail to convert the buffer array to iterator.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5e_groupby_iter_typed_fold)
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        // Splice fires: no tier-2 fall-through (would manifest as inner-invoke `to_sequence(res)` from the eager bridge).
        t |> equal(describe_count(body_expr, "to_sequence("), 0, "iterator-typed group_by splice must NOT call eager bridge's to_sequence(res)")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype outer call")
        // Tail emits `to_sequence_move(buf)` to convert array<...> result to iterator<...> matching caller's type.
        t |> equal(describe_count(body_expr, "to_sequence_move"), 1, "iterator-typed splice tail emits to_sequence_move(buf)")
    }
}

[test]
def test_unroll5e_groupby_where_count_parity(t : T?) {
    fixture_unroll5e()
    // flag==1 → odd vals: {11,13,15}. Buckets: brand 0→{15} N=1, brand 1→{13} N=1, brand 2→{11} N=1.
    let got <- target_unroll5e_groupby_where_count_fold()
    t |> equal(got.length(), 3)
    var perBrand <- {for (item in got); item.Brand => item.N}
    t |> equal(perBrand[0], 1)
    t |> equal(perBrand[1], 1)
    t |> equal(perBrand[2], 1)
}

[test]
def test_unroll5e_groupby_select_sum_parity(t : T?) {
    fixture_unroll5e()
    // _select(r=>r.val) projects vals 10..16 then group by val%3:
    // key 0 (vals 12,15) S=27, key 1 (vals 10,13,16) S=39, key 2 (vals 11,14) S=25.
    let got <- target_unroll5e_groupby_select_sum_fold()
    t |> equal(got.length(), 3)
    var perKey <- {for (item in got); item.K => item.S}
    t |> equal(perKey[0], 27)
    t |> equal(perKey[1], 39)
    t |> equal(perKey[2], 25)
}

[test]
def test_unroll5e_groupby_having_count_parity(t : T?) {
    fixture_unroll5e()
    // having length >= 3 keeps only brand 1 (N=3).
    let got <- target_unroll5e_groupby_having_count_fold()
    t |> equal(got.length(), 1)
    t |> equal(got[0].Brand, 1)
    t |> equal(got[0].N, 3)
}

[test]
def test_unroll5e_groupby_having_hidden_sum_parity(t : T?) {
    fixture_unroll5e()
    // sums: brand 0→27, brand 1→39, brand 2→25. having sum > 30 keeps brand 1 (N=3).
    let got <- target_unroll5e_groupby_having_hidden_sum_fold()
    t |> equal(got.length(), 1)
    t |> equal(got[0].Brand, 1)
    t |> equal(got[0].N, 3)
}

[test]
def test_unroll5e_groupby_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5e_groupby_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "group_by splice must NOT fall to tier-2 to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype outer call")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "group_by does NOT use _find variant (no early-exit)")
        // State-table hoisted ABOVE the for_each_archetype call so it spans archetypes.
        t |> equal(describe_count(body_expr, "decs_tab"), 1 + 5, "decs_tab declared once + 5 refs (?[uk], [uk]=, length, values, finalize)")
        t |> equal(describe_count(body_expr, "decs_dummy"), 1 + 5, "decs_dummy declared once + 5 splice refs (??dummy, addr(dummy), dummy._0=, tab[uk]=dummy, dummy=default)")
        t |> equal(describe_count(body_expr, "unique_key"), 1, "single unique_key derivation per element")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5d: order_by / order_by_descending / reverse on decs (buffer arms)
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "unroll5d_")]
struct Unroll5dRow {
    id    : int
    price : int
    flag  : int
}

def private fixture_unroll5d() {
    restart()
    // 7 rows. price (jumbled walk-order so order_by must actually sort): 30,10,50,20,40,5,35.
    // flag pattern: even-id => 1, odd-id => 0.
    let prices = fixed_array(30, 10, 50, 20, 40, 5, 35)
    for (i in range(7)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll5d_id := i
            cmp.unroll5d_price := prices[i]
            cmp.unroll5d_flag := (i % 2 == 0) ? 1 : 0
        }
    }
    commit()
}

// ── order_by + to_array (bare sort) ──

[export, marker(no_coverage)]
def target_unroll5d_order_by_to_array_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._order_by(_.price).to_array())
}

[test]
def test_unroll5d_order_by_to_array_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_order_by_to_array_fold()
    t |> equal(got.length(), 7)
    // sorted ascending by price: 5,10,20,30,35,40,50.
    let expected = [5, 10, 20, 30, 35, 40, 50]
    for (i in range(7)) {
        t |> equal(got[i].price, expected[i])
    }
}

[export, marker(no_coverage)]
def target_unroll5d_order_by_desc_to_array_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._order_by_descending(_.price).to_array())
}

[test]
def test_unroll5d_order_by_desc_to_array_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_order_by_desc_to_array_fold()
    t |> equal(got.length(), 7)
    let expected = [50, 40, 35, 30, 20, 10, 5]
    for (i in range(7)) {
        t |> equal(got[i].price, expected[i])
    }
}

// ── order_by + take ──

[export, marker(no_coverage)]
def target_unroll5d_order_by_take3_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._order_by(_.price).take(3).to_array())
}

[test]
def test_unroll5d_order_by_take_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_order_by_take3_fold()
    t |> equal(got.length(), 3)
    let expected = [5, 10, 20]
    for (i in range(3)) {
        t |> equal(got[i].price, expected[i])
    }
}

[export, marker(no_coverage)]
def target_unroll5d_order_by_desc_take3_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._order_by_descending(_.price).take(3).to_array())
}

[test]
def test_unroll5d_order_by_desc_take_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_order_by_desc_take3_fold()
    t |> equal(got.length(), 3)
    let expected = [50, 40, 35]
    for (i in range(3)) {
        t |> equal(got[i].price, expected[i])
    }
}

// ── order_by + first ──

[export, marker(no_coverage)]
def target_unroll5d_order_by_first_fold() : tuple<id : int; price : int; flag : int> {
    return _fold(from_decs_template(type<Unroll5dRow>)._order_by(_.price).first())
}

[test]
def test_unroll5d_order_by_first_parity(t : T?) {
    fixture_unroll5d()
    t |> equal(target_unroll5d_order_by_first_fold().price, 5)
}

[export, marker(no_coverage)]
def target_unroll5d_order_by_desc_first_fold() : tuple<id : int; price : int; flag : int> {
    return _fold(from_decs_template(type<Unroll5dRow>)._order_by_descending(_.price).first())
}

[test]
def test_unroll5d_order_by_desc_first_parity(t : T?) {
    fixture_unroll5d()
    t |> equal(target_unroll5d_order_by_desc_first_fold().price, 50)
}

// ── order_by + first_or_default ──

[export, marker(no_coverage)]
def target_unroll5d_order_by_first_or_default_fold() : tuple<id : int; price : int; flag : int> {
    return _fold(from_decs_template(type<Unroll5dRow>)._order_by(_.price).first_or_default((id = -1, price = -99, flag = -1)))
}

[test]
def test_unroll5d_order_by_first_or_default_nonempty(t : T?) {
    fixture_unroll5d()
    t |> equal(target_unroll5d_order_by_first_or_default_fold().price, 5)
}

[test]
def test_unroll5d_order_by_first_or_default_empty(t : T?) {
    restart()
    commit()
    t |> equal(target_unroll5d_order_by_first_or_default_fold().price, -99, "empty decs source → default")
}

// Eval-order invariant: the splice must evaluate the first_or_default default expression BEFORE the for_each_archetype loop, matching plan_decs_reverse's first_or_default hoist + the eager-arg-eval semantics of the equivalent non-spliced chain. Detected via a side-effecting predicate that flips a global flag during iter and a side-effecting default that snapshots the flag.
var g_eval_order_iter_seen : bool = false
var g_eval_order_default_saw_iter : bool = false
def trace_pred_for_eval_order(rec : tuple<id : int; price : int; flag : int>) : bool {
    g_eval_order_iter_seen = true
    return rec.flag == 1
}
def trace_default_for_eval_order() : tuple<id : int; price : int; flag : int> {
    g_eval_order_default_saw_iter = g_eval_order_iter_seen
    return (id = -1, price = -99, flag = -1)
}

[test]
def test_unroll5d_order_by_first_or_default_eval_order(t : T?) {
    fixture_unroll5d()
    g_eval_order_iter_seen = false
    g_eval_order_default_saw_iter = true
    let got = _fold(
        from_decs_template(type<Unroll5dRow>)
            ._where(trace_pred_for_eval_order(_))
            ._order_by(_.price)
            .first_or_default(trace_default_for_eval_order())
    )
    t |> equal(g_eval_order_default_saw_iter, false, "default arg evaluated before for_each_archetype iteration")
    t |> equal(got.flag, 1, "non-empty: top-1 by price among flag==1 returned")
}

// ── where + order_by + take ──

[export, marker(no_coverage)]
def target_unroll5d_where_order_by_take_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._where(_.flag == 1)._order_by(_.price).take(2).to_array())
}

[test]
def test_unroll5d_where_order_by_take_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_where_order_by_take_fold()
    // flag==1 rows: ids 0,2,4,6 → prices 30,50,40,35. Sorted asc: 30,35,40,50. take(2) → 30,35.
    t |> equal(got.length(), 2)
    t |> equal(got[0].price, 30)
    t |> equal(got[1].price, 35)
}

// ── where + order_by + first ──

[export, marker(no_coverage)]
def target_unroll5d_where_order_by_first_fold() : tuple<id : int; price : int; flag : int> {
    return _fold(from_decs_template(type<Unroll5dRow>)._where(_.flag == 1)._order_by(_.price).first())
}

[test]
def test_unroll5d_where_order_by_first_parity(t : T?) {
    fixture_unroll5d()
    // flag==1 rows: prices 30,50,40,35 → min = 30.
    t |> equal(target_unroll5d_where_order_by_first_fold().price, 30)
}

// ── reverse + to_array ──

[export, marker(no_coverage)]
def target_unroll5d_reverse_to_array_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>).reverse().to_array())
}

[test]
def test_unroll5d_reverse_to_array_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_reverse_to_array_fold()
    // Decs walk order is archetype-dependent. We don't assert exact order vs the eager bridge — we assert .reverse() produces the SAME multiset reversed wrt a non-reverse fold. Cheap check: length + presence of all ids.
    t |> equal(got.length(), 7)
    var idsSeen : table<int>
    for (r in got) {
        idsSeen |> insert(r.id)
    }
    for (i in range(7)) {
        t |> success(key_exists(idsSeen, i), "id {i} present")
    }
}

// ── reverse + take ──

[export, marker(no_coverage)]
def target_unroll5d_reverse_take_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>).reverse().take(3).to_array())
}

[test]
def test_unroll5d_reverse_take_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_reverse_take_fold()
    t |> equal(got.length(), 3, "reverse + take(3) yields 3 rows")
}

[test]
def test_unroll5d_reverse_take_beyond_length(t : T?) {
    fixture_unroll5d()
    let huge = _fold(from_decs_template(type<Unroll5dRow>).reverse().take(99).to_array())
    t |> equal(huge.length(), 7, "take(N>length) returns all rows reversed")
}

[test]
def test_unroll5d_reverse_take_zero(t : T?) {
    fixture_unroll5d()
    // Runtime-0 take expression: a const-folded `take(0)` literal trips PERF017 on the splice-emitted resize formula (`takeB < length(buf)` collapses to `0 < length(buf)`).
    var zeroN = 0  // nolint:LINT003 — kept as var to defeat const-fold of the take(N) arg
    let zero = _fold(from_decs_template(type<Unroll5dRow>).reverse().take(zeroN).to_array())
    t |> success(empty(zero), "take(0) returns empty")
}

// Side-effecting take(N) source — the splice must evaluate the take expression EXACTLY ONCE.
var g_take_call_count = 0
def bump_take_count() : int {
    g_take_call_count ++
    return 3
}

[test]
def test_unroll5d_reverse_take_single_eval(t : T?) {
    fixture_unroll5d()
    g_take_call_count = 0
    let got <- _fold(from_decs_template(type<Unroll5dRow>).reverse().take(bump_take_count()).to_array())
    t |> equal(got.length(), 3, "take(N) honored — 3 rows")
    t |> equal(g_take_call_count, 1, "take(N) expression evaluated exactly once (no multi-eval from splice clones)")
}

// ── reverse + count (identity — counter loop) ──

[export, marker(no_coverage)]
def target_unroll5d_reverse_count_fold() : int {
    return _fold(from_decs_template(type<Unroll5dRow>).reverse().count())
}

[test]
def test_unroll5d_reverse_count_parity(t : T?) {
    fixture_unroll5d()
    t |> equal(target_unroll5d_reverse_count_fold(), 7)
}

// ── reverse + first (= last of source) ──

[export, marker(no_coverage)]
def target_unroll5d_where_reverse_first_or_default_fold() : tuple<id : int; price : int; flag : int> {
    return _fold(from_decs_template(type<Unroll5dRow>)._where(_.flag == 1).reverse().first_or_default((id = -1, price = -99, flag = -1)))
}

[test]
def test_unroll5d_where_reverse_first_or_default_empty(t : T?) {
    restart()
    commit()
    t |> equal(target_unroll5d_where_reverse_first_or_default_fold().id, -1, "empty decs source → default")
}

[test]
def test_unroll5d_where_reverse_first_or_default_nonempty(t : T?) {
    fixture_unroll5d()
    // flag==1 ids = {0,2,4,6}; last-encountered (reverse-of-first) is the maximum encountered id in archetype walk order. We only assert it's one of the flag==1 ids.
    let id = target_unroll5d_where_reverse_first_or_default_fold().id
    t |> success(id == 0 || id == 2 || id == 4 || id == 6, "first-of-reverse(flag==1) is one of the matching ids")
}

// ── where + reverse + to_array ──

[export, marker(no_coverage)]
def target_unroll5d_where_reverse_to_array_fold() : array<tuple<id : int; price : int; flag : int>> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._where(_.flag == 1).reverse().to_array())
}

[test]
def test_unroll5d_where_reverse_to_array_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_where_reverse_to_array_fold()
    t |> equal(got.length(), 4)
    for (r in got) {
        t |> equal(r.flag, 1)
    }
}

// ── where + select + reverse + to_array (projection through reverse) ──

[export, marker(no_coverage)]
def target_unroll5d_where_select_reverse_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<Unroll5dRow>)._where(_.flag == 1)._select(_.id).reverse().to_array())
}

[test]
def test_unroll5d_where_select_reverse_to_array_parity(t : T?) {
    fixture_unroll5d()
    let got <- target_unroll5d_where_select_reverse_to_array_fold()
    t |> equal(got.length(), 4, "4 flag==1 rows projected to id")
    var idsSeen : table<int>
    for (id in got) {
        idsSeen |> insert(id)
    }
    for (expectedId in [0, 2, 4, 6]) {
        t |> success(key_exists(idsSeen, expectedId), "id {expectedId} present in projection")
    }
}

// ── AST shape gates ──

[test]
def test_unroll5d_order_by_take_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5d_order_by_take3_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "order_by+take splice must NOT fall to tier-2 to_sequence (cascade ordering trap)")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype (no eager-bridge wrapping)")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "order family uses for_each_archetype (no early-exit needed)")
        // PR C: emit_bounded_heap (unified array+decs) uses `order_buf` instead of decs_buf. Buffer still hoisted above for_each_archetype.
        t |> success(describe_count(body_expr, "order_buf") >= 2, "order_buf declared + populated")
        // Bounded-heap emit: spliced_push_heap appears twice (fill + replace), spliced_pop_heap once (replace). top_n_by is no longer used.
        t |> equal(describe_count(body_expr, "spliced_push_heap"), 2, "bounded-heap fill + replace each call spliced_push_heap")
        t |> equal(describe_count(body_expr, "spliced_pop_heap"), 1, "bounded-heap replace calls spliced_pop_heap once")
        t |> equal(describe_count(body_expr, "top_n_by"), 0, "bounded-heap path does NOT call top_n_by")
    }
}

[test]
def test_unroll5d_order_by_first_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5d_order_by_first_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "order_by+first splice must NOT fall to tier-2 to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
        // Streaming-min emit: best + seen state vars, no buffer + min_by/top_n.
        t |> equal(describe_count(body_expr, "min_by"), 0, "streaming-min path does NOT call min_by")
        t |> equal(describe_count(body_expr, "top_n_by"), 0, "streaming-min path does NOT call top_n_by")
        // PR C: emit_streaming_min uses `order_best` / `order_seen` (unified array+decs naming).
        t |> success(describe_count(body_expr, "order_best") >= 2, "order_best state var declared + updated")
        t |> success(describe_count(body_expr, "order_seen") >= 2, "order_seen flag declared + updated")
        // Empty-source panic guard preserved.
        t |> equal(describe_count(body_expr, "sequence contains no elements"), 1, "panic-on-empty guard for first()")
    }
}

[test]
def test_unroll5d_reverse_to_array_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5d_reverse_to_array_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "reverse splice must NOT fall to tier-2 to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
        t |> equal(describe_count(body_expr, "reverse_inplace"), 1, "buffer reversed via reverse_inplace")
    }
}

[test]
def test_unroll5d_reverse_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5d_reverse_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // reverse+count → counter loop, NO buffer, NO reverse_inplace.
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "reverse+count splice must NOT fall to tier-2 to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
        t |> equal(describe_count(body_expr, "reverse_inplace"), 0, "reverse is identity for count — no reverse_inplace")
        t |> equal(describe_count(body_expr, "decs_buf"), 0, "no buffer needed for count")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5c: distinct / distinct_by on decs (streaming dedup via hoisted table<unique_key>)
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "unroll5c_")]
struct Unroll5cRow {
    id    : int
    brand : int
    price : int
}

def private fixture_unroll5c() {
    restart()
    // 12 rows. brand cycles 0..3 (so 4 distinct brands). price = id*10 (12 distinct values).
    for (i in range(12)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll5c_id := i
            cmp.unroll5c_brand := i % 4
            cmp.unroll5c_price := i * 10
        }
    }
    commit()
}

// ── distinct |> to_array (bare select + distinct) ──

[export, marker(no_coverage)]
def target_unroll5c_select_distinct_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().to_array())
}

[test]
def test_unroll5c_select_distinct_to_array_parity(t : T?) {
    fixture_unroll5c()
    let got <- target_unroll5c_select_distinct_to_array_fold()
    t |> equal(got.length(), 4, "4 distinct brands (0..3)")
    // Brands cycle 0,1,2,3,0,1,2,3,0,1,2,3 → first-seen order is 0,1,2,3.
    let expected = [0, 1, 2, 3]
    for (i in range(4)) {
        t |> equal(got[i], expected[i])
    }
}

[test]
def test_unroll5c_select_distinct_to_array_empty(t : T?) {
    restart()
    commit()
    let got <- target_unroll5c_select_distinct_to_array_fold()
    t |> equal(got.length(), 0, "empty decs → empty distinct")
}

// ── distinct |> count ──

[export, marker(no_coverage)]
def target_unroll5c_select_distinct_count_fold() : int {
    return _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().count())
}

[test]
def test_unroll5c_select_distinct_count_parity(t : T?) {
    fixture_unroll5c()
    t |> equal(target_unroll5c_select_distinct_count_fold(), 4, "length(seen) = 4 distinct brands")
}

// ── distinct |> long_count ──

[export, marker(no_coverage)]
def target_unroll5c_select_distinct_long_count_fold() : int64 {
    return _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().long_count())
}

[test]
def test_unroll5c_select_distinct_long_count_parity(t : T?) {
    fixture_unroll5c()
    t |> equal(target_unroll5c_select_distinct_long_count_fold(), 4l, "int64(length(seen)) = 4")
}

// ── distinct |> sum ──

[export, marker(no_coverage)]
def target_unroll5c_select_distinct_sum_fold() : int {
    return _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().sum())
}

[test]
def test_unroll5c_select_distinct_sum_parity(t : T?) {
    fixture_unroll5c()
    // sum of distinct brands {0,1,2,3} = 6
    t |> equal(target_unroll5c_select_distinct_sum_fold(), 6, "acc folds at fresh-key site only")
}

// ── distinct |> take(N) |> to_array (streaming early-exit) ──

[export, marker(no_coverage)]
def target_unroll5c_select_distinct_take2_fold() : array<int> {
    return <- _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().take(2).to_array())
}

[test]
def test_unroll5c_select_distinct_take_parity(t : T?) {
    fixture_unroll5c()
    let got <- target_unroll5c_select_distinct_take2_fold()
    // First 2 distinct in source order: 0, 1.
    t |> equal(got.length(), 2)
    t |> equal(got[0], 0)
    t |> equal(got[1], 1)
}

[test]
def test_unroll5c_select_distinct_take_zero(t : T?) {
    fixture_unroll5c()
    let got <- _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().take(0).to_array())
    t |> equal(got.length(), 0, "take(0) yields empty")
}

[test]
def test_unroll5c_select_distinct_take_beyond(t : T?) {
    fixture_unroll5c()
    let got <- _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().take(99).to_array())
    t |> equal(got.length(), 4, "take(N>num_distinct) returns all distinct")
}

// ── where + distinct |> to_array ──

[export, marker(no_coverage)]
def target_unroll5c_where_distinct_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<Unroll5cRow>)._where(_.id < 8)._select(_.brand).distinct().to_array())
}

[test]
def test_unroll5c_where_distinct_parity(t : T?) {
    fixture_unroll5c()
    let got <- target_unroll5c_where_distinct_to_array_fold()
    // id<8 → ids 0..7 → brands 0,1,2,3,0,1,2,3 → distinct = 4.
    t |> equal(got.length(), 4)
}

// ── distinct_by(key) |> to_array ──

[export, marker(no_coverage)]
def target_unroll5c_distinct_by_to_array_fold() : array<tuple<id : int; brand : int; price : int>> {
    return <- _fold(from_decs_template(type<Unroll5cRow>)._distinct_by(_.brand).to_array())
}

[test]
def test_unroll5c_distinct_by_parity(t : T?) {
    fixture_unroll5c()
    let got <- target_unroll5c_distinct_by_to_array_fold()
    // First row per distinct brand: ids 0,1,2,3 (brands 0,1,2,3).
    t |> equal(got.length(), 4)
    for (i in range(4)) {
        t |> equal(got[i].id, i)
        t |> equal(got[i].brand, i)
    }
}

// ── distinct_by(key) |> count ──

[export, marker(no_coverage)]
def target_unroll5c_distinct_by_count_fold() : int {
    return _fold(from_decs_template(type<Unroll5cRow>)._distinct_by(_.brand).count())
}

[test]
def test_unroll5c_distinct_by_count_parity(t : T?) {
    fixture_unroll5c()
    t |> equal(target_unroll5c_distinct_by_count_fold(), 4)
}

// ── distinct_by(key) |> take(N) |> to_array ──

[export, marker(no_coverage)]
def target_unroll5c_distinct_by_take2_fold() : array<tuple<id : int; brand : int; price : int>> {
    return <- _fold(from_decs_template(type<Unroll5cRow>)._distinct_by(_.brand).take(2).to_array())
}

[test]
def test_unroll5c_distinct_by_take_parity(t : T?) {
    fixture_unroll5c()
    let got <- target_unroll5c_distinct_by_take2_fold()
    t |> equal(got.length(), 2)
    t |> equal(got[0].brand, 0)
    t |> equal(got[1].brand, 1)
}

// ── Side-effect single-eval guard: take(N) limit binds once, projection binds once per element ──

var g_unroll5c_take_arg_calls : int = 0
def trace_take_arg() : int {
    g_unroll5c_take_arg_calls ++
    return 2
}

[test]
def test_unroll5c_distinct_take_arg_evaluated_once(t : T?) {
    fixture_unroll5c()
    g_unroll5c_take_arg_calls = 0
    let got <- _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().take(trace_take_arg()).to_array())
    t |> equal(g_unroll5c_take_arg_calls, 1, "take(N) arg must evaluate exactly once at invoke entry, not per fresh-key check")
    t |> equal(got.length(), 2, "result still matches the bound N")
}

// ── AST shape gates ──

[test]
def test_unroll5c_select_distinct_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5c_select_distinct_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "distinct+count splice must NOT fall to tier-2 to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype (no _find — no take present)")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "no take → no _find")
        // PR C: emit_hashtable_dedup (unified array+decs) uses `buf` / `seen` (drop the `decs_` prefix).
        t |> equal(describe_count(body_expr, "`buf"), 0, "count terminator elides the buffer")
        t |> success(describe_count(body_expr, "`seen") >= 2, "seen-table declared + queried")
        t |> equal(describe_count(body_expr, "key_exists"), 1, "single fresh-key check per element")
    }
}

[test]
def test_unroll5c_select_distinct_take_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5c_select_distinct_take2_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "distinct+take splice must NOT fall to tier-2 to_sequence")
        // take present → outer iteration switches to _find so take-cap can stop across archetypes.
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take present → for_each_archetype_find")
        // PR C: emit_hashtable_dedup (unified array+decs) uses `buf` / `seen` / `taken`.
        t |> success(describe_count(body_expr, "`buf") >= 2, "buffer needed for to_array terminator")
        t |> success(describe_count(body_expr, "`seen") >= 2, "seen-table declared + queried")
        t |> success(describe_count(body_expr, "`taken") >= 2, "taken counter declared + incremented")
    }
}

[test]
def test_unroll5c_distinct_by_to_array_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5c_distinct_by_to_array_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "distinct_by splice must NOT fall to tier-2")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "no take → no _find")
        // The key lambda invocation is the distinct_by signature — invoke the key block to compute the key, then unique_key wraps it.
        t |> success(describe_count(body_expr, "unique_key") >= 1, "distinct_by routes key through unique_key")
    }
}

[test]
def test_unroll5c_select_distinct_sum_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5c_select_distinct_sum_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "distinct+sum splice must NOT fall to tier-2")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "no take → plain for_each_archetype")
        // PR C: emit_hashtable_dedup (unified array+decs) uses `buf` / `acc`.
        t |> equal(describe_count(body_expr, "`buf"), 0, "sum terminator elides the buffer")
        t |> success(describe_count(body_expr, "`acc") >= 2, "accumulator declared + folded")
    }
}

// ── Predicate-form terminator regression (Copilot review on PR #2785) — plan_decs_distinct bails on the predicate overload (arg-count guard at the terminator pop); plan_distinct must also bail (post-fix in the same PR). The chain must NOT silently drop the predicate — pre-fix this returned `length(seen)` ignoring the predicate. ──

[test]
def test_unroll5c_select_distinct_count_pred_parity(t : T?) {
    fixture_unroll5c()
    // brands cycle 0..3 (4 distinct). distinct |> count(>=2) → only brands 2 and 3 → 2.
    let got = _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().count($(x : int) => x >= 2))
    t |> equal(got, 2, "predicate must be honored, not dropped")
}

[test]
def test_unroll5c_select_distinct_long_count_pred_parity(t : T?) {
    fixture_unroll5c()
    let got = _fold(from_decs_template(type<Unroll5cRow>)._select(_.brand).distinct().long_count($(x : int) => x >= 2))
    t |> equal(got, 2l, "predicate must be honored, not dropped")
}

// ─────────────────────────────────────────────────────────────────────────────
// Wave 4 — chain-driven component pruning (build_decs_inner_for_pruned)
// Tests that emit_decs_* paths prune get_ro/iter slots down to fields the chain references,
// and that whole-tuple references (bare to_array, pass-to-user-fn) correctly disable pruning.
// ─────────────────────────────────────────────────────────────────────────────

[decs_template(prefix = "wave4_")]
struct Wave4Row {
    brand : int
    price : int
    year  : int
}

def private fixture_wave4(n : int) {
    restart()
    for (i in range(n)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.wave4_brand := i % 4
            cmp.wave4_price := i * 10
            cmp.wave4_year  := 2000 + (i % 25)
        }
    }
    commit()
}

[export, marker(no_coverage)]
def target_wave4_pruned_select_sum_fold() : int {
    return _fold(from_decs_template(type<Wave4Row>)._select(_.price).sum())
}

[test]
def test_wave4_pruned_select_sum_parity(t : T?) {
    fixture_wave4(10)
    // sum(price) for i in 0..9 = 10*(0+1+...+9) = 450
    t |> equal(target_wave4_pruned_select_sum_fold(), 450, "_select(_.price).sum() — only price referenced; pruning must preserve semantics")
}

[export, marker(no_coverage)]
def target_wave4_pruned_where_count_fold() : int {
    return _fold(from_decs_template(type<Wave4Row>)._where(_.price > 50).count())
}

[test]
def test_wave4_pruned_where_count_parity(t : T?) {
    fixture_wave4(10)
    // i=0..9 → price=0,10,20,...90; price>50 → i in {6,7,8,9} → 4 rows
    t |> equal(target_wave4_pruned_where_count_fold(), 4, "_where(_.price>50).count() — only price referenced; pruning must preserve semantics")
}

[export, marker(no_coverage)]
def target_wave4_pruned_select_where_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<Wave4Row>)._where(_.price > 50)._select(_.brand).to_array())
}

[test]
def test_wave4_pruned_select_where_to_array_parity(t : T?) {
    fixture_wave4(10)
    // Survivors: i=6,7,8,9 → brand = 6%4=2, 7%4=3, 8%4=0, 9%4=1
    let got <- target_wave4_pruned_select_where_to_array_fold()
    t |> equal(got |> length, 4, "4 surviving brands")
    t |> equal(got |> sum, 2 + 3 + 0 + 1, "brand sum independent of order")
}

[export, marker(no_coverage)]
def target_wave4_unpruned_bare_to_array_fold() : array<tuple<brand : int; price : int; year : int>> {
    return <- _fold(from_decs_template(type<Wave4Row>).to_array())
}

[test]
def test_wave4_unpruned_bare_to_array_parity(t : T?) {
    fixture_wave4(3)
    // Bare to_array: walker sees `push_clone(decs_tup)` whole-var ref → no pruning → full tuple preserved
    let got <- target_wave4_unpruned_bare_to_array_fold()
    t |> equal(got |> length, 3, "3 rows materialized")
    // Sum each field independently — order-agnostic full-tuple check
    var brand_sum = 0
    var price_sum = 0
    var year_sum = 0
    for (c in got) {
        brand_sum += c.brand
        price_sum += c.price
        year_sum += c.year
    }
    t |> equal(brand_sum, 0 + 1 + 2, "brand sum (full tuple preserved)")
    t |> equal(price_sum, 0 + 10 + 20, "price sum (full tuple preserved)")
    t |> equal(year_sum, 2000 + 2001 + 2002, "year sum (full tuple preserved)")
}

def private wave4_pred_uses_whole_tup(t : tuple<brand : int; price : int; year : int>) : bool {
    return (t.brand + t.price + t.year) > 0
}

[export, marker(no_coverage)]
def target_wave4_unpruned_pass_to_fn_fold() : int {
    return _fold(from_decs_template(type<Wave4Row>)._where(wave4_pred_uses_whole_tup(_)).count())
}

[test]
def test_wave4_unpruned_pass_to_fn_parity(t : T?) {
    fixture_wave4(3)
    // _where peel renames `_` to `decs_tup`; the predicate body becomes
    // `wave4_pred_uses_whole_tup(decs_tup)` — decs_tup referenced as a whole-var arg →
    // walker sets allFieldsUsed=true → pruning disabled. All 3 fields materialized.
    // All 3 rows pass the > 0 predicate (year always ≥ 2000), so count = 3.
    t |> equal(target_wave4_unpruned_pass_to_fn_fold(), 3, "pass-to-fn predicate must see full tuple")
}

[test]
def test_wave4_unpruned_pass_to_fn_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_unpruned_pass_to_fn_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // Whole-tup reference in the where predicate disables pruning — all 3 get_ros present.
        t |> equal(describe_count(body_expr, "get_ro"), 3, "pass-to-fn must emit all 3 get_ros (no pruning)")
    }
}

[test]
def test_wave4_pruned_where_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_pruned_where_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // Wave 4 contract: chain reads only `price`; helper prunes brand+year out of the multi-iter for-loop.
        t |> equal(describe_count(body_expr, "get_ro"), 1, "_where(_.price>50).count() must emit exactly 1 get_ro (price)")
        t |> success(describe_count(body_expr, "wave4_price") >= 1, "kept slot is for price")
        t |> equal(describe_count(body_expr, "wave4_brand"), 0, "brand pruned out")
        t |> equal(describe_count(body_expr, "wave4_year"), 0, "year pruned out")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype (no _find — no take/early-exit)")
    }
}

[test]
def test_wave4_unpruned_bare_to_array_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_unpruned_bare_to_array_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // Whole-tup-rewriter contract: push_clone(decs_tup) → push_clone((brand=wave4_brand, price=wave4_price, year=wave4_year)) — all 3 iter vars survive so the pruner keeps all 3 slots, but the named-tuple bind is elided (no per-iter `var decs_tup = ...`). Test name says "unpruned" historically — slot pruning still doesn't fire, but bind elision does.
        t |> equal(describe_count(body_expr, "get_ro"), 3, "bare to_array must emit all 3 get_ros (no slot pruning)")
        t |> equal(describe_count(body_expr, "decs_tup"), 0, "named-tuple bind elided — whole-var rewriter inlined (brand=iter,...) literal")
        t |> success(describe_count(body_expr, "wave4_brand") >= 1, "brand slot present")
        t |> success(describe_count(body_expr, "wave4_price") >= 1, "price slot present")
        t |> success(describe_count(body_expr, "wave4_year") >= 1, "year slot present")
    }
}

[export, marker(no_coverage)]
def target_wave4_take_count_no_field_ref_fold() : int {
    return _fold(from_decs_template(type<Wave4Row>).take(5).count())
}

[test]
def test_wave4_take_count_no_field_ref_parity(t : T?) {
    fixture_wave4(10)
    // .take(5).count() — the per-element body only references the take counter + acc; no
    // tuple field refs. Walker reports empty usedNames → helper short-circuits to unpruned
    // emission (still safe; no field referenced → no real difference).
    t |> equal(target_wave4_take_count_no_field_ref_fold(), 5, "take(5).count() splice must still return 5")
}

[export, marker(no_coverage)]
def target_wave4_all_fields_via_access_fold() : int {
    // All 3 fields referenced via field access (chained single-field where_s — same all-fields trigger as a compound predicate, but each peeled body has a single field-access node, matching the predicate shape the rest of the suite uses).
    return _fold(from_decs_template(type<Wave4Row>)
        ._where(_.brand >= 0)
        ._where(_.price >= 0)
        ._where(_.year > 0)
        .count())
}

[test]
def test_wave4_all_fields_via_access_parity(t : T?) {
    fixture_wave4(10)
    // brand = i%4 (always >= 0), price = i*10 (always >= 0), year = 2000+(i%25) (always > 0) → all 10 rows match.
    t |> equal(target_wave4_all_fields_via_access_fold(), 10, "all-fields-access predicate must return all rows")
}

[test]
def test_wave4_all_fields_via_access_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_all_fields_via_access_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // No slots pruned (all 3 fields read), but the bind is elided and body references iter vars directly.
        t |> equal(describe_count(body_expr, "get_ro"), 3, "all 3 get_ros present (no slot pruning)")
        t |> equal(describe_count(body_expr, "decs_tup"), 0, "named-tuple bind elided when no whole-var ref")
        t |> success(describe_count(body_expr, "wave4_brand") >= 2, "brand iter var read directly (slot + body ref)")
        t |> success(describe_count(body_expr, "wave4_price") >= 2, "price iter var read directly")
        t |> success(describe_count(body_expr, "wave4_year") >= 2, "year iter var read directly")
    }
}

// ── C2: buffer-required planners (order/reverse/distinct) ──────────────────────

[export, marker(no_coverage)]
def target_wave4_unpruned_reverse_to_array_fold() : array<tuple<brand : int; price : int; year : int>> {
    return <- _fold(from_decs_template(type<Wave4Row>).reverse().to_array())
}

[test]
def test_wave4_unpruned_reverse_to_array_parity(t : T?) {
    fixture_wave4(3)
    // reverse + to_array (no _select): buffer holds full tuples → push_clone(decs_tup) →
    // walker sees whole-var → pruning disabled. All 3 fields preserved.
    let got <- target_wave4_unpruned_reverse_to_array_fold()
    t |> equal(got |> length, 3, "3 rows after reverse")
    var brand_sum = 0
    var price_sum = 0
    var year_sum = 0
    for (c in got) {
        brand_sum += c.brand
        price_sum += c.price
        year_sum += c.year
    }
    t |> equal(brand_sum, 0 + 1 + 2, "brand sum preserved (reverse + bare to_array)")
    t |> equal(price_sum, 0 + 10 + 20, "price sum preserved")
    t |> equal(year_sum, 2000 + 2001 + 2002, "year sum preserved")
}

[export, marker(no_coverage)]
def target_wave4_unpruned_distinct_to_array_fold() : array<tuple<brand : int; price : int; year : int>> {
    return <- _fold(from_decs_template(type<Wave4Row>).distinct().to_array())
}

[test]
def test_wave4_unpruned_distinct_to_array_parity(t : T?) {
    fixture_wave4(3)
    // distinct + to_array (no _select): each row distinct by full-tuple → push_clone(decs_tup) →
    // walker sees whole-var → pruning disabled.
    let got <- target_wave4_unpruned_distinct_to_array_fold()
    t |> equal(got |> length, 3, "3 distinct rows preserved as full tuples")
}

[export, marker(no_coverage)]
def target_wave4_pruned_select_reverse_to_array_fold() : array<int> {
    return <- _fold(from_decs_template(type<Wave4Row>)._select(_.brand).reverse().to_array())
}

[test]
def test_wave4_pruned_select_reverse_to_array_parity(t : T?) {
    fixture_wave4(4)
    // _select(_.brand).reverse().to_array() — chain rebinds to a scalar; the reverse buffer
    // holds projected brands (not the full tuple). Pruning safe — only brand referenced.
    // Brands for i=0..3 are 0,1,2,3; reverse order = 3,2,1,0; sum = 6.
    let got <- target_wave4_pruned_select_reverse_to_array_fold()
    t |> equal(got |> length, 4, "4 brands after reverse")
    t |> equal(got |> sum, 6, "sum preserved across projection + reverse")
}

[export, marker(no_coverage)]
def target_wave4_pruned_select_distinct_count_fold() : int {
    return _fold(from_decs_template(type<Wave4Row>)._select(_.brand).distinct().count())
}

[test]
def test_wave4_pruned_select_distinct_count_parity(t : T?) {
    fixture_wave4(10)
    // _select(_.brand).distinct().count() — brand cycles 0..3 (4 distinct values) across 10 rows.
    // Chain references only brand → walker collects {brand} → pruning fires → only get_ro("wave4_brand").
    t |> equal(target_wave4_pruned_select_distinct_count_fold(), 4, "4 distinct brand values")
}

[test]
def test_wave4_pruned_select_distinct_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_pruned_select_distinct_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // Pruned via C2 (plan_decs_distinct path): only brand referenced.
        t |> equal(describe_count(body_expr, "get_ro"), 1, "distinct.count chain emits exactly 1 get_ro")
        t |> success(describe_count(body_expr, "wave4_brand") >= 1, "kept slot is brand")
        t |> equal(describe_count(body_expr, "wave4_price"), 0, "price pruned out")
        t |> equal(describe_count(body_expr, "wave4_year"), 0, "year pruned out")
    }
}

// ── C3: group_by via shared adapter_emit_source_loop ───────────────────────────

[export, marker(no_coverage)]
def target_wave4_pruned_groupby_count_fold() : array<tuple<Brand : int; N : int>> {
    return <- _fold(from_decs_template(type<Wave4Row>)._group_by(_.brand)._select((Brand = _._0, N = _._1 |> length)).to_array())
}

[test]
def test_wave4_pruned_groupby_count_parity(t : T?) {
    fixture_wave4(10)
    // brand cycles 0..3 across 10 rows → 4 distinct buckets with sizes [3,3,2,2].
    let got <- target_wave4_pruned_groupby_count_fold()
    t |> equal(got |> length, 4, "4 distinct brand buckets")
    var total = 0
    for (b in got) {
        total += b.N
    }
    t |> equal(total, 10, "buckets sum to source count")
}

[export, marker(no_coverage)]
def target_wave4_pruned_groupby_sum_fold() : array<tuple<Brand : int; S : int>> {
    return <- _fold(from_decs_template(type<Wave4Row>)._group_by(_.brand)._select((Brand = _._0, S = _._1 |> _select(_.price) |> sum)).to_array())
}

[test]
def test_wave4_pruned_groupby_sum_parity(t : T?) {
    fixture_wave4(10)
    // _group_by_lazy(_.brand) + inner-select-sum(_.price). Same safety rule as the count case:
    // plan_group_by_core bucket-build pushes the whole `decs_tup` → walker disables pruning →
    // all 3 get_ros emit, even though the chain only reads brand + price.
    let got <- target_wave4_pruned_groupby_sum_fold()
    t |> equal(got |> length, 4, "4 distinct brand buckets")
    // Sum of all prices across all buckets = 10*(0+1+...+9) = 450 — fixture independent of bucket layout.
    var total = 0
    for (b in got) {
        total += b.S
    }
    t |> equal(total, 450, "all-bucket price sum = full-table price sum")
}

[test]
def test_wave4_pruned_groupby_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_wave4_pruned_groupby_count_fold)
        t |> success(func != null, "RTTI must resolve target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch must capture body")
        // plan_group_by_core peels the key block (`_.brand` → `decs_tup.brand`) inline; the count-by-group shape avoids materializing the bucket — only `brand` survives in the body, so pruning drops `price`+`year`.
        t |> equal(describe_count(body_expr, "get_ro"), 1, "count-by-group reads only brand → pruning keeps 1 get_ro")
        t |> success(describe_count(body_expr, "wave4_brand") >= 1, "brand slot present")
        t |> equal(describe_count(body_expr, "wave4_price"), 0, "price pruned out")
        t |> equal(describe_count(body_expr, "wave4_year"), 0, "year pruned out")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// from_decs($(args){...}) lambda-arg form — splice + pruning coverage.
//
// Both from_decs (lambda-arg) and from_decs_template (type<T>) expand to the same
// invoke-block shape: `invoke($() { var res : array<tuple>; query(...); return
// res.to_sequence() })`. extract_decs_bridge pattern-matches the expanded shape, not
// the call macro, so it recognizes both. This block proves that empirically: every
// chain shape the type-arg form supports also works on the lambda-arg form, with the
// same emitted AST and the same Wave 4 component pruning.
//
// Tests are intentionally lean — the type-arg form is already covered exhaustively
// above; here we hit one representative case per planner family + the pruning gate.
// ─────────────────────────────────────────────────────────────────────────────

// ----- Parity: lambda-arg form returns the same answer as the type-arg form -----

[export, marker(no_coverage)]
def target_lambda_count_fold() : int {
    return _fold(from_decs($(unroll_val : int) {}).count())
}

[export, marker(no_coverage)]
def target_lambda_where_count_fold() : int {
    return _fold(from_decs($(unroll_val : int) {})._where(_.unroll_val >= 3).count())
}

[export, marker(no_coverage)]
def target_lambda_select_sum_fold() : int {
    return _fold(from_decs($(unroll_val : int) {})._select(_.unroll_val).sum())
}

[export, marker(no_coverage)]
def target_lambda_take_count_fold() : int {
    return _fold(from_decs($(unroll_val : int) {}).take(3).count())
}

[export, marker(no_coverage)]
def target_lambda_distinct_count_fold() : int {
    return _fold(from_decs($(unroll_val : int) {})._select(_.unroll_val).distinct().count())
}

[export, marker(no_coverage)]
def target_lambda_to_array_fold() : array<tuple<unroll_val : int>> {
    return <- _fold(from_decs($(unroll_val : int) {}).to_array())
}

[export, marker(no_coverage)]
def target_lambda_groupby_count_fold() : array<tuple<Bucket : int; N : int>> {
    return <- _fold(from_decs($(unroll_val : int) {})._group_by(_.unroll_val % 3)._select((Bucket = _._0, N = _._1 |> length)).to_array())
}

def private populate_unroll(n : int) {
    restart()
    for (i in range(n)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.unroll_val := i
        }
    }
    commit()
}

[test]
def test_lambda_count_parity(t : T?) {
    populate_unroll(7)
    t |> equal(target_lambda_count_fold(), 7, "lambda-arg count matches type-arg")
}

[test]
def test_lambda_where_count_parity(t : T?) {
    populate_unroll(7)
    // vals 0..6, _where(_>=3) → 3,4,5,6 → 4 matches
    t |> equal(target_lambda_where_count_fold(), 4, "lambda-arg where+count matches type-arg")
}

[test]
def test_lambda_select_sum_parity(t : T?) {
    populate_unroll(7)
    // sum(0..6) = 21
    t |> equal(target_lambda_select_sum_fold(), 21, "lambda-arg select+sum matches type-arg")
}

[test]
def test_lambda_take_count_parity(t : T?) {
    populate_unroll(7)
    // take(3).count() — early-exit at 3, regardless of source size
    t |> equal(target_lambda_take_count_fold(), 3, "lambda-arg take+count matches type-arg")
}

[test]
def test_lambda_distinct_count_parity(t : T?) {
    populate_unroll(7)
    // 7 unique vals → 7 distinct
    t |> equal(target_lambda_distinct_count_fold(), 7, "lambda-arg select+distinct+count matches type-arg")
}

[test]
def test_lambda_to_array_parity(t : T?) {
    populate_unroll(5)
    let got <- target_lambda_to_array_fold()
    t |> equal(got |> length, 5, "lambda-arg to_array preserves whole tuple")
    var sum = 0
    for (item in got) {
        sum += item.unroll_val
    }
    t |> equal(sum, 0 + 1 + 2 + 3 + 4, "lambda-arg to_array element values match")
}

[test]
def test_lambda_groupby_count_parity(t : T?) {
    populate_unroll(10)
    // vals 0..9, key = val % 3 → buckets {0:[0,3,6,9], 1:[1,4,7], 2:[2,5,8]} → 3 buckets sized 4,3,3
    let got <- target_lambda_groupby_count_fold()
    t |> equal(got |> length, 3, "lambda-arg group_by yields 3 distinct keys")
    var total = 0
    for (b in got) {
        total += b.N
    }
    t |> equal(total, 10, "lambda-arg group_by buckets sum to source count")
}

// ----- AST splice-shape gates: lambda-arg emits the same shape as type-arg -----

[test]
def test_lambda_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_lambda_count_fold)
        t |> success(func != null, "RTTI resolves count target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch captures body")
        // bare count goes through the arch.size shortcut: one for_each_archetype, no inner
        // per-entity for-loop, no eager-bridge to_sequence. Identical to the type-arg form.
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "lambda-arg count emits one for_each_archetype")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "lambda-arg count does NOT eager-bridge")
        t |> equal(count_expr_for(body_expr), 0, "lambda-arg count has no inner per-entity for")
        t |> equal(describe_count(body_expr, ".size"), 1, "lambda-arg count reads arch.size")
    }
}

[test]
def test_lambda_where_count_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_lambda_where_count_fold)
        t |> success(func != null, "RTTI resolves where+count target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch captures body")
        // where+count emits one for_each_archetype with an inner multi-iter for-loop;
        // the predicate's matching counter accumulates inline. NO to_sequence.
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "lambda-arg where+count emits one for_each_archetype")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "lambda-arg where+count does NOT eager-bridge")
    }
}

// ----- Wave 4 component pruning fires on lambda-arg form too -----

[export, marker(no_coverage)]
def target_lambda_pruned_select_sum_fold() : int {
    // Multi-component query, but chain only references `wave4_price`. Pruning must
    // drop wave4_brand + wave4_year get_ros to match the type-arg pruning behavior.
    return _fold(from_decs($(wave4_brand : int; wave4_price : int; wave4_year : int) {})._select(_.wave4_price).sum())
}

[test]
def test_lambda_pruned_select_sum_parity(t : T?) {
    fixture_wave4(10)
    // i*10 for i in 0..9 = 0+10+20+...+90 = 450
    t |> equal(target_lambda_pruned_select_sum_fold(), 450, "lambda-arg pruning preserves sum semantics")
}

[test]
def test_lambda_pruned_select_sum_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_lambda_pruned_select_sum_fold)
        t |> success(func != null, "RTTI resolves pruned target")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched, "qmatch captures body")
        // Lambda-arg declared 3 components but chain reads only wave4_price → pruning
        // keeps a single get_ro slot. Same gate the type-arg Wave 4 tests assert.
        t |> equal(describe_count(body_expr, "get_ro"), 1, "lambda-arg pruning drops unused get_ros (3 declared → 1 emitted)")
        t |> success(describe_count(body_expr, "wave4_price") >= 1, "wave4_price retained")
        t |> equal(describe_count(body_expr, "wave4_brand"), 0, "wave4_brand pruned")
        t |> equal(describe_count(body_expr, "wave4_year"), 0, "wave4_year pruned")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 5f: walk terminators (last / single / aggregate) + element_at
//          — the 4 outliers from PR #2812's m4 bench snapshot
// ─────────────────────────────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_unroll_last_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).last())
}

[export, marker(no_coverage)]
def target_unroll_last_or_default_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.val > 1000)._select(_.val).last_or_default(-9))
}

[export, marker(no_coverage)]
def target_unroll_single_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.val == 3)._select(_.val).single())
}

[export, marker(no_coverage)]
def target_unroll_single_or_default_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.val > 1000)._select(_.val).single_or_default(-7))
}

[export, marker(no_coverage)]
def target_unroll_aggregate_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).aggregate(0) <| $(a, x) => a + x * x)
}

[export, marker(no_coverage)]
def target_unroll_element_at_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).element_at(3))
}

[export, marker(no_coverage)]
def target_unroll_element_at_or_default_fold() : int {
    // element_at_or_default returns default<int> (0) on out-of-range; no user-supplied default.
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).element_at_or_default(999))
}

[test]
def test_unroll_last_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0..4 → last is 4
    t |> equal(target_unroll_last_fold(), 4, "last splice parity")
}

[test]
def test_unroll_last_or_default_empty(t : T?) {
    fixture_unroll2(5)
    // val>1000 → none → default -9
    t |> equal(target_unroll_last_or_default_fold(), -9, "last_or_default empty source")
}

[test]
def test_unroll_single_parity(t : T?) {
    fixture_unroll2(5)
    // val==3 matches exactly once → 3
    t |> equal(target_unroll_single_fold(), 3, "single splice parity")
}

[test]
def test_unroll_single_or_default_empty(t : T?) {
    fixture_unroll2(5)
    // val>1000 → none → default -7
    t |> equal(target_unroll_single_or_default_fold(), -7, "single_or_default empty source")
}

[export, marker(no_coverage)]
def target_unroll_single_or_default_multi_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.val < 100)._select(_.val).single_or_default(-7))
}

[test]
def test_unroll_single_or_default_multi(t : T?) {
    fixture_unroll2(5)
    // 5 rows match → single_or_default returns default on 2nd match
    t |> equal(target_unroll_single_or_default_multi_fold(), -7, "single_or_default returns default when >1 match")
}

[test]
def test_unroll_aggregate_parity(t : T?) {
    fixture_unroll2(5)
    // 0^2 + 1^2 + 2^2 + 3^2 + 4^2 = 30
    t |> equal(target_unroll_aggregate_fold(), 30, "aggregate sum-of-squares splice parity")
}

[test]
def test_unroll_aggregate_empty(t : T?) {
    fixture_unroll2(0)
    // empty source → returns seed
    t |> equal(target_unroll_aggregate_fold(), 0, "aggregate empty returns seed")
}

[test]
def test_unroll_element_at_parity(t : T?) {
    fixture_unroll2(5)
    // vals 0..4, element_at(3) → 3
    t |> equal(target_unroll_element_at_fold(), 3, "element_at splice parity")
}

[test]
def test_unroll_element_at_or_default_out_of_range(t : T?) {
    fixture_unroll2(5)
    // idx 999 way past end → default<int> == 0
    t |> equal(target_unroll_element_at_or_default_fold(), 0, "element_at_or_default out-of-range → default<int>")
}

// Range interaction: take + last / take + aggregate
[export, marker(no_coverage)]
def target_unroll_take_last_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3).last())
}

[test]
def test_unroll_take_last_parity(t : T?) {
    fixture_unroll2(10)
    // take(3) of 0..9 → 0,1,2 → last is 2
    t |> equal(target_unroll_take_last_fold(), 2, "take+last interaction")
}

[export, marker(no_coverage)]
def target_unroll_where_aggregate_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._where(_.flag == 1)._select(_.val).aggregate(0) <| $(a, x) => a + x)
}

[test]
def test_unroll_where_aggregate_parity(t : T?) {
    fixture_unroll2(5)
    // flag==1 rows: vals 1,3 → 1+3 = 4
    t |> equal(target_unroll_where_aggregate_fold(), 4, "where+aggregate interaction")
}

// Splice-shape gates: emit must not hit to_sequence; exact for_each_archetype{,_find} dispatch
[test]
def test_unroll_last_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_last_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_last_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "last splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "last (no range) uses plain for_each_archetype")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "last (no range) does NOT need _find")
    }
}

[test]
def test_unroll_single_or_default_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_single_or_default_multi_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_single_or_default_multi_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "single_or_default splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "single_or_default uses _find for early-exit on 2nd match")
    }
}

[test]
def test_unroll_aggregate_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_aggregate_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_aggregate_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "aggregate splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "aggregate (no range) uses plain for_each_archetype")
    }
}

[test]
def test_unroll_element_at_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_element_at_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_element_at_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "element_at splice must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "element_at uses _find for counter early-exit")
    }
}

[test]
def test_unroll_take_last_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_last_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_take_last_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "take+last splice must NOT call to_sequence")
        // take present → bool-lambda dispatch (for_each_archetype_find) so the range's `return true` exits the walk
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take+last uses _find when range present")
    }
}

// Range × walk/element_at combos: take present upgrades the otherwise-Each walk ops (single / aggregate)
// to the _find dispatch (the range guard's `return true` only works in a bool-lambda), and caps the
// counter-driven element_at. take+last covers `last`; these cover the rest of the migrated terminators.
[export, marker(no_coverage)]
def target_unroll_take_single_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(1).single())
}

[test]
def test_unroll_take_single_parity(t : T?) {
    fixture_unroll2(5)
    // take(1) of 0..4 → exactly [0] → single returns 0 (no >1-match panic)
    t |> equal(target_unroll_take_single_fold(), 0, "take+single returns the sole windowed element")
}

[test]
def test_unroll_take_single_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_single_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_take_single_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take upgrades single from Each to _find")
    }
}

[export, marker(no_coverage)]
def target_unroll_take_aggregate_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3).aggregate(0) <| $(a, x) => a + x)
}

[test]
def test_unroll_take_aggregate_parity(t : T?) {
    fixture_unroll2(10)
    // take(3) of 0..9 → 0,1,2 → 0+0+1+2 = 3
    t |> equal(target_unroll_take_aggregate_fold(), 3, "take+aggregate folds the windowed elements")
}

[test]
def test_unroll_take_aggregate_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_take_aggregate_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_take_aggregate_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "take upgrades aggregate from Each to _find")
    }
}

[export, marker(no_coverage)]
def target_unroll_take_element_at_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(3).element_at(1))
}

[test]
def test_unroll_take_element_at_parity(t : T?) {
    fixture_unroll2(10)
    // take(3) → window 0,1,2; element_at(1) → 1
    t |> equal(target_unroll_take_element_at_fold(), 1, "take+element_at indexes inside the window")
}

[export, marker(no_coverage)]
def target_unroll_take_element_at_or_default_oor_fold() : int {
    // idx 5 is past the take(2) window [0,1] → default<int> == 0
    return _fold(from_decs_template(type<UnrollChainRow>)._select(_.val).take(2).element_at_or_default(5))
}

[test]
def test_unroll_take_element_at_or_default_out_of_window(t : T?) {
    fixture_unroll2(10)
    t |> equal(target_unroll_take_element_at_or_default_oor_fold(), 0, "take+element_at_or_default past window → default")
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 1: trivial-let elision for `_select(_.userName)` → iter var (skips
//          the `let decs_sel_N = car_price` no-op binding when the projection
//          is a single field access on decs_tup)
// ─────────────────────────────────────────────────────────────────────────────

[test]
def test_unroll_select_sum_trivial_let_elision_splice_shape(t : T?) {
    // `_select(_.val).sum()` peels to a single ExprField(decs_tup.val) which the flattener would rewrite to a bare iter var anyway — slice 1 elides the synthetic `let decs_sel_N = car_price` entirely and rewrites finalBind to the iter var, leaving body as `acc += <iterVar>` with no intermediate binding.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_select_sum_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_select_sum_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "decs_sel"), 0, "trivial single-field _select elided — no synthetic decs_sel binding")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "still exactly one for_each_archetype walk")
    }
}

[test]
def test_unroll_where_select_sum_trivial_let_elision_splice_shape(t : T?) {
    // Same elision applies through a where filter: where(_.flag==1).select(_.val).sum() — the select still peels to a single ExprField on decs_tup, so elision fires and the binding is dropped. Asserts the elision is not gated on whether the chain is bare or filtered.
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll_where_select_sum_fold)
        t |> success(func != null, "RTTI must resolve target_unroll_where_select_sum_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "decs_sel"), 0, "trivial single-field _select elided after a where_")
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slice 2: reverse + take(N) skip-into-tail fast path on decs
//          Two-pass: sum arch.size, then for_each_archetype_find skips whole
//          archetypes that fit below the threshold + early-exits once the
//          takeN-sized buffer is full. Reverses the small N buffer at end.
// ─────────────────────────────────────────────────────────────────────────────

[test]
def test_unroll5d_reverse_take_skip_into_tail_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_unroll5d_reverse_take_fold)
        t |> success(func != null, "RTTI must resolve target_unroll5d_reverse_take_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 1, "skip-into-tail uses _find for early-exit once buf reaches takeN")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 2, "two walks total: 1 size-sum + 1 _find (describe_count is substring match)")
        // decs_skips is the slice-2 unique skip-counter local (qn(\"decs_skips\", at)). Its presence proves the fast path fired vs. the legacy buffer+reverse_inplace+resize emit.
        t |> success(describe_count(body_expr, "decs_skips") >= 1, "decs_skips local present — slice-2 fast path fired")
    }
}

// ── Multi-archetype reverse + take: exercises whole-archetype-skip arm + partial-archetype skip-counter arm ──

[decs_template(prefix = "rev2_")]
struct RevTakeMultiArchA {
    id : int
}

[decs_template(prefix = "rev2_b_")]
struct RevTakeMultiArchB {
    bid  : int
    bval : int
}

[test]
def test_reverse_take_multi_archetype_parity(t : T?) {
    // Creates two MATCHING archetypes — both have `rev2_id` so both satisfy `from_decs_template(type<RevTakeMultiArchA>)`. Group A1: just rev2_id. Group A2: rev2_id + extra rev2_b_* components — same query matches, but the extra components land it in a separate archetype. With A1=4 + A2=5 → totalCount=9, take(3) → skip=6: A1 (size 4) skipped entirely via the size-sum arm, A2 enters with skipsLeft=2 → drains 2 then pushes 3 then returns true. Exercises both the whole-archetype-skip and partial-archetype skip-counter + early-exit.
    restart()
    for (i in 0..4) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.rev2_id := i
        }
    }
    for (i in 0..5) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.rev2_id := i + 100
            cmp.rev2_b_bid := i + 1000
            cmp.rev2_b_bval := i * 10
        }
    }
    commit()
    let got <- _fold(from_decs_template(type<RevTakeMultiArchA>).reverse().take(3).to_array())
    t |> equal(got.length(), 3, "reverse+take(3) on two matching archetypes returns 3 rows")
    var idSet : table<int>
    for (r in got) {
        idSet |> insert(r.id)
    }
    t |> equal(length(idSet), 3, "all 3 ids are distinct")
    // All returned ids must be from one of the two matching archetypes: A1 (ids 0..3) or A2 (ids 100..104). The exact subset depends on archetype iteration order (a decs-internal detail), so we just check membership.
    for (r in got) {
        let inA1 = r.id >= 0 && r.id < 4
        let inA2 = r.id >= 100 && r.id < 105
        t |> success(inA1 || inA2, "row id {r.id} must be from one of the two matching archetypes")
    }
}

[test]
def test_reverse_take_skip_zero_when_take_exceeds_total(t : T?) {
    // takeN > totalCount → skip = 0, returns all rows reversed. Exercises the actualTake = totalCount branch and the early-exit not firing (buf never reaches takeN before iteration completes).
    restart()
    for (i in 0..3) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.rev2_id := i
        }
    }
    commit()
    let got <- _fold(from_decs_template(type<RevTakeMultiArchA>).reverse().take(99).to_array())
    t |> equal(got.length(), 3, "take(N) where N > total returns all rows")
}

[test]
def test_reverse_take_empty_source(t : T?) {
    // No matching archetypes → totalCount = 0 → early-return empty buf before for_each_archetype_find.
    restart()
    commit()
    let got <- _fold(from_decs_template(type<RevTakeMultiArchA>).reverse().take(3).to_array())
    t |> success(empty(got), "empty source returns empty")
}

// ── Session 3: plan_decs_join — hashed equi-join over two from_decs_template eager bridges ──

[decs_template(prefix = "jt_car_")]
struct JoinTestCar {
    id        : int
    dealer_id : int
}

[decs_template(prefix = "jt_dealer_")]
struct JoinTestDealer {
    id   : int
    name : string
}

def private join_test_fixture(numCars, numDealers : int) {
    restart()
    create_entities(numCars) $(_eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, JoinTestCar(id = i + 100, dealer_id = (i % max(numDealers, 1)) + 1))
    }
    create_entities(numDealers) $(_eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, JoinTestDealer(id = i + 1, name = "D{i}"))
    }
}

[test]
def test_decs_join_count_basic(t : T?) {
    // 10 cars × 5 dealers, each car matches exactly one dealer.
    join_test_fixture(10, 5)
    let c = _fold(from_decs_template(type<JoinTestCar>) |> _join(from_decs_template(type<JoinTestDealer>),
                                                                  $(l, r) => l.dealer_id == r.id,
                                                                  $(l, r) => (l.id, r.name))
                                                         |> count())
    t |> equal(c, 10, "every car matches exactly one dealer")
}

[test]
def test_decs_join_count_unmatched_cars(t : T?) {
    // 5 cars with dealer_id 1..5, only 3 dealers (ids 1..3) — cars whose dealer_id is 4 or 5 (the 4th and 5th car) are unmatched, so 3 of 5 cars contribute.
    restart()
    create_entities(5) $(_eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, JoinTestCar(id = i + 100, dealer_id = i + 1))
    }
    create_entities(3) $(_eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, JoinTestDealer(id = i + 1, name = "D{i}"))
    }
    let c = _fold(from_decs_template(type<JoinTestCar>) |> _join(from_decs_template(type<JoinTestDealer>),
                                                                  $(l, r) => l.dealer_id == r.id,
                                                                  $(l, r) => (l.id, r.name))
                                                         |> count())
    t |> equal(c, 3, "only cars with matching dealer ids contribute")
}

[test]
def test_decs_join_empty_sides(t : T?) {
    restart()
    let c = _fold(from_decs_template(type<JoinTestCar>) |> _join(from_decs_template(type<JoinTestDealer>),
                                                                  $(l, r) => l.dealer_id == r.id,
                                                                  $(l, r) => (l.id, r.name))
                                                         |> count())
    t |> equal(c, 0, "no entities → 0 pairs")
}

[test]
def test_decs_join_many_to_one(t : T?) {
    // 10 cars distributed across 2 dealers — each car matches one dealer; counts 10.
    join_test_fixture(10, 2)
    let c = _fold(from_decs_template(type<JoinTestCar>) |> _join(from_decs_template(type<JoinTestDealer>),
                                                                  $(l, r) => l.dealer_id == r.id,
                                                                  $(l, r) => (l.id, r.name))
                                                         |> count())
    t |> equal(c, 10, "many-cars-to-one-dealer-each yields one pair per car")
}

[test]
def test_decs_join_to_array_projection(t : T?) {
    // to_array path: named-tuple projection picks up CarId / DealerName.
    join_test_fixture(6, 3)
    let arr <- _fold(from_decs_template(type<JoinTestCar>) |> _join(from_decs_template(type<JoinTestDealer>),
                                                                     $(l, r) => l.dealer_id == r.id,
                                                                     $(l, r) => (CarId = l.id, DealerName = r.name))
                                                            |> to_array())
    t |> equal(length(arr), 6, "every car matched")
    // Each row's CarId is 100..105, DealerName is D0..D2 in round-robin order. Order isn't guaranteed across archetypes, so check the set.
    var seen : table<int>
    for (row in arr) {
        seen |> insert(row.CarId)
        t |> success(row.DealerName |> starts_with("D"), "dealer name prefixed D")
    }
    t |> equal(length(seen), 6, "all 6 distinct car ids present")
}

// ─────────────────────────────────────────────────────────────────────────────
// Random-index tail fast path: BARE last() / last_or_default() (no where/select)
// over indexable decs sources read the last archetype's [size-1] directly instead
// of walking all N. (Predicated / _select'd last keeps the walk — Slice 5f above.)
// reverse|>take's random-index branch is exercised by the Unroll5dRow / RevTakeMultiArch
// tests; here we add the get_default_ro iterator-source FALLBACK those don't reach.
// ─────────────────────────────────────────────────────────────────────────────

typedef UnrollTuple = tuple<val : int; flag : int>

[export, marker(no_coverage)]
def target_bare_last_fold() : UnrollTuple {
    return _fold(from_decs_template(type<UnrollChainRow>).last())
}

[export, marker(no_coverage)]
def target_bare_last_cascade() : UnrollTuple {
    return from_decs_template(type<UnrollChainRow>) |> last()
}

[export, marker(no_coverage)]
def target_bare_last_or_default_fold() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).last_or_default((val = -1, flag = -1))).val
}

[test]
def test_bare_last_parity(t : T?) {
    fixture_unroll2(7)
    let fast = target_bare_last_fold()
    let cascade = target_bare_last_cascade()
    // single archetype = insertion order → last is the 7th entity (val 6)
    t |> equal(fast.val, 6, "bare last() reads the last entity")
    t |> equal(fast.val, cascade.val, "bare last val == tier-2 cascade")
    t |> equal(fast.flag, cascade.flag, "bare last flag == tier-2 cascade")
}

[test]
def test_bare_last_or_default_empty(t : T?) {
    restart()
    commit()
    t |> equal(target_bare_last_or_default_fold(), -1, "bare last_or_default empty → explicit default")
}

[test]
def test_bare_last_or_default_nonempty(t : T?) {
    fixture_unroll2(5)
    t |> equal(target_bare_last_or_default_fold(), 4, "bare last_or_default nonempty → last val")
}

// Eager-eval regression: the default expr must be bound BEFORE iteration (matches the walk lane),
// so its side effects fire even when the query is non-empty. A lazy tail-only eval would skip them.
var g_bare_default_evals = 0

def private bare_side_effecting_default() : UnrollTuple {
    g_bare_default_evals++
    return (val = -77, flag = -77)
}

[export, marker(no_coverage)]
def target_bare_last_or_default_sidefx() : int {
    return _fold(from_decs_template(type<UnrollChainRow>).last_or_default(bare_side_effecting_default())).val
}

[test]
def test_bare_last_or_default_eager_eval(t : T?) {
    fixture_unroll2(5)
    g_bare_default_evals = 0
    let v = target_bare_last_or_default_sidefx()
    t |> equal(v, 4, "non-empty → last val, not the default")
    t |> equal(g_bare_default_evals, 1, "default expr evaluated eagerly even when query non-empty")
}

// Proves the random-index fast path fired: bare last() emits a single void
// for_each_archetype with NO per-entity inner for-loop (the walk path clones the
// bridge for-loop → count_expr_for >= 1). No to_sequence, no _find.
[test]
def test_bare_last_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_bare_last_fold)
        t |> success(func != null, "RTTI must resolve target_bare_last_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> equal(describe_count(body_expr, "to_sequence"), 0, "bare last must NOT call to_sequence")
        t |> equal(describe_count(body_expr, "for_each_archetype_find"), 0, "bare last uses void for_each_archetype")
        t |> equal(describe_count(body_expr, "for_each_archetype"), 1, "exactly one for_each_archetype")
        t |> equal(count_expr_for(body_expr), 0, "random-index last has NO per-entity inner for-loop")
    }
}

// Iterator fallback: a default-init field compiles to get_default_ro (an iterator,
// not an indexable array), so decs_can_random_index → false. bare last() / reverse.take
// must still be correct via the walk fallback — checked vs the tier-2 cascade.
[decs_template(prefix = "fb_")]
struct FallbackRow {
    val : int
    hp  : int = 100
}

def private fixture_fallback(n : int) {
    restart()
    create_entities(n) $(_eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, FallbackRow(val = i, hp = i + 1))
    }
}

[export, marker(no_coverage)]
def target_fb_last_fold() : tuple<val : int; hp : int> {
    return _fold(from_decs_template(type<FallbackRow>).last())
}

[export, marker(no_coverage)]
def target_fb_last_cascade() : tuple<val : int; hp : int> {
    return from_decs_template(type<FallbackRow>) |> last()
}

[export, marker(no_coverage)]
def target_fb_reverse_take_fold() : array<tuple<val : int; hp : int>> {
    return <- _fold(from_decs_template(type<FallbackRow>).reverse().take(2).to_array())
}

[export, marker(no_coverage)]
def target_fb_reverse_take_cascade() : array<tuple<val : int; hp : int>> {
    return <- (from_decs_template(type<FallbackRow>) |> reverse() |> take(2) |> to_array())
}

[test]
def test_fb_last_parity(t : T?) {
    fixture_fallback(6)
    let fast = target_fb_last_fold()
    let cascade = target_fb_last_cascade()
    // assert the default-init field hp too (= i+1 = 6) — that's what forces the iterator-source fallback
    t |> equal(fast.val, 5, "fallback bare last() correct via walk")
    t |> equal(fast.hp, 6, "fallback bare last() default-init field via walk")
    t |> equal(fast.val, cascade.val, "fallback last val == cascade")
    t |> equal(fast.hp, cascade.hp, "fallback last hp == cascade")
}

[test]
def test_fb_reverse_take_parity(t : T?) {
    fixture_fallback(6)
    let fast <- target_fb_reverse_take_fold()
    let cascade <- target_fb_reverse_take_cascade()
    t |> equal(length(fast), 2, "fallback reverse.take(2) → 2 elements")
    t |> equal(length(fast), length(cascade), "fallback fast len == cascade len")
    for (a, b in fast, cascade) {
        t |> equal(a.val, b.val, "fallback reverse take val parity")
        t |> equal(a.hp, b.hp, "fallback reverse take hp parity")   // hp is the default-init field forcing the fallback
    }
    t |> equal(fast[0].val, 5, "fallback reverse take [0] val")
    t |> equal(fast[0].hp, 6, "fallback reverse take [0] hp")
    t |> equal(fast[1].val, 4, "fallback reverse take [1] val")
    t |> equal(fast[1].hp, 5, "fallback reverse take [1] hp")
}

// The fallback emission must take the walk (clones the bridge for-loop) — proven by
// the presence of a per-entity inner for-loop, which the random-index path lacks.
[test]
def test_fb_last_splice_shape(t : T?) {
    ast_gc_guard() {
        var func = find_module_function_via_rtti(compiling_module(), @@target_fb_last_fold)
        t |> success(func != null, "RTTI must resolve target_fb_last_fold")
        if (func == null) return
        var body_expr : ExpressionPtr
        let r = qmatch_function(func) $() {
            return <- $e(body_expr)
        }
        t |> success(r.matched && body_expr is ExprInvoke, "expected splice invoke wrapper")
        t |> success(count_expr_for(body_expr) >= 1, "fallback last walks → has a per-entity inner for-loop")
    }
}

