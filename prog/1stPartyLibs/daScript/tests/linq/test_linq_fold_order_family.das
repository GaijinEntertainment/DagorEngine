options gen2

require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require dastest/testing_boost public

// PR B2 — per-archetype coverage for the migrated plan_order_family. The 5 rows
// (streaming_min / bounded_heap / order_then_plain_distinct / fused_prefilter /
// buffer_helper_dispatch) are exercised end-to-end by the existing per-feature
// test suites (test_linq_sorting, theme3_c1_c5, theme45, theme8, terminal_select).
// These tests assert the behaviors that historically caught regressions:
//
// 1. normalize_order_reverse path: chains like `_order_by(_).reverse().take(N)`
//    must produce descending top-N. After PR B1's pattern table, `single_name`
//    preserves the post-normalize LinqCall.name so emit fns see the swap.
// 2. KR-1 closure: chained wheres before order_family must compose (collapse_
//    chained_wheres pre-pass).
// 3. Row 3 (order_then_plain_distinct) must accept plain `distinct` after order
//    (whole-tuple equality is position-invariant) but reject distinct_by (would
//    pick an arbitrary K1 representative).

// ─── 1. normalize_order_reverse parity ───

[test]
def test_order_by_reverse_take_via_pattern_table(t : T?) {
    t |> run("order_by + reverse + take splices to descending top-N (single_name preserves the normalize swap)") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            let top3 <- _fold(each(scores)._order_by(_).reverse().take(3).to_array())
            tt |> equal(length(top3), 3)
            tt |> equal(top3[0], 9)
            tt |> equal(top3[1], 6)
            tt |> equal(top3[2], 5)
        }
    }
}

[test]
def test_order_by_descending_reverse_first_via_pattern_table(t : T?) {
    t |> run("order_by_descending + reverse + first splices to ascending min (streaming_min path post-normalize)") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            let lo = _fold(each(scores)._order_by_descending(_).reverse().first())
            tt |> equal(lo, 1)
        }
    }
}

// ─── 2. KR-1: collapse_chained_wheres + order_family ───

[test]
def test_chained_wheres_then_order_take(t : T?) {
    t |> run("two consecutive _where calls compose via collapse_chained_wheres, then route through fused_prefilter") @(tt : T?) {
        let scores <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        unsafe {
            // _ > 2 → [3..10]; _ < 9 → [3..8]; order_by → [3,4,5,6,7,8]; take(3) → [3,4,5].
            let buf <- _fold(each(scores)._where(_ > 2)._where(_ < 9)._order_by(_).take(3).to_array())
            tt |> equal(length(buf), 3)
            tt |> equal(buf[0], 3)
            tt |> equal(buf[1], 4)
            tt |> equal(buf[2], 5)
        }
    }
}

// ─── 3. Row 5 — order_then_plain_distinct (plain distinct only; distinct_by NOT in scope) ───

[test]
def test_order_then_plain_distinct(t : T?) {
    t |> run("order_by then plain distinct: whole-tuple equality is position-invariant; deduped sorted result") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            let buf <- _fold(each(scores)._order_by(_) |> distinct() |> to_array())
            // Ascending [1,1,2,3,3,4,5,5,5,6,9] then distinct → [1,2,3,4,5,6,9].
            tt |> equal(length(buf), 7)
            tt |> equal(buf[0], 1)
            tt |> equal(buf[1], 2)
            tt |> equal(buf[6], 9)
        }
    }
}

// ─── 4. Streaming-min spot checks ───

[test]
def test_streaming_min_with_where(t : T?) {
    t |> run("where + order_by + first_or_default: streaming-min with prefilter, default on empty") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            let lo = _fold(each(scores)._where(_ > 4)._order_by(_).first_or_default(-1))
            // _ > 4 → [5,9,6,5,5]; ascending min → 5
            tt |> equal(lo, 5)
            var empty_arr : array<int>
            let dflt = _fold(each(empty_arr)._where(_ > 4)._order_by(_).first_or_default(-1))
            tt |> equal(dflt, -1)
        }
    }
}

// ─── 5. Bounded-heap with distinct gate ───

[test]
def test_bounded_heap_with_distinct(t : T?) {
    t |> run("distinct + order_by + take: bounded-heap with dset gate; single-pass dedup-then-top-N") @(tt : T?) {
        let scores <- [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        unsafe {
            let top3 <- _fold(each(scores) |> distinct() |> _order_by(_) |> take(3) |> to_array())
            // distinct [3,1,4,5,9,2,6] → ascending [1,2,3,4,5,6,9] → top 3 → [1,2,3]
            tt |> equal(length(top3), 3)
            tt |> equal(top3[0], 1)
            tt |> equal(top3[1], 2)
            tt |> equal(top3[2], 3)
        }
    }
}
