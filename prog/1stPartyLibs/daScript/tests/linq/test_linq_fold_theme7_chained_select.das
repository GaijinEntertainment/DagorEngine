options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 7 (audit `benchmarks/sql/linq_fold_chain_audit.md` "Theme 7" section):
// chained `_select(f) |> _select(g)` collapse. Symmetric to how chained `_where`
// already compose via `&&`. `collapse_chained_selects` mutates the call chain
// in place — N consecutive `_select` calls become a single composed `_select(g(f(_)))`.
// Gated on PURITY of the inner projection (`!has_sideeffects(innerBody)`) — for
// impure inner, eval-count would shift (outer may reference its param 0 / 1 / >1
// times); cascading preserves semantics.
//
// Primary audit target is 7c (chained selects on `plan_zip`). The pass also fires
// in plan_distinct / plan_order_family / plan_reverse / plan_decs_* / plan_decs_join
// — the same 7c shape on those upstream arms now splices.

// ── 7c primary: zip + 2 pure selects + to_array ──────────────────────────────

[test]
def test_t7c_zip_two_selects_to_array(t : T?) {
    t |> run("7c: each(a) |> zip(each(b)) |> _select(_._0) |> _select(_ * 2) |> to_array — audit primary target") @(tt : T?) {
        let a <- [1, 2, 3, 4]
        let b <- [10, 20, 30, 40]
        let r <- _fold(each(a)
            |> zip(each(b))
            |> _select(_._0)
            |> _select(_ * 2)
            |> to_array())
        tt |> equal(length(r), 4)
        tt |> equal(r[0], 2)
        tt |> equal(r[1], 4)
        tt |> equal(r[2], 6)
        tt |> equal(r[3], 8)
    }
}

// ── 7c N=3: zip + three pure selects + count ─────────────────────────────────

[test]
def test_t7c_zip_three_selects_count(t : T?) {
    t |> run("7c N=3: zip + _select(_._0) + _select(_ + 1) + _select(_ * 10) + count — runs of three adjacent selects fold left-to-right") @(tt : T?) {
        let a <- [1, 2, 3]
        let b <- [10, 20, 30]
        let n = _fold(each(a)
            |> zip(each(b))
            |> _select(_._0)
            |> _select(_ + 1)
            |> _select(_ * 10)
            |> count())
        tt |> equal(n, 3)
    }
}

// ── 7c-parity: collapsed splice produces same array as handwritten cascade ──

[test]
def test_t7c_parity_vs_cascade(t : T?) {
    t |> run("7c parity: chained-select splice output equals handwritten cascade element-wise") @(tt : T?) {
        let a <- [for (i in range(20)); i * 3]
        let b <- [for (i in range(20)); 100 - i]
        let spliced <- _fold(each(a)
            |> zip(each(b))
            |> _select(_._0)
            |> _select(_ + 7)
            |> to_array())
        // Handwritten cascade: just compute (a[i] + 7) element-wise.
        let cascade <- [for (x in a); x + 7]
        tt |> equal(length(spliced), length(cascade))
        for (i in range(length(cascade))) {
            tt |> equal(spliced[i], cascade[i])
        }
    }
}

// ── 7c distinct: 2 pure selects + distinct + count ───────────────────────────

[test]
def test_t7c_distinct_two_selects(t : T?) {
    t |> run("plan_distinct: each + _select(_ + 1) + _select(_ * 10) + distinct + count — chained selects collapse before distinct") @(tt : T?) {
        // 10 elements 0..9 → +1 → ×10 = 10, 20, 30, ..., 100 → all distinct → 10.
        let a <- [for (i in range(10)); i]
        let n = _fold(each(a)
            |> _select(_ + 1)
            |> _select(_ * 10)
            |> distinct
            |> count())
        tt |> equal(n, 10)
    }
}

// ── 7c reverse: 2 pure selects + reverse + to_array ──────────────────────────

[test]
def test_t7c_reverse_two_selects(t : T?) {
    t |> run("plan_reverse: each + _select(_ + 1) + _select(_ * 10) + reverse + to_array — chained selects collapse before reverse") @(tt : T?) {
        let a <- [1, 2, 3]
        let r <- _fold(each(a)
            |> _select(_ + 1)
            |> _select(_ * 10)
            |> reverse()
            |> to_array())
        // Composed: x => (x + 1) * 10. Values: 20, 30, 40. Reversed: 40, 30, 20.
        tt |> equal(length(r), 3)
        tt |> equal(r[0], 40)
        tt |> equal(r[1], 30)
        tt |> equal(r[2], 20)
    }
}

// ── 7c-decs: from_decs_template + 2 selects + distinct + count ───────────────

[decs_template(prefix = "t7_")]
struct T7Item {
    id : int
    grp : int
}

def populate_t7_decs() {
    restart()
    create_entities(20) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, T7Item(id = i, grp = i / 4))
    }
}

[test]
def test_t7c_decs_distinct_two_selects(t : T?) {
    t |> run("plan_decs_distinct: from_decs_template + _select(_.grp) + _select(_ + 1) + distinct + count — chained selects collapse on decs distinct lane") @(tt : T?) {
        populate_t7_decs()
        // 20 items, grp = i/4 ∈ {0,1,2,3,4}. Composed: grp + 1 ∈ {1,2,3,4,5}. Distinct = 5.
        let n = _fold(from_decs_template(type<T7Item>)
            |> _select(_.grp)
            |> _select(_ + 1)
            |> distinct
            |> count())
        tt |> equal(n, 5)
        restart()
    }
}

// ── 7c-decs-reverse: from_decs_template + 2 selects + reverse + to_array ─────

[test]
def test_t7c_decs_reverse_two_selects(t : T?) {
    t |> run("plan_decs_reverse: from_decs_template + _select(_.id) + _select(_ * 10) + reverse + to_array — chained selects collapse on decs reverse path") @(tt : T?) {
        populate_t7_decs()
        // 20 items, id = 0..19. Composed: id * 10 ∈ {0,10,...,190}. Reversed: 190..0.
        let r <- _fold(from_decs_template(type<T7Item>)
            |> _select(_.id)
            |> _select(_ * 10)
            |> reverse()
            |> to_array())
        tt |> equal(length(r), 20)
        tt |> equal(r[0], 190)
        tt |> equal(r[1], 180)
        tt |> equal(r[19], 0)
        restart()
    }
}

// ── 7c-anti-impure: inner `% 5` is flagged unsafe → cascade preserves output ──

[test]
def test_t7c_anti_impure_inner_cascade(t : T?) {
    t |> run("anti-impure: each + _select(_ % 5) + _select(_ + 1) + distinct + count — inner has division-class op, gate falls back to cascade; output still correct") @(tt : T?) {
        // Inner `_ % 5` is op2(%) which has_sideeffects flags as unsafe (div-by-zero panic).
        // Gate refuses collapse; cascade fires; output must still be correct.
        let a <- [for (i in range(20)); i]
        let n = _fold(each(a)
            |> _select(_ % 5)
            |> _select(_ + 1)
            |> distinct
            |> count())
        // 0..19 % 5 ∈ {0,1,2,3,4}. + 1 ∈ {1,2,3,4,5}. Distinct = 5.
        tt |> equal(n, 5)
    }
}

// ── 7c-anti-nonadjacent: select + take + select → not collapsed (intervening op) ──

[test]
def test_t7c_anti_nonadjacent_selects(t : T?) {
    t |> run("anti-nonadjacent: each + _select(_ + 1) + take(N) + _select(_ * 10) + sum — selects separated by take don't collapse; output still correct") @(tt : T?) {
        // plan_loop_or_count handles intermediate selects via its own intermediateBinds.
        // collapse_chained_selects only fires on ADJACENT `_select(...) |> _select(...)`.
        let a <- [for (i in range(10)); i]
        let n = _fold(each(a)
            |> _select(_ + 1)
            |> take(3)
            |> _select(_ * 10)
            |> sum())
        // First 3 of (i+1): 1, 2, 3. * 10: 10, 20, 30. Sum: 60.
        tt |> equal(n, 60)
    }
}
