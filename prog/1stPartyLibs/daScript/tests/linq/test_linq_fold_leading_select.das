options gen2

require daslib/linq_fold
require daslib/linq_boost public
require dastest/testing_boost public

// A leading `_select(f)` projects the source before an order/distinct/take chain. The order-family splices
// previously bailed on it (the matcher had no leading-select slot), dropping the whole chain to tier-2. The
// `srcsel` slot + `ProjectedSourceAdapter` now absorb the projection so the fused single-pass emit fires.
// Each shape asserts the fused `_fold` result against hand-computed expected values; the un-fused chain (no
// `_fold`) is observationally equivalent, so these also guard fusion correctness.

struct Rec {
    k : int
    v : int
}

def make_recs() : array<Rec> {
    var a : array<Rec>
    a |> reserve(20)
    for (i in range(20)) {
        a |> emplace <| Rec(k = i % 5, v = i)   // k cycles 0..4, four of each
    }
    return <- a
}

[test]
def leading_select_order_distinct_take(t : T?) {
    // Row 3 (order_then_plain_distinct): distinct sorted keys, top 3.
    let recs <- make_recs()
    var got <- _fold(each(recs) |> _select(_.k) |> _order_by(_) |> distinct() |> take(3) |> to_array())
    t |> equal(length(got), 3)
    t |> equal(got[0], 0)
    t |> equal(got[1], 1)
    t |> equal(got[2], 2)
    delete got
}

[test]
def leading_select_order_take(t : T?) {
    // Row 2 (bounded_heap): sorted keys with dups, top 3 smallest.
    let recs <- make_recs()
    var got <- _fold(each(recs) |> _select(_.k) |> _order_by(_) |> take(3) |> to_array())
    t |> equal(length(got), 3)
    t |> equal(got[0], 0)
    t |> equal(got[1], 0)
    t |> equal(got[2], 0)
    delete got
}

[test]
def leading_select_where_order(t : T?) {
    // Row 4 (fused_prefilter): keys >= 2, sorted.
    let recs <- make_recs()
    var got <- _fold(each(recs) |> _select(_.k) |> _where(_ >= 2) |> _order_by(_) |> to_array())
    t |> equal(length(got), 12)   // {2,3,4} x4 each
    t |> equal(got[0], 2)
    t |> equal(got[length(got) - 1], 4)
    var prev = -1
    for (x in got) {
        t |> success(x >= prev, "sorted ascending")
        t |> success(x >= 2, "where filtered")
        prev = x
    }
    delete got
}

[test]
def leading_select_order_first(t : T?) {
    // Row 1 (streaming_min): min projected key.
    let recs <- make_recs()
    let got = _fold(each(recs) |> _select(_.k) |> _order_by(_) |> first())
    t |> equal(got, 0)
}

[test]
def leading_select_no_regression_plain_order(t : T?) {
    // A chain WITHOUT a leading select must still fuse + produce the same order (srcsel slot is optional).
    let recs <- make_recs()
    var got <- _fold(each(recs) |> _order_by(_.k) |> take(3) |> to_array())
    t |> equal(length(got), 3)
    for (r in got) {
        t |> equal(r.k, 0)
    }
    delete got
}
