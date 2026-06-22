options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 2 (audit `benchmarks/sql/linq_fold_chain_audit.md`): trailing `_where` extensions
// across plan_decs_join (8a, C6), plan_group_by_core (4a, 4e), plan_loop_or_count (5c).

struct Item {
    category : string
    price    : int
}

struct ActItem {
    active : bool
    score  : int
}

[decs_template(prefix = "dc_")]
struct DecsCar {
    id : int
    dealer_id : int
    name : string
}

[decs_template(prefix = "dd_")]
struct DecsDealer {
    id : int
    name : string
}

[decs_template(prefix = "di_")]
struct DecsItem {
    category : string
    price : int
}

def make_items() : array<Item> {
    return <- [
        Item(category = "A", price = 100),
        Item(category = "A", price = 300),
        Item(category = "B", price = 200),
        Item(category = "B", price = 800),
        Item(category = "B", price = 200),
        Item(category = "C", price = 50)
    ]
}

def make_act_items() : array<ActItem> {
    return <- [
        ActItem(active = true,  score = 10),
        ActItem(active = false, score = 20),
        ActItem(active = true,  score = 30),
        ActItem(active = false, score = 40),
        ActItem(active = true,  score = 50),
        ActItem(active = false, score = 60),
        ActItem(active = true,  score = 70),
        ActItem(active = true,  score = 80)
    ]
}

// ── plan_decs_join trailing _where (closes 8a, C6) ─────────────────────────────

[test]
def test_join_where_count(t : T?) {
    t |> run("plan_decs_join: trailing _where + count (probe 8a)") @(tt : T?) {
        restart()
        create_entities(4) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsCar(id = i + 1, dealer_id = i % 2 + 1, name = "Car{i}"))
        }
        create_entities(2) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsDealer(id = i + 1, name = "Dealer{i}"))
        }
        unsafe {
            let filtered = _fold(from_decs_template(type<DecsCar>) |> _join(from_decs_template(type<DecsDealer>),
                                                                            $(l, r) => l.dealer_id == r.id,
                                                                            $(l, r) => (CarName = l.name, DealerName = r.name))
                                                                   |> _where(_.DealerName == "Dealer0")
                                                                   |> count())
            // 4 cars × dealer_id 1,2,1,2; matching Dealer0 (id=1) → cars i=0,2 → 2 results.
            tt |> equal(filtered, 2)
        }
        restart()
    }
}

[test]
def test_join_where_to_array(t : T?) {
    t |> run("plan_decs_join: trailing _where + to_array") @(tt : T?) {
        restart()
        create_entities(4) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsCar(id = i + 1, dealer_id = i % 2 + 1, name = "Car{i}"))
        }
        create_entities(2) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsDealer(id = i + 1, name = "Dealer{i}"))
        }
        unsafe {
            let rows <- _fold(from_decs_template(type<DecsCar>) |> _join(from_decs_template(type<DecsDealer>),
                                                                         $(l, r) => l.dealer_id == r.id,
                                                                         $(l, r) => (CarName = l.name, DealerName = r.name))
                                                                |> _where(_.DealerName == "Dealer1")
                                                                |> to_array())
            tt |> equal(length(rows), 2)
        }
        restart()
    }
}

[test]
def test_join_where_select_to_array(t : T?) {
    t |> run("plan_decs_join: trailing _where + _select + to_array (combination)") @(tt : T?) {
        restart()
        create_entities(4) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsCar(id = i + 1, dealer_id = i % 2 + 1, name = "Car{i}"))
        }
        create_entities(2) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsDealer(id = i + 1, name = "Dealer{i}"))
        }
        unsafe {
            let names <- _fold(from_decs_template(type<DecsCar>) |> _join(from_decs_template(type<DecsDealer>),
                                                                          $(l, r) => l.dealer_id == r.id,
                                                                          $(l, r) => (CarName = l.name, DealerName = r.name))
                                                                 |> _where(_.DealerName == "Dealer0")
                                                                 |> _select(_.CarName)
                                                                 |> to_array())
            tt |> equal(length(names), 2)
        }
        restart()
    }
}

// ── plan_group_by_core trailing _where as HAVING (closes 4a, 4e) ───────────────

[test]
def test_groupby_having_where_to_array(t : T?) {
    t |> run("plan_group_by: trailing _where (HAVING) on post-aggregate tuple (probe 4a)") @(tt : T?) {
        let items <- make_items()
        unsafe {
            let rows <- _fold(each(items)
                ._group_by(_.category)
                ._select((Cat = _._0, Total = _._1 |> select(@(i : Item) => i.price) |> sum))
                ._where(_.Total > 500) |> to_array())
            // Category totals: A=400, B=1200, C=50. Filter Total > 500 → only B.
            tt |> equal(length(rows), 1)
        }
    }
}

[test]
def test_groupby_having_where_count(t : T?) {
    t |> run("plan_group_by: trailing _where + count") @(tt : T?) {
        let items <- make_items()
        unsafe {
            let n = _fold(each(items)
                ._group_by(_.category)
                ._select((Cat = _._0, Total = _._1 |> select(@(i : Item) => i.price) |> sum))
                ._where(_.Total > 100)
                |> count())
            // Buckets with Total > 100: A (400), B (1200) → 2.
            tt |> equal(n, 2)
        }
    }
}

[test]
def test_groupby_having_where_decs(t : T?) {
    t |> run("plan_decs_group_by: trailing _where (probe 4e)") @(tt : T?) {
        restart()
        create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            let cats = ["A", "A", "B", "B", "B", "C"]
            let prices = [100, 300, 200, 800, 200, 50]
            apply_decs_template(cmp, DecsItem(category = cats[i], price = prices[i]))
        }
        unsafe {
            let rows <- _fold(from_decs_template(type<DecsItem>)
                ._group_by(_.category)
                ._select((Cat = _._0, Total = _._1 |> select(@(i : tuple<category : string; price : int>) => i.price) |> sum))
                ._where(_.Total > 500)
                |> to_array())
            // Same data as array: A=400, B=1200, C=50. Filter keeps B.
            tt |> equal(length(rows), 1)
        }
        restart()
    }
}

// ── plan_loop_or_count take.where (closes 5c, all 4 lanes) ─────────────────────

[test]
def test_take_where_count(t : T?) {
    t |> run("plan_loop_or_count: take(N)._where(p).count() (counter lane, probe 5c)") @(tt : T?) {
        let items <- make_act_items()
        unsafe {
            // First 5: active = [T,F,T,F,T]. Count active = 3.
            // Semantic distinction from _where(p).take(5): would be 5 (5 active in items).
            let r1 = _fold(each(items).take(5)._where(_.active).count())
            tt |> equal(r1, 3)
            let r2 = _fold(each(items)._where(_.active).take(5).count())
            tt |> equal(r2, 5)
        }
    }
}

[test]
def test_take_where_sum(t : T?) {
    t |> run("plan_loop_or_count: take(N)._where(p).sum() (accumulator lane)") @(tt : T?) {
        let items <- make_act_items()
        unsafe {
            // _select projects scores. take(5) → [10,20,30,40,50]. _where(>0) keeps all. sum=150.
            let s = _fold(each(items)._select(_.score).take(5)._where(_ > 0).sum())
            tt |> equal(s, 150)
        }
    }
}

[test]
def test_take_where_first(t : T?) {
    t |> run("plan_loop_or_count: take(N)._where(p).first_or_default() (early-exit lane)") @(tt : T?) {
        let items <- make_act_items()
        unsafe {
            // take(3) → first 3 (T,F,T). First active = 10.
            let f = _fold(each(items).take(3)._where(_.active).first_or_default(ActItem(active = false, score = -1)))
            tt |> equal(f.score, 10)
        }
    }
}

[test]
def test_take_where_to_array(t : T?) {
    t |> run("plan_loop_or_count: take(N)._where(p).to_array() (array lane)") @(tt : T?) {
        let items <- make_act_items()
        unsafe {
            // take(5) → first 5 (T,F,T,F,T). where(active) → 3 items, scores 10,30,50.
            let arr <- _fold(each(items).take(5)._where(_.active).to_array())
            tt |> equal(length(arr), 3)
            tt |> equal(arr[0].score, 10)
            tt |> equal(arr[2].score, 50)
        }
    }
}

[test]
def test_take_zero_where(t : T?) {
    t |> run("plan_loop_or_count: take(0)._where edge case") @(tt : T?) {
        let items <- make_act_items()
        unsafe {
            let r = _fold(each(items).take(0)._where(_.active).count())
            tt |> equal(r, 0)
        }
    }
}

// ── plan_loop_or_count take.where — DECS arm (closes 5c on decs, mirrors array above) ─

// Mirrors make_act_items() via DecsItem (category="T" ⇔ active=true, price=score). 8 rows:
// [T:10, F:20, T:30, F:40, T:50, F:60, T:70, T:80].
def populate_decs_act_items() {
    let cats   = ["T", "F", "T", "F", "T", "F", "T", "T"]
    let prices = [ 10,  20,  30,  40,  50,  60,  70,  80]
    create_entities(8) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, DecsItem(category = cats[i], price = prices[i]))
    }
}

[test]
def test_take_where_count_decs(t : T?) {
    t |> run("plan_loop_or_count: decs take(N)._where(p).count() (counter lane, probe 5c)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            let r1 = _fold(from_decs_template(type<DecsItem>).take(5)._where(_.category == "T").count())
            tt |> equal(r1, 3)
            let r2 = _fold(from_decs_template(type<DecsItem>)._where(_.category == "T").take(5).count())
            tt |> equal(r2, 5)
        }
        restart()
    }
}

[test]
def test_take_where_sum_decs(t : T?) {
    t |> run("plan_loop_or_count: decs take(N)._where(p).sum() (accumulator lane)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            // _select(price) → [10..80]. take(5) → [10,20,30,40,50]. _where(>25) → [30,40,50]. sum=120.
            // Threshold chosen above the first taken element so the gate actually filters — `_ > 0` would
            // pass all 5 and let a bypassed gate vacuously match the expected sum.
            let s = _fold(from_decs_template(type<DecsItem>)._select(_.price).take(5)._where(_ > 25).sum())
            tt |> equal(s, 120)
        }
        restart()
    }
}

[test]
def test_take_where_first_decs(t : T?) {
    t |> run("plan_loop_or_count: decs take(N)._where(p).first_or_default() (early-exit lane)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            let f = _fold(from_decs_template(type<DecsItem>).take(3)._where(_.category == "T").first_or_default((category = "", price = -1)))
            tt |> equal(f.price, 10)
        }
        restart()
    }
}

[test]
def test_take_where_to_array_decs(t : T?) {
    t |> run("plan_loop_or_count: decs take(N)._where(p).to_array() (array lane)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            let arr <- _fold(from_decs_template(type<DecsItem>).take(5)._where(_.category == "T").to_array())
            tt |> equal(length(arr), 3)
            tt |> equal(arr[0].price, 10)
            tt |> equal(arr[2].price, 50)
        }
        restart()
    }
}

[test]
def test_head_where_take_post_where_count_decs(t : T?) {
    t |> run("plan_loop_or_count: decs _where(P1)._take(N)._where(P2).count() (head + gate fire together)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            // _where(price > 15) drops [10] → [20,30,40,50,60,70,80]. take(4) → [20,30,40,50]. _where("T") → [30,50]. count=2.
            let r = _fold(from_decs_template(type<DecsItem>)._where(_.price > 15).take(4)._where(_.category == "T").count())
            tt |> equal(r, 2)
        }
        restart()
    }
}

[test]
def test_select_take_post_where_count_decs(t : T?) {
    t |> run("plan_loop_or_count: decs _select(f).take(N)._where(p).count() (gate peels against finalBind)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            // _select(price) → [10..80]. take(5) → [10,20,30,40,50]. _where(>25) → [30,40,50]. count=3.
            let r = _fold(from_decs_template(type<DecsItem>)._select(_.price).take(5)._where(_ > 25).count())
            tt |> equal(r, 3)
        }
        restart()
    }
}

[test]
def test_skip_take_post_where_count_decs(t : T?) {
    t |> run("plan_loop_or_count: decs skip(M).take(N)._where(p).count() (skip + post-take gate)") @(tt : T?) {
        restart()
        populate_decs_act_items()
        unsafe {
            // skip(2) → [30,40,50,60,70,80]. take(4) → [30,40,50,60]. _where("T") → [30,50]. count=2.
            let r = _fold(from_decs_template(type<DecsItem>).skip(2).take(4)._where(_.category == "T").count())
            tt |> equal(r, 2)
        }
        restart()
    }
}
