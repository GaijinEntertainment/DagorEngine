options gen2

require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require dastest/testing_boost public

// KR-1 regression coverage — PR B collapse_chained_wheres pre-pass.
//
// Before PR B, chains like `..._where(p1)._where(p2).reverse()...` no longer spliced through
// `plan_reverse` / `plan_distinct` (single optional `where_` slot in the pattern row vs N-fold
// accept-and-merge in master's imperative code). Cascade still worked — correctness was OK,
// but a perf regression on uncommon chains. The decs mirror `plan_decs_reverse` always
// composed via `merge_where_cond` in a loop, so this is a parity restoration.
//
// These tests verify the COMPOSED predicate produces the same result as the manual
// `_where(p1 && p2)` form. Splice-firing itself isn't asserted from runtime (would need AST
// inspection); we rely on the per-archetype test files to exercise the emit paths and these
// tests to assert composition correctness end-to-end.

// ═════════════════════════════════════════════════════════════════════════════
// 1. plan_reverse surface
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_chained_wheres_reverse_n2(t : T?) {
    t |> run("chained wheres N=2 + reverse + to_array: composed pred matches manual && form") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let split <- _fold(each(arr)._where(_ > 2)._where(_ < 8).reverse() |> to_array())
        let combined <- _fold(each(arr)._where(_ > 2 && _ < 8).reverse() |> to_array())
        tt |> equal(length(split), length(combined))
        tt |> equal(length(split), 5)        // 3,4,5,6,7 reversed
        for (i in 0 .. length(split)) {
            tt |> equal(split[i], combined[i])
        }
        tt |> equal(split[0], 7)
        tt |> equal(split[4], 3)
    }
}

[test]
def test_chained_wheres_reverse_n3(t : T?) {
    t |> run("chained wheres N=3 + reverse + to_array: all three compose into single && chain") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let split <- _fold(each(arr)._where(_ > 1)._where(_ < 9)._where(_ % 2 == 0).reverse() |> to_array())
        let combined <- _fold(each(arr)._where(_ > 1 && _ < 9 && _ % 2 == 0).reverse() |> to_array())
        // _ > 1 && _ < 9 && _ % 2 == 0 → [2, 4, 6, 8]; reversed → [8, 6, 4, 2]
        tt |> equal(length(split), length(combined))
        tt |> equal(length(split), 4)
        for (i in 0 .. length(split)) {
            tt |> equal(split[i], combined[i])
        }
        tt |> equal(split[0], 8)
        tt |> equal(split[3], 2)
    }
}

[test]
def test_chained_wheres_reverse_first(t : T?) {
    t |> run("chained wheres + reverse + first: scalar terminator splice (Rb archetype) sees composed pred") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let v = _fold(each(arr)._where(_ > 3)._where(_ < 9).reverse().first())
        // _ > 3 && _ < 9 → [4,5,6,7,8]; reversed: [8,7,6,5,4]; first = 8
        tt |> equal(v, 8)
    }
}

[test]
def test_chained_wheres_reverse_count(t : T?) {
    t |> run("chained wheres + reverse + count: counter terminator (Ra archetype) — reverse is identity for count") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let cnt = _fold(each(arr)._where(_ > 2)._where(_ < 8).reverse().count())
        tt |> equal(cnt, 5)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 2. plan_distinct surface
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_chained_wheres_distinct_n2(t : T?) {
    t |> run("chained wheres N=2 + distinct + to_array: composed pred matches manual && form") @(tt : T?) {
        let arr <- [1, 2, 1, 3, 2, 4, 3, 5, 4, 6]
        let split <- _fold(each(arr)._where(_ > 1)._where(_ < 6) |> distinct() |> to_array())
        let combined <- _fold(each(arr)._where(_ > 1 && _ < 6) |> distinct() |> to_array())
        tt |> equal(length(split), length(combined))
        tt |> equal(length(split), 4)        // 2,3,4,5 first-occurrence
        for (i in 0 .. length(split)) {
            tt |> equal(split[i], combined[i])
        }
    }
}

[test]
def test_chained_wheres_distinct_n3(t : T?) {
    t |> run("chained wheres N=3 + distinct + count: all three compose; count fast-path") @(tt : T?) {
        let arr <- [1, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8]
        let cnt = _fold(each(arr)._where(_ > 1)._where(_ < 8)._where(_ != 4) |> distinct() |> count())
        // _ > 1 && _ < 8 && _ != 4 → 2,3,5,6,7 distinct → 5
        tt |> equal(cnt, 5)
    }
}

[test]
def test_chained_wheres_distinct_take(t : T?) {
    t |> run("chained wheres + distinct + take(N): bounded splice path with composed pred") @(tt : T?) {
        let arr <- [1, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8]
        let buf <- _fold(each(arr)._where(_ > 1)._where(_ < 8) |> distinct() |> take(3) |> to_array())
        // _ > 1 && _ < 8 distinct first-N=3 → [2,3,4]
        tt |> equal(length(buf), 3)
        tt |> equal(buf[0], 2)
        tt |> equal(buf[1], 3)
        tt |> equal(buf[2], 4)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 3. Edge cases: no collapse when call between, single where unaffected
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_single_where_unchanged(t : T?) {
    t |> run("single where unchanged: collapse pre-pass is a no-op on chains without adjacent wheres") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let buf <- _fold(each(arr)._where(_ > 2).reverse() |> to_array())
        tt |> equal(length(buf), 3)
        tt |> equal(buf[0], 5)
        tt |> equal(buf[2], 3)
    }
}

[test]
def test_wheres_split_by_select_no_collapse(t : T?) {
    t |> run("wheres with select between: NOT collapsed (collapse only fires on ADJACENT wheres)") @(tt : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // After collapse_chained_selects (no chained selects here) + collapse_chained_wheres
        // (wheres not adjacent — select sits between): chain stays as [where, select, where, reverse, to_array].
        // plan_reverse's pattern rows expect a single optional `where_` slot before `reverse` — this chain
        // has a where AFTER select that the pattern can't match. Cascade fires; correctness preserved.
        let buf <- _fold(each(arr)._where(_ > 2)._select(_ * 10)._where(_ < 80).reverse() |> to_array())
        // _ > 2: 3..10 (×10: 30..100); _ < 80: 30..70; reversed: 70,60,50,40,30
        tt |> equal(length(buf), 5)
        tt |> equal(buf[0], 70)
        tt |> equal(buf[4], 30)
    }
}
