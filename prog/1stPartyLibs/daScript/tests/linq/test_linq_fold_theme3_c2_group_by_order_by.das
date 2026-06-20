options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 3 Phase 2 (audit `benchmarks/sql/linq_fold_chain_audit.md` probe C2): the
// canonical SQL `GROUP BY ... ORDER BY COUNT(*)` shape ("brands sorted by
// frequency"). plan_group_by_core gained an `orderName`/`orderKey` lane that
// emits an inline-cmp `sort(buf, ...)` right after the to_array buf-fill,
// mutating the same buffer in place. Today (pre-PR) this composition cascades
// to tier-2 (3 array allocs: group_by_lazy_to_array → select → order_by_inplace).

// ── C2a: array source + order_by ascending ────────────────────────────────────

[test]
def test_c2a_array_order_by_asc(t : T?) {
    t |> run("C2a: each(items) + _group_by + _select((B,C=count)) + _order_by(_.C) + to_array — array source, ascending") @(tt : T?) {
        var items <- [for (i in range(20)); (brand = "B{i % 4}", price = float(i))]
        let rows <- _fold(each(items)
            |> _group_by(_.brand)
            |> _select((B = _._0, C = _._1 |> count()))
            |> _order_by(_.C)
            |> to_array())
        // 4 brands, each with 5 rows. C is identical → order_by ties trivially.
        tt |> equal(length(rows), 4)
        for (r in rows) {
            tt |> equal(r.C, 5)
        }
    }
}

// ── C2b: array source + order_by descending with unequal counts ───────────────

[test]
def test_c2b_array_order_by_desc(t : T?) {
    t |> run("C2b: _order_by_descending + unequal bucket counts — must produce descending-by-C sequence") @(tt : T?) {
        // Brand frequencies: A×6, B×3, C×2, D×1 → desc order [A, B, C, D].
        var items <- [for (k in [
            "A", "A", "A", "A", "A", "A",
            "B", "B", "B",
            "C", "C",
            "D"
        ]); (brand = k, price = 0.0f)]
        let rows <- _fold(each(items)
            |> _group_by(_.brand)
            |> _select((B = _._0, C = _._1 |> count()))
            |> _order_by_descending(_.C)
            |> to_array())
        tt |> equal(length(rows), 4)
        tt |> equal(rows[0].B, "A")
        tt |> equal(rows[0].C, 6)
        tt |> equal(rows[1].B, "B")
        tt |> equal(rows[1].C, 3)
        tt |> equal(rows[2].B, "C")
        tt |> equal(rows[2].C, 2)
        tt |> equal(rows[3].B, "D")
        tt |> equal(rows[3].C, 1)
    }
}

// ── C2c: parity vs handwritten cascade — same input, two paths agree ──────────

def c2c_handwritten(items : array<tuple<brand : string; price : float>>) : array<tuple<B : string; C : int>> {
    var perBrand : table<string; int>
    for (it in items) {
        if (key_exists(perBrand, it.brand)) {
            perBrand[it.brand] ++
        } else {
            perBrand |> insert(it.brand, 1)
        }
    }
    var out <- [for (k, v in keys(perBrand), values(perBrand)); (B = k, C = v)]
    sort(out, $(a, b) => _::less(a.C, b.C))
    return <- out
}

[test]
def test_c2c_parity_handwritten(t : T?) {
    t |> run("C2c: spliced sort vs handwritten cascade — sequences must match element-for-element") @(tt : T?) {
        // Distinct counts so ascending sort is deterministic.
        var items <- [for (k in [
            "X", "X",
            "Y", "Y", "Y", "Y",
            "Z",
            "W", "W", "W"
        ]); (brand = k, price = 0.0f)]
        let spliced <- _fold(each(items)
            |> _group_by(_.brand)
            |> _select((B = _._0, C = _._1 |> count()))
            |> _order_by(_.C)
            |> to_array())
        let hand <- c2c_handwritten(items)
        tt |> equal(length(spliced), length(hand))
        for (i in range(length(spliced))) {
            tt |> equal(spliced[i].B, hand[i].B)
            tt |> equal(spliced[i].C, hand[i].C)
        }
    }
}

// ── C2d: order_by composes with HAVING (trailing _where) ──────────────────────

[test]
def test_c2d_order_by_after_having(t : T?) {
    t |> run("C2d: _select(reducer) + _where(HAVING) + _order_by + to_array — HAVING gates buckets, sort fires on survivors") @(tt : T?) {
        // A×6, B×3, C×2, D×1 → HAVING C>=2 keeps A,B,C; sort asc → C,B,A.
        var items <- [for (k in [
            "A", "A", "A", "A", "A", "A",
            "B", "B", "B",
            "C", "C",
            "D"
        ]); (brand = k, price = 0.0f)]
        let rows <- _fold(each(items)
            |> _group_by(_.brand)
            |> _select((B = _._0, C = _._1 |> count()))
            |> _where(_.C >= 2)
            |> _order_by(_.C)
            |> to_array())
        tt |> equal(length(rows), 3)
        tt |> equal(rows[0].B, "C")
        tt |> equal(rows[0].C, 2)
        tt |> equal(rows[1].B, "B")
        tt |> equal(rows[1].C, 3)
        tt |> equal(rows[2].B, "A")
        tt |> equal(rows[2].C, 6)
    }
}

// ── C2-anti: non-inline key cascades ─────────────────────────────────────────

// Side-effecting key body fails try_make_inline_cmp's `has_sideeffects(ret.subexpr)` guard → plan_group_by_core returns null → full cascade.
var g_c2anti_key_calls = 0

def c2anti_count_and_return_C(v : tuple<B : string; C : int>) : int {
    g_c2anti_key_calls ++
    return v.C
}

[test]
def test_c2_anti_non_inline_key_cascades(t : T?) {
    t |> run("C2-anti: _order_by(side-effecting fn call) — must cascade to tier-2 (no splice) AND produce correct sorted output") @(tt : T?) {
        let items <- [for (k in [
            "A", "A", "A",
            "B", "B",
            "C"
        ]); (brand = k, price = 0.0f)]
        g_c2anti_key_calls = 0
        let rows <- _fold(each(items)
            |> _group_by(_.brand)
            |> _select((B = _._0, C = _._1 |> count()))
            |> _order_by(c2anti_count_and_return_C(_))
            |> to_array())
        // Output sorted ascending by C: C(1), B(2), A(3).
        tt |> equal(length(rows), 3)
        tt |> equal(rows[0].B, "C")
        tt |> equal(rows[1].B, "B")
        tt |> equal(rows[2].B, "A")
        // Cascade path (linq.das `order_by_inplace`) invokes the key per element — counter must be > 0. Splice would have inlined the body and never executed the increment.
        tt |> equal(g_c2anti_key_calls > 0, true)
    }
}

// ── C2-decs: decs source + order_by ──────────────────────────────────────────

[decs_template(prefix = "c2dt_")]
struct C2Sale {
    region : string
    amount : int
}

def populate_c2_decs() {
    restart()
    // R×4, S×2, T×1 sales → desc by count: R, S, T.
    create_entities(7) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        let region = (i < 4 ? "R" : (i < 6 ? "S" : "T"))
        apply_decs_template(cmp, C2Sale(region = region, amount = i * 10))
    }
}

[test]
def test_c2_decs_order_by_desc(t : T?) {
    t |> run("C2-decs: from_decs_template + _group_by + _select(count) + _order_by_descending + to_array — decs adapter feeds inline sort") @(tt : T?) {
        populate_c2_decs()
        unsafe {
            let rows <- _fold(from_decs_template(type<C2Sale>)
                |> _group_by(_.region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> _order_by_descending(_.N)
                |> to_array())
            tt |> equal(length(rows), 3)
            tt |> equal(rows[0].R, "R")
            tt |> equal(rows[0].N, 4)
            tt |> equal(rows[1].R, "S")
            tt |> equal(rows[1].N, 2)
            tt |> equal(rows[2].R, "T")
            tt |> equal(rows[2].N, 1)
        }
        restart()
    }
}

// ── C2-decs-join: cross-arm composition — decs_join + group_by + order_by ────

[decs_template(prefix = "c2j_car_")]
struct C2JoinCar {
    id : int
    dealer_id : int
}

[decs_template(prefix = "c2j_dlr_")]
struct C2JoinDealer {
    id : int
    region : string
}

def populate_c2_decs_join() {
    restart()
    // 6 cars: dealer_ids cycle 1,2,3,1,2,3 → 2 cars per dealer.
    create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, C2JoinCar(id = i, dealer_id = i % 3 + 1))
    }
    // 3 dealers; ids 1,2 → "north" (4 cars), id 3 → "south" (2 cars).
    create_entities(3) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        let region = (i == 2 ? "south" : "north")
        apply_decs_template(cmp, C2JoinDealer(id = i + 1, region = region))
    }
}

[test]
def test_c2_decs_join_order_by(t : T?) {
    t |> run("C2-decs-join: from_decs_template(Car) + _join(Dealer) + _group_by(Region) + _select(count) + _order_by + to_array — isDecsJoin adapter + inline sort, single pass end-to-end") @(tt : T?) {
        populate_c2_decs_join()
        unsafe {
            let rows <- _fold(from_decs_template(type<C2JoinCar>)
                |> _join(from_decs_template(type<C2JoinDealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarId = l.id))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> _order_by(_.N)
                |> to_array())
            // south(2), north(4) ascending.
            tt |> equal(length(rows), 2)
            tt |> equal(rows[0].R, "south")
            tt |> equal(rows[0].N, 2)
            tt |> equal(rows[1].R, "north")
            tt |> equal(rows[1].N, 4)
        }
        restart()
    }
}
