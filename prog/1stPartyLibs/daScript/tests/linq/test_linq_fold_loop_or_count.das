options gen2

require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require dastest/testing_boost public

// PR B1 regression coverage — plan_loop_or_count migrated to pattern-table.
//
// The new architecture: `slot_chain_of(["where_", "select"], "head")` greedy-consumes the
// pre-range head; canonical-order positional slots (skip / skip_while / take_while / take /
// post_take_where / term) carry the rest. The emit fn (emit_loop_or_count_lane) walks
// c.many["head"] applying the same where-after-select / chained-select / AND-merge logic the
// imperative loop did, then dispatches to the same lane emit fns.
//
// These tests assert end-to-end correctness across the lane × head-shape matrix.

// ═════════════════════════════════════════════════════════════════════════════
// 1. Canonical chains — one lane per terminator family
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_counter_lane(t : T?) {
    t |> run("counter lane: where + count") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let n = _fold(each(arr)._where(_ > 3).count())
        tt |> equal(n, 7)
    }
}

[test]
def test_array_lane(t : T?) {
    t |> run("array lane: select + implicit to_array") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let buf <- _fold(each(arr)._select(_ * 10) |> to_array())
        tt |> equal(length(buf), 5)
        tt |> equal(buf[0], 10)
        tt |> equal(buf[4], 50)
    }
}

[test]
def test_accumulator_lane(t : T?) {
    t |> run("accumulator lane: where + select + sum") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let s = _fold(each(arr)._where(_ > 2)._select(_ * 2).sum())
        // 3+4+5+6+7+8+9+10 = 52; ×2 = 104
        tt |> equal(s, 104)
    }
}

[test]
def test_early_exit_lane(t : T?) {
    t |> run("early-exit lane: where + any") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let yes = _fold(each(arr)._where(_ > 3).any())
        let no = _fold(each(arr)._where(_ > 100).any())
        tt |> equal(yes, true)
        tt |> equal(no, false)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 2. Where-after-select rebinding (single & multiple)
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_where_after_select(t : T?) {
    t |> run("where(p2) after select(f) sees PROJECTED value, not raw source") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // _ > 1 keeps [2,3,4,5]; project ×10 → [20,30,40,50]; then keep _%20==0 → [20,40]; count=2.
        let n = _fold(each(arr)._where(_ > 1)._select(_ * 10)._where(_ % 20 == 0).count())
        tt |> equal(n, 2)
    }
}

[test]
def test_multiple_wheres_post_select(t : T?) {
    t |> run("two wheres after a select: collapse_chained_wheres composes them") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // ×2 → [2..20], _>5 keeps [6,8,10,12,14,16,18,20], _%4==0 keeps [8,12,16,20] = 4.
        let n = _fold(each(arr)._select(_ * 2)._where(_ > 5)._where(_ % 4 == 0).count())
        tt |> equal(n, 4)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 3. Range op chains (skip / take / take_while)
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_skip_take_count(t : T?) {
    t |> run("where + skip(N) + take(M) + count") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // _ > 0 keeps all; skip 3 → [4..10]; take 4 → [4,5,6,7]; count=4.
        let n = _fold(each(arr)._where(_ > 0).skip(3).take(4).count())
        tt |> equal(n, 4)
    }
}

[test]
def test_take_while_sum(t : T?) {
    t |> run("take_while + sum: streams until predicate fails") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6]
        // _take_while(_<4): [1,2,3]; sum=6.
        let s = _fold(each(arr)._take_while(_ < 4).sum())
        tt |> equal(s, 6)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 4. Post-take where (Theme 2 5c — gates contribution; take cap still ticks)
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_post_take_where(t : T?) {
    t |> run("take(N)._where(p): take fires unconditionally, where gates per-element acc") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // take(5) → [1..5]; _%2==0 keeps [2,4]; count=2.
        let n = _fold(each(arr).take(5)._where(_ % 2 == 0).count())
        tt |> equal(n, 2)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 5. Fast paths — length shortcut + any-empty shortcut on length-bearing source
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_length_shortcut(t : T?) {
    t |> run("count() on bare length-bearing source: emit_length_shortcut") @(tt : T?) {
        let arr <- [10, 20, 30, 40, 50]
        let n = _fold(each(arr).count())
        tt |> equal(n, 5)
    }
}

[test]
def test_any_empty_shortcut(t : T?) {
    t |> run("any() no-pred on bare length-bearing source: emit_any_empty_shortcut") @(tt : T?) {
        let arr <- [1, 2, 3]
        var empty_arr : array<int>
        tt |> equal(_fold(each(arr).any()), true)
        tt |> equal(_fold(each(empty_arr).any()), false)
    }
}
