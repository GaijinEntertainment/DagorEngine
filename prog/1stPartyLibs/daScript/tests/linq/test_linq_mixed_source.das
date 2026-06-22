options gen2
require daslib/linq public
require daslib/linq_boost public
require daslib/linq_fold
require dastest/testing_boost public

// Mixed-source overloads: every binary two-source linq op (set-ops, join family, sequence_equal)
// accepts ONE array and ONE iterator source, not just both-array or both-iterator. Each test runs
// the op three ways — all-array (ground truth), (iterator, array), and (array, iterator) — and
// asserts the mixed results equal the array result. The iterator side is produced with each(...).

struct L { id : int; k : int }
struct R { k : int; v : int }

def fixture_ints_a() : array<int> {
    return <- [1, 2, 3, 3]
}
def fixture_ints_b() : array<int> {
    return <- [3, 4, 4, 5]
}
def fixture_l() : array<L> {
    return <- [L(id = 1, k = 1), L(id = 2, k = 2), L(id = 3, k = 1)]
}
def fixture_r() : array<R> {
    return <- [R(k = 1, v = 10), R(k = 2, v = 20), R(k = 1, v = 30)]
}

// Assert an iterator result (mixed lane) matches the array ground truth element-wise. Both lanes
// run the same *_impl, so output order is identical — a direct compare (not a checksum) is valid.
def same(t : T?; name : string; refArr : array<int>; var got : iterator<int -const>) {
    var g <- got |> to_array()
    t |> equal(length(g), length(refArr), name)
    if (length(g) == length(refArr)) {
        for (i in range(length(refArr))) {
            t |> equal(g[i], refArr[i], name)
        }
    }
    delete g
}

[test]
def test_mixed_concat(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> concat(b))
    same(t, "concat iter+array", refArr, unsafe(each(a)) |> concat(b))
    same(t, "concat array+iter", refArr, a |> concat(unsafe(each(b))))
}

[test]
def test_mixed_union(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> union(b))
    same(t, "union iter+array", refArr, unsafe(each(a)) |> union(b))
    same(t, "union array+iter", refArr, a |> union(unsafe(each(b))))
}

[test]
def test_mixed_union_by(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> union_by(b, $(x : int) => x % 10))
    same(t, "union_by iter+array", refArr, unsafe(each(a)) |> union_by(b, $(x : int) => x % 10))
    same(t, "union_by array+iter", refArr, a |> union_by(unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_mixed_intersect(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> intersect(b))
    same(t, "intersect iter+array", refArr, unsafe(each(a)) |> intersect(b))
    same(t, "intersect array+iter", refArr, a |> intersect(unsafe(each(b))))
}

[test]
def test_mixed_intersect_by(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> intersect_by(b, $(x : int) => x % 10))
    same(t, "intersect_by iter+array", refArr, unsafe(each(a)) |> intersect_by(b, $(x : int) => x % 10))
    same(t, "intersect_by array+iter", refArr, a |> intersect_by(unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_mixed_except(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> except(b))
    same(t, "except iter+array", refArr, unsafe(each(a)) |> except(b))
    same(t, "except array+iter", refArr, a |> except(unsafe(each(b))))
}

[test]
def test_mixed_except_by(t : T?) {
    let a <- fixture_ints_a(); let b <- fixture_ints_b()
    let refArr <- (a |> except_by(b, $(x : int) => x % 10))
    same(t, "except_by iter+array", refArr, unsafe(each(a)) |> except_by(b, $(x : int) => x % 10))
    same(t, "except_by array+iter", refArr, a |> except_by(unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_mixed_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
    same(t, "join iter+array", refArr,
        join(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
    same(t, "join array+iter", refArr,
        join(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
}

[test]
def test_mixed_join_macro(t : T?) {
    // The _join shorthand macro forwards to join(...), so it picks up the mixed overload automatically.
    let a <- fixture_l(); let b <- fixture_r()
    let c1 = _fold(unsafe(each(a)) |> _join(b, $(l : L, r : R) => l.k == r.k, $(l : L, r : R) => l.id + r.v) |> count())
    let c2 = _fold(a |> _join(unsafe(each(b)), $(l : L, r : R) => l.k == r.k, $(l : L, r : R) => l.id + r.v) |> count())
    t |> equal(c1, 5)
    t |> equal(c2, 5)
}

[test]
def test_mixed_group_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (group_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
    same(t, "group_join iter+array", refArr,
        group_join(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
    same(t, "group_join array+iter", refArr,
        group_join(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
}

[test]
def test_mixed_left_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (left_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
    same(t, "left_join iter+array", refArr,
        left_join(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
    same(t, "left_join array+iter", refArr,
        left_join(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
}

[test]
def test_mixed_right_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (right_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
    same(t, "right_join iter+array", refArr,
        right_join(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
    same(t, "right_join array+iter", refArr,
        right_join(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
}

[test]
def test_mixed_full_outer_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (full_outer_join(a, b, $(l : L) => l.k, $(r : R) => r.k,
        $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
    same(t, "full_outer_join iter+array", refArr,
        full_outer_join(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k,
            $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
    same(t, "full_outer_join array+iter", refArr,
        full_outer_join(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k,
            $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
}

[test]
def test_mixed_cross_join(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let refArr <- (cross_join(a, b, $(l : L, r : R) => l.id * r.v))
    same(t, "cross_join iter+array", refArr,
        cross_join(unsafe(each(a)), b, $(l : L, r : R) => l.id * r.v))
    same(t, "cross_join array+iter", refArr,
        cross_join(a, unsafe(each(b)), $(l : L, r : R) => l.id * r.v))
}

[test]
def test_mixed_sequence_equal(t : T?) {
    let a <- fixture_ints_a(); let a2 <- fixture_ints_a(); let b <- fixture_ints_b()
    // equal sequences
    t |> success(sequence_equal(unsafe(each(a)), a2), "sequence_equal iter+array (equal)")
    t |> success(sequence_equal(a, unsafe(each(a2))), "sequence_equal array+iter (equal)")
    // unequal sequences
    t |> success(!sequence_equal(unsafe(each(a)), b), "sequence_equal iter+array (unequal)")
    t |> success(!sequence_equal(a, unsafe(each(b))), "sequence_equal array+iter (unequal)")
}

[test]
def test_mixed_sequence_equal_by(t : T?) {
    let a <- fixture_ints_a(); let a2 <- fixture_ints_a(); let b <- fixture_ints_b()
    t |> success(sequence_equal_by(unsafe(each(a)), a2, $(x : int) => x), "sequence_equal_by iter+array (equal)")
    t |> success(sequence_equal_by(a, unsafe(each(a2)), $(x : int) => x), "sequence_equal_by array+iter (equal)")
    t |> success(!sequence_equal_by(unsafe(each(a)), b, $(x : int) => x), "sequence_equal_by iter+array (unequal)")
    t |> success(!sequence_equal_by(a, unsafe(each(b)), $(x : int) => x), "sequence_equal_by array+iter (unequal)")
}
