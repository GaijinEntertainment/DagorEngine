options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require dastest/testing_boost public

// Theme 8 (audit `benchmarks/sql/linq_fold_chain_audit.md`) — three small specialized fusion arms,
// each closing one previously-FALLS-OFF chain. Bundled because they are independent and small.
//
//   2a — `reverse + distinct_by` on array source (plan_reverse)
//        → single backward source walk with set-gated push. Saves cascade's reverse_to_array
//          allocation and second dedup pass.
//   3b — `distinct[_by] + order_by` without take, to_array (plan_order_family)
//        → single fused loop with dset gate, then sort in place. Saves cascade's
//          distinct_by_to_array intermediate iterator setup.
//   C4 — `zip + reverse` on array lane / counter lane / accumulator lane (plan_zip)
//        → trailing reverse_inplace on the spliced buffer. Skips zip_to_array iterator wrap;
//          identity on counter / sum/min/max/avg / any/all/contains. Bails on first/first_or_default.

// ═════════════════════════════════════════════════════════════════════════════
// 2a — reverse + distinct_by (array source)
// ═════════════════════════════════════════════════════════════════════════════

struct Theme8Event {
    kind : int
    val  : int
}

[test]
def test_2a_array_reverse_distinct_by(t : T?) {
    t |> run("2a: each(arr).reverse()._distinct_by(_.kind).to_array() — keep FIRST per kind in reversed order = LAST per kind in source") @(tt : T?) {
        var events : array<Theme8Event>
        events |> push(Theme8Event(kind = 1, val = 10))
        events |> push(Theme8Event(kind = 2, val = 20))
        events |> push(Theme8Event(kind = 1, val = 30))
        events |> push(Theme8Event(kind = 3, val = 40))
        events |> push(Theme8Event(kind = 2, val = 50))
        events |> push(Theme8Event(kind = 1, val = 60))
        // reverse → [(1,60),(2,50),(3,40),(1,30),(2,20),(1,10)]
        // distinct_by(kind) keeps FIRST per key in that order → [(1,60),(2,50),(3,40)]
        let got <- _fold(each(events).reverse()._distinct_by(_.kind).to_array())
        tt |> equal(length(got), 3)
        tt |> equal(got[0].kind, 1); tt |> equal(got[0].val, 60)
        tt |> equal(got[1].kind, 2); tt |> equal(got[1].val, 50)
        tt |> equal(got[2].kind, 3); tt |> equal(got[2].val, 40)
    }
}

[test]
def test_2a_array_reverse_distinct(t : T?) {
    t |> run("2a-distinct: each(arr).reverse().distinct().to_array() — whole-tuple dedup on reversed source") @(tt : T?) {
        var items : array<int>
        items |> push(1); items |> push(2); items |> push(1); items |> push(3); items |> push(2)
        // reverse → [2,3,1,2,1] → distinct (whole element) keeps first-seen-in-reversed-order → [2,3,1]
        let got <- _fold(each(items).reverse().distinct().to_array())
        tt |> equal(length(got), 3)
        tt |> equal(got[0], 2); tt |> equal(got[1], 3); tt |> equal(got[2], 1)
    }
}

def theme8_2a_handwritten(events : array<Theme8Event>) : array<Theme8Event> {
    var seen : table<int>
    var out : array<Theme8Event>
    for (k in 0 .. length(events)) {
        let e = events[length(events) - 1 - k]
        if (!key_exists(seen, e.kind)) {
            seen |> insert(e.kind)
            out |> push_clone(e)
        }
    }
    return <- out
}

[test]
def test_2a_parity_handwritten(t : T?) {
    t |> run("2a-parity: spliced vs handwritten backward walk — element-for-element match across 100 rows") @(tt : T?) {
        var events : array<Theme8Event>
        for (i in range(100)) {
            events |> push(Theme8Event(kind = i % 7, val = i))
        }
        let spliced <- _fold(each(events).reverse()._distinct_by(_.kind).to_array())
        let hand <- theme8_2a_handwritten(events)
        tt |> equal(length(spliced), length(hand))
        for (i in range(length(spliced))) {
            tt |> equal(spliced[i].kind, hand[i].kind)
            tt |> equal(spliced[i].val, hand[i].val)
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 3b — distinct[_by] + order_by, no take (to_array)
// ═════════════════════════════════════════════════════════════════════════════

struct Theme8Row {
    user_id : int
    ts      : int
    score   : int
}

[test]
def test_3b_distinct_by_order_by(t : T?) {
    t |> run("3b: each(rows)._distinct_by(_.user_id)._order_by(_.ts).to_array() — distinct gate + sort, no take") @(tt : T?) {
        var rows : array<Theme8Row>
        rows |> push(Theme8Row(user_id = 3, ts = 100, score = 1))
        rows |> push(Theme8Row(user_id = 1, ts = 200, score = 2))
        rows |> push(Theme8Row(user_id = 2, ts = 150, score = 3))
        rows |> push(Theme8Row(user_id = 1, ts = 300, score = 4))// dup user_id=1 → dropped
        rows |> push(Theme8Row(user_id = 3, ts = 50,  score = 5))// dup user_id=3 → dropped
        rows |> push(Theme8Row(user_id = 4, ts = 250, score = 6))
        // distinct_by keeps FIRST per user_id → [(3,100),(1,200),(2,150),(4,250)]
        // order_by(ts) → [(3,100),(2,150),(1,200),(4,250)]
        let got <- _fold(each(rows)._distinct_by(_.user_id)._order_by(_.ts).to_array())
        tt |> equal(length(got), 4)
        tt |> equal(got[0].user_id, 3); tt |> equal(got[0].ts, 100)
        tt |> equal(got[1].user_id, 2); tt |> equal(got[1].ts, 150)
        tt |> equal(got[2].user_id, 1); tt |> equal(got[2].ts, 200)
        tt |> equal(got[3].user_id, 4); tt |> equal(got[3].ts, 250)
    }
}

[test]
def test_3b_distinct_by_order_descending(t : T?) {
    t |> run("3b-desc: each(rows)._distinct_by(_.user_id)._order_by_descending(_.ts).to_array()") @(tt : T?) {
        var rows : array<Theme8Row>
        rows |> push(Theme8Row(user_id = 3, ts = 100, score = 1))
        rows |> push(Theme8Row(user_id = 1, ts = 200, score = 2))
        rows |> push(Theme8Row(user_id = 2, ts = 150, score = 3))
        rows |> push(Theme8Row(user_id = 4, ts = 250, score = 6))
        // distinct unchanged (no dupes), order_by_descending(ts)
        let got <- _fold(each(rows)._distinct_by(_.user_id)._order_by_descending(_.ts).to_array())
        tt |> equal(length(got), 4)
        tt |> equal(got[0].ts, 250); tt |> equal(got[1].ts, 200)
        tt |> equal(got[2].ts, 150); tt |> equal(got[3].ts, 100)
    }
}

[test]
def test_3b_distinct_whole_order_by(t : T?) {
    t |> run("3b-distinct: each(items).distinct()._order_by(_.score).to_array() — whole-tuple dedup + sort") @(tt : T?) {
        var items <- [
            (name = "a", score = 30),
            (name = "b", score = 10),
            (name = "a", score = 30),  // dup tuple → dropped
            (name = "c", score = 20),
            (name = "b", score = 10)   // dup tuple → dropped
        ]
        let got <- _fold(each(items).distinct()._order_by(_.score).to_array())
        tt |> equal(length(got), 3)
        tt |> equal(got[0].score, 10); tt |> equal(got[0].name, "b")
        tt |> equal(got[1].score, 20); tt |> equal(got[1].name, "c")
        tt |> equal(got[2].score, 30); tt |> equal(got[2].name, "a")
    }
}

[test]
def test_3b_where_then_distinct_then_order(t : T?) {
    t |> run("3b-where: each(rows)._where(...)._distinct_by(_.user_id)._order_by(_.ts).to_array() — where filters BEFORE distinct gate") @(tt : T?) {
        var rows : array<Theme8Row>
        rows |> push(Theme8Row(user_id = 1, ts = 50,  score = 5))// dropped by where
        rows |> push(Theme8Row(user_id = 2, ts = 200, score = 10))
        rows |> push(Theme8Row(user_id = 1, ts = 300, score = 15))
        rows |> push(Theme8Row(user_id = 2, ts = 100, score = 20))// dup user_id=2 after where → dropped
        rows |> push(Theme8Row(user_id = 3, ts = 150, score = 25))
        // where(score >= 10) → [(2,200),(1,300),(2,100),(3,150)]
        // distinct_by(user_id) keeps FIRST → [(2,200),(1,300),(3,150)]
        // order_by(ts) → [(3,150),(2,200),(1,300)]
        let got <- _fold(each(rows)._where(_.score >= 10)._distinct_by(_.user_id)._order_by(_.ts).to_array())
        tt |> equal(length(got), 3)
        tt |> equal(got[0].user_id, 3); tt |> equal(got[0].ts, 150)
        tt |> equal(got[1].user_id, 2); tt |> equal(got[1].ts, 200)
        tt |> equal(got[2].user_id, 1); tt |> equal(got[2].ts, 300)
    }
}

[test]
def test_3b_with_terminal_select(t : T?) {
    t |> run("3b-select: each(rows)._distinct_by(_.user_id)._order_by(_.ts)._select(_.user_id).to_array() — Theme 1 terminal _select composes") @(tt : T?) {
        var rows : array<Theme8Row>
        rows |> push(Theme8Row(user_id = 3, ts = 100, score = 1))
        rows |> push(Theme8Row(user_id = 1, ts = 200, score = 2))
        rows |> push(Theme8Row(user_id = 2, ts = 150, score = 3))
        let got <- _fold(each(rows)._distinct_by(_.user_id)._order_by(_.ts)._select(_.user_id).to_array())
        tt |> equal(length(got), 3)
        tt |> equal(got[0], 3); tt |> equal(got[1], 2); tt |> equal(got[2], 1)
    }
}

def theme8_3b_handwritten(rows : array<Theme8Row>) : array<Theme8Row> {
    var seen : table<int>
    var buf : array<Theme8Row>
    for (r in rows) {
        if (!key_exists(seen, r.user_id)) {
            seen |> insert(r.user_id)
            buf |> push_clone(r)
        }
    }
    sort(buf, $(a, b) => _::less(a.ts, b.ts))
    return <- buf
}

[test]
def test_3b_parity_handwritten(t : T?) {
    t |> run("3b-parity: spliced vs handwritten distinct+sort cascade — 100 rows, 12 distinct user_ids") @(tt : T?) {
        var rows : array<Theme8Row>
        for (i in range(100)) {
            rows |> push(Theme8Row(user_id = i % 12, ts = (i * 17) % 1000, score = i))
        }
        let spliced <- _fold(each(rows)._distinct_by(_.user_id)._order_by(_.ts).to_array())
        let hand <- theme8_3b_handwritten(rows)
        tt |> equal(length(spliced), length(hand))
        for (i in range(length(spliced))) {
            tt |> equal(spliced[i].user_id, hand[i].user_id)
            tt |> equal(spliced[i].ts, hand[i].ts)
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// C4 — zip + reverse (array lane / counter lane / accumulator lane)
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_c4_zip_reverse_to_array(t : T?) {
    t |> run("C4: each(a) |> zip(each(b)) |> reverse() |> to_array() — array lane, reverse_inplace tail") @(tt : T?) {
        var a : array<int>; var b : array<int>
        a |> push(1); a |> push(2); a |> push(3); a |> push(4)
        b |> push(10); b |> push(20); b |> push(30); b |> push(40)
        unsafe {
            let got <- _fold(each(a) |> zip(each(b)) |> reverse() |> to_array())
            tt |> equal(length(got), 4)
            tt |> equal(got[0]._0, 4); tt |> equal(got[0]._1, 40)
            tt |> equal(got[1]._0, 3); tt |> equal(got[1]._1, 30)
            tt |> equal(got[2]._0, 2); tt |> equal(got[2]._1, 20)
            tt |> equal(got[3]._0, 1); tt |> equal(got[3]._1, 10)
        }
    }
}

[test]
def test_c4_zip_reverse_count(t : T?) {
    t |> run("C4-count: zip + reverse + count — reverse is identity for count, no buf alloc") @(tt : T?) {
        var a : array<int>; var b : array<int>
        for (i in range(20)) { a |> push(i); }
        for (i in range(15)) { b |> push(i); }
        // zip stops at shorter (15); reverse is identity for count
        unsafe {
            let got = _fold(each(a) |> zip(each(b)) |> reverse() |> count())
            tt |> equal(got, 15)
        }
    }
}

[test]
def test_c4_zip_reverse_sum(t : T?) {
    t |> run("C4-sum: zip + select(sum-of-pair) + reverse + sum — accumulator lane, reverse identity") @(tt : T?) {
        var a : array<int>; var b : array<int>
        a |> push(1); a |> push(2); a |> push(3)
        b |> push(10); b |> push(20); b |> push(30)
        // pair sums: 11, 22, 33; sum after reverse = 66 (identity)
        unsafe {
            let got = _fold(each(a) |> zip(each(b)) |> _select(_._0 + _._1) |> reverse() |> sum())
            tt |> equal(got, 66)
        }
    }
}

[test]
def test_c4_zip_where_reverse(t : T?) {
    t |> run("C4-where: zip + where + reverse + to_array — where filters BEFORE reverse_inplace") @(tt : T?) {
        var a : array<int>; var b : array<int>
        for (i in range(6)) { a |> push(i); }
        for (i in range(6)) { b |> push(i * 10); }
        // pairs: (0,0),(1,10),(2,20),(3,30),(4,40),(5,50); where(_._0 % 2 == 0) → (0,0),(2,20),(4,40); reverse → (4,40),(2,20),(0,0)
        unsafe {
            let got <- _fold(each(a) |> zip(each(b)) |> _where(_._0 % 2 == 0) |> reverse() |> to_array())
            tt |> equal(length(got), 3)
            tt |> equal(got[0]._0, 4); tt |> equal(got[0]._1, 40)
            tt |> equal(got[1]._0, 2); tt |> equal(got[1]._1, 20)
            tt |> equal(got[2]._0, 0); tt |> equal(got[2]._1, 0)
        }
    }
}

def theme8_c4_handwritten(a : array<int>; b : array<int>) : array<tuple<int; int>> {
    var buf : array<tuple<int; int>>
    let n = min(length(a), length(b))
    buf |> reserve(n)
    for (i in range(n)) {
        buf |> push_clone((a[i], b[i]))
    }
    _::reverse_inplace(buf)
    return <- buf
}

[test]
def test_c4_parity_handwritten(t : T?) {
    t |> run("C4-parity: spliced vs handwritten zip+reverse — 50-pair match") @(tt : T?) {
        var a : array<int>; var b : array<int>
        for (i in range(50)) { a |> push(i * 3); }
        for (i in range(50)) { b |> push(i * 5); }
        unsafe {
            let spliced <- _fold(each(a) |> zip(each(b)) |> reverse() |> to_array())
            let hand <- theme8_c4_handwritten(a, b)
            tt |> equal(length(spliced), length(hand))
            for (i in range(length(spliced))) {
                tt |> equal(spliced[i]._0, hand[i]._0)
                tt |> equal(spliced[i]._1, hand[i]._1)
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Anti-tests — out-of-scope shapes still produce correct cascade output.
// (We do NOT assert "cascades to tier-2" — that's an AST-level assertion outside
//  the test framework — only that output is correct.)
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_anti_2a_with_take_cascades(t : T?) {
    t |> run("anti-2a-take: reverse + distinct_by + take — out of v1 scope, must still produce correct output via cascade") @(tt : T?) {
        var events : array<Theme8Event>
        events |> push(Theme8Event(kind = 1, val = 10))
        events |> push(Theme8Event(kind = 2, val = 20))
        events |> push(Theme8Event(kind = 1, val = 30))
        events |> push(Theme8Event(kind = 3, val = 40))
        let got <- _fold(each(events).reverse()._distinct_by(_.kind).take(2).to_array())
        tt |> equal(length(got), 2)
        // reverse → [(3,40),(1,30),(2,20),(1,10)] → distinct keeps first per kind → [(3,40),(1,30),(2,20)] → take 2
        tt |> equal(got[0].kind, 3); tt |> equal(got[0].val, 40)
        tt |> equal(got[1].kind, 1); tt |> equal(got[1].val, 30)
    }
}

[test]
def test_anti_3b_distinct_with_first_cascades(t : T?) {
    t |> run("anti-3b-first: distinct_by + order_by + first — out of v1 scope (streaming-min path has no dset hook), cascades correctly") @(tt : T?) {
        var rows : array<Theme8Row>
        rows |> push(Theme8Row(user_id = 1, ts = 200, score = 1))
        rows |> push(Theme8Row(user_id = 2, ts = 100, score = 2))
        rows |> push(Theme8Row(user_id = 1, ts = 50,  score = 3))
        let got = _fold(each(rows)._distinct_by(_.user_id)._order_by(_.ts).first())
        // distinct_by keeps FIRST per user → [(1,200),(2,100)]; order_by(ts) → [(2,100),(1,200)]; first = (2,100)
        tt |> equal(got.user_id, 2); tt |> equal(got.ts, 100)
    }
}

[test]
def test_anti_c4_zip_reverse_first_cascades(t : T?) {
    t |> run("anti-C4-first: zip + reverse + first — out of v1 scope (reverse is NOT identity for first), cascades correctly") @(tt : T?) {
        var a : array<int>; var b : array<int>
        a |> push(1); a |> push(2); a |> push(3)
        b |> push(10); b |> push(20); b |> push(30)
        unsafe {
            let got = _fold(each(a) |> zip(each(b)) |> reverse() |> first())
            // reverse → [(3,30),(2,20),(1,10)]; first = (3,30)
            tt |> equal(got._0, 3); tt |> equal(got._1, 30)
        }
    }
}

[test]
def test_anti_c4_zip_reverse_select_cascades(t : T?) {
    t |> run("anti-C4-not-last: zip + reverse + select — reverse not last chain op, splice bails (select after reverse), cascades correctly") @(tt : T?) {
        var a : array<int>; var b : array<int>
        a |> push(1); a |> push(2); a |> push(3)
        b |> push(10); b |> push(20); b |> push(30)
        unsafe {
            let got <- _fold(each(a) |> zip(each(b)) |> reverse() |> _select(_._0 + _._1) |> to_array())
            tt |> equal(length(got), 3)
            tt |> equal(got[0], 33); tt |> equal(got[1], 22); tt |> equal(got[2], 11)
        }
    }
}

