options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 4 + Theme 5 (audit `benchmarks/sql/linq_fold_chain_audit.md`):
//   Theme 4 — 2-arg terminator predicates: plan_distinct + plan_decs_distinct + plan_zip
//             accept count(p) / long_count(p). plan_decs_unroll already had the machinery
//             in emit_decs_accumulator; this PR unblocks it via the extract_decs_bridge
//             iter-name recovery fix for bare-chain from_decs_template shapes.
//   Theme 5 — _order_by(k).reverse() → _order_by_descending(k) normalization (and the
//             three symmetric flips: order/order_descending and order_by_descending/order_by).
//             Closes 1b (order + reverse + take) and 2b (reverse over order); pre-planner
//             rewrite so the existing order_family arms fire on the normalized chain.

struct User {
    region : string
    active : bool
    score  : int
}

[decs_template(prefix = "dr_")]
struct DecsRecord {
    region : string
    active : bool
    score  : int
}

[decs_template(prefix = "di_")]
struct DecsItem {
    category : string
    price : int
}

def make_users() : array<User> {
    return <- [
        User(region = "us", active = true,  score = 10),
        User(region = "us", active = false, score = 20),
        User(region = "us", active = true,  score = 30),
        User(region = "eu", active = true,  score = 40),
        User(region = "eu", active = true,  score = 50),
        User(region = "ap", active = false, score = 60),
        User(region = "ap", active = true,  score = 70)
    ]
}

// ── Theme 4: plan_distinct count(p) / long_count(p) (closes audit 3c) ─────────

[test]
def test_distinct_count_pred(t : T?) {
    t |> run("plan_distinct: distinct_by(region).count(active)") @(tt : T?) {
        let users <- make_users()
        unsafe {
            // distinct_by keeps FIRST occurrence per region: us→active=true, eu→active=true, ap→active=false.
            // count(active) over deduped → 2.
            let n = _fold(each(users)._distinct_by(_.region) |> count($(u) => u.active))
            tt |> equal(n, 2)
        }
    }
}

[test]
def test_distinct_long_count_pred(t : T?) {
    t |> run("plan_distinct: distinct_by(region).long_count(active)") @(tt : T?) {
        let users <- make_users()
        unsafe {
            let n = _fold(each(users)._distinct_by(_.region) |> long_count($(u) => u.active))
            tt |> equal(n, 2l)
        }
    }
}

[test]
def test_distinct_count_pred_all_match(t : T?) {
    t |> run("plan_distinct: distinct_by + count(p) where all distinct match") @(tt : T?) {
        let users <- make_users()
        unsafe {
            // All distinct regions have score > 0 → all 3 inserted.
            let n = _fold(each(users)._distinct_by(_.region) |> count($(u) => u.score > 0))
            tt |> equal(n, 3)
        }
    }
}

// ── Theme 4: plan_decs_distinct count(p) / long_count(p) (decs mirror of 3c) ──

[test]
def test_decs_distinct_count_pred(t : T?) {
    t |> run("plan_decs_distinct: from_decs_template + distinct_by(region) + count(active)") @(tt : T?) {
        restart()
        let regions = ["us", "us", "us", "eu", "eu", "ap", "ap"]
        let actives = [true, false, true, true, true, false, true]
        let scores  = [10, 20, 30, 40, 50, 60, 70]
        create_entities(7) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsRecord(region = regions[i], active = actives[i], score = scores[i]))
        }
        unsafe {
            let n = _fold(from_decs_template(type<DecsRecord>)
                          |> _distinct_by(_.region)
                          |> count($(r) => r.active))
            // distinct_by keeps FIRST per region: us→true, eu→true, ap→false. count(active) → 2.
            tt |> equal(n, 2)
        }
        restart()
    }
}

// ── Theme 4: plan_zip count(p) / long_count(p) (closes audit 7-family predicated count) ──

[test]
def test_zip_count_pred(t : T?) {
    t |> run("plan_zip: zip + count(p) over pair") @(tt : T?) {
        let a <- [1, 2, 3, 4, 5]
        let b <- [10, 20, 30, 40, 50]
        unsafe {
            // Pairs: (1,10)=11, (2,20)=22, (3,30)=33, (4,40)=44, (5,50)=55. count where sum>30 → 3.
            let n = _fold(each(a) |> zip(each(b)) |> count($(p) => p._0 + p._1 > 30))
            tt |> equal(n, 3)
        }
    }
}

[test]
def test_zip_long_count_pred(t : T?) {
    t |> run("plan_zip: zip + long_count(p)") @(tt : T?) {
        let a <- [1, 2, 3]
        let b <- [10, 20, 30]
        unsafe {
            let n = _fold(each(a) |> zip(each(b)) |> long_count($(p) => p._0 < 3))
            tt |> equal(n, 2l)
        }
    }
}

[test]
def test_zip_where_then_count_pred(t : T?) {
    t |> run("plan_zip: zip + where + count(p) AND merge") @(tt : T?) {
        let a <- [1, 2, 3, 4, 5]
        let b <- [10, 20, 30, 40, 50]
        unsafe {
            // zip pairs filtered where p._0 > 1 → 4 pairs (2,3,4,5); count where p._1 < 50 → 3.
            let n = _fold(each(a) |> zip(each(b)) |> _where(_._0 > 1) |> count($(p) => p._1 < 50))
            tt |> equal(n, 3)
        }
    }
}

[test]
def test_zip_select_then_count_pred(t : T?) {
    t |> run("plan_zip: zip + select(F) + count(p) — p binds to post-select element") @(tt : T?) {
        let a <- [1, 2, 3, 4, 5]
        let b <- [10, 20, 30, 40, 50]
        unsafe {
            // Pairs: (1,10),(2,20),(3,30),(4,40),(5,50). Projected sums: 11,22,33,44,55. Count where sum > 30 → 3.
            // Critical: P's `s` must bind to the projected sum, not the pre-select tuple.
            let n = _fold(each(a) |> zip(each(b)) |> select($(p : tuple<int; int>) => p._0 + p._1) |> count($(s) => s > 30))
            tt |> equal(n, 3)
        }
    }
}

[test]
def test_zip_select_then_long_count_pred(t : T?) {
    t |> run("plan_zip: zip + select(F) + long_count(p) — p binds to post-select element") @(tt : T?) {
        let a <- [1, 2, 3, 4, 5]
        let b <- [10, 20, 30, 40, 50]
        unsafe {
            // Projected products: 10,40,90,160,250. Count where product > 50 → 3 (90,160,250).
            let n = _fold(each(a) |> zip(each(b)) |> select($(p : tuple<int; int>) => p._0 * p._1) |> long_count($(v) => v > 50))
            tt |> equal(n, 3l)
        }
    }
}

var g_side_effect_calls = 0

def side_effect_proj(x : int) : int {
    g_side_effect_calls ++
    return x * 2
}

[test]
def test_zip_where_select_count_pred_side_effect_order(t : T?) {
    t |> run("plan_zip: where filters BEFORE side-effecting select runs (R3-1 regression)") @(tt : T?) {
        let a <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let b <- [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        // Upstream where keeps 5 elements (a._0 in {2,4,6,8,10}). Eager `where(W).select(F).count(P)` runs F only on those 5.
        // If counterPred merged into whereCond (old bug), F would run on all 10 — driving g_side_effect_calls to 10.
        g_side_effect_calls = 0
        unsafe {
            let n = _fold(
                each(a) |> zip(each(b))
                |> where_($(p : tuple<int; int>) => p._0 % 2 == 0)
                |> select($(p : tuple<int; int>) => side_effect_proj(p._0))
                |> count($(v) => v > 5)
            )
            // 5 elements survive where (2,4,6,8,10) → projected to (4,8,12,16,20) → 4 are > 5.
            tt |> equal(n, 4)
            tt |> equal(g_side_effect_calls, 5)
        }
    }
}

// ── Theme 4: bare from_decs_template + count() and count(p) (extract_decs_bridge fix) ──

[test]
def test_decs_bare_count(t : T?) {
    t |> run("plan_decs_unroll Slice 1: bare count() over from_decs_template (bridge recovery)") @(tt : T?) {
        restart()
        create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            let cats = ["A", "A", "B", "B", "B", "C"]
            let prices = [100, 200, 300, 400, 500, 600]
            apply_decs_template(cmp, DecsItem(category = cats[i], price = prices[i]))
        }
        unsafe {
            let n = _fold(from_decs_template(type<DecsItem>) |> count())
            tt |> equal(n, 6)
        }
        restart()
    }
}

[test]
def test_decs_bare_count_pred(t : T?) {
    t |> run("plan_decs_unroll: bare count(p) over from_decs_template (bridge recovery + 2-arg)") @(tt : T?) {
        restart()
        create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            let cats = ["A", "A", "B", "B", "B", "C"]
            let prices = [100, 200, 300, 400, 500, 600]
            apply_decs_template(cmp, DecsItem(category = cats[i], price = prices[i]))
        }
        unsafe {
            let n = _fold(from_decs_template(type<DecsItem>) |> count($(x) => x.category == "B"))
            tt |> equal(n, 3)
        }
        restart()
    }
}

// ── Theme 5: order_by(k).reverse() → order_by_descending(k) (closes audit 1b) ─

[test]
def test_order_by_reverse_take(t : T?) {
    t |> run("Theme 5 1b: _order_by(_).reverse().take(3) normalizes to _order_by_descending(_).take(3)") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            // Ascending: [1,1,2,3,3,4,5,5,5,6,9]. Reversed: [9,6,5,5,5,4,3,3,2,1,1]. Take 3: [9,6,5].
            let top3 <- _fold(each(scores)._order_by(_).reverse().take(3).to_array())
            tt |> equal(length(top3), 3)
            tt |> equal(top3[0], 9)
            tt |> equal(top3[1], 6)
            tt |> equal(top3[2], 5)
        }
    }
}

[test]
def test_order_by_descending_reverse_take(t : T?) {
    t |> run("Theme 5 symmetric: _order_by_descending(_).reverse().take(3) → _order_by(_).take(3)") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            // Descending: [9,6,5,5,5,4,3,3,2,1,1]. Reversed: ascending. Take 3: [1,1,2].
            let bottom3 <- _fold(each(scores)._order_by_descending(_).reverse().take(3).to_array())
            tt |> equal(length(bottom3), 3)
            tt |> equal(bottom3[0], 1)
            tt |> equal(bottom3[1], 1)
            tt |> equal(bottom3[2], 2)
        }
    }
}

[test]
def test_double_reverse_after_order_descending(t : T?) {
    t |> run("Theme 5: _order_by_descending(_).reverse().reverse() cancels to _order_by_descending(_)") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5]
        unsafe {
            // After cancellation: descending [5,4,3,1,1].
            let r <- _fold(each(scores)._order_by_descending(_).reverse().reverse().to_array())
            tt |> equal(length(r), 5)
            tt |> equal(r[0], 5)
            tt |> equal(r[1], 4)
            tt |> equal(r[2], 3)
        }
    }
}

[test]
def test_decs_order_by_reverse(t : T?) {
    t |> run("Theme 5 decs mirror: from_decs_template._order_by(_.price).reverse().take(2)") @(tt : T?) {
        restart()
        create_entities(5) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            let cats = ["A", "B", "C", "D", "E"]
            let prices = [100, 500, 200, 800, 50]
            apply_decs_template(cmp, DecsItem(category = cats[i], price = prices[i]))
        }
        unsafe {
            // Normalized to _order_by_descending(_.price).take(2) — top 2 by price.
            let top2 <- _fold(from_decs_template(type<DecsItem>)._order_by(_.price).reverse().take(2).to_array())
            tt |> equal(length(top2), 2)
            tt |> equal(top2[0].price, 800)
            tt |> equal(top2[1].price, 500)
        }
        restart()
    }
}
