options gen2
require daslib/linq_boost public
require daslib/linq_fold
require dastest/testing_boost public

// Mixed-source *_to_array overloads: every binary linq op that materializes to an array now accepts
// ONE array + ONE iterator source in both directions (not just both-array or both-iterator). These
// are what the _fold binary-op array-rewrite needs — `_fold(each(a) |> _join(arr) |> count())`
// lowers to join_to_array(iterator, array, ...), which previously failed to resolve. Each mixed
// _to_array delegates to the same *_impl_const as the mixed iterator-returning overload, so each
// test asserts the array result (both directions) equals the all-array ground truth, and the final
// test exercises the _fold consumer directly (count and to_array terminals).

struct L { id : int; k : int }
struct R { k : int; v : int }

def fixture_a() : array<int> {
    return <- [1, 2, 3, 3]
}
def fixture_b() : array<int> {
    return <- [3, 4, 4, 5]
}
def fixture_l() : array<L> {
    return <- [L(id = 1, k = 1), L(id = 2, k = 2), L(id = 3, k = 1)]
}
def fixture_r() : array<R> {
    return <- [R(k = 1, v = 10), R(k = 2, v = 20), R(k = 1, v = 30)]
}

// Both lanes run the same *_impl_const, so output order matches the array ground truth — a direct
// element-wise compare (not a checksum) is valid. `got` is moved in and deleted here.
def same_arr(t : T?; name : string; ref : array<int>; var got : array<int>) {
    t |> equal(length(got), length(ref), name)
    if (length(got) == length(ref)) {
        for (i in range(length(ref))) {
            t |> equal(got[i], ref[i], name)
        }
    }
    delete got
}

[test]
def test_concat_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> concat(b))
    same_arr(t, "concat_to_array iter+array", ref, concat_to_array(unsafe(each(a)), b))
    same_arr(t, "concat_to_array array+iter", ref, concat_to_array(a, unsafe(each(b))))
}

[test]
def test_union_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> union(b))
    same_arr(t, "union_to_array iter+array", ref, union_to_array(unsafe(each(a)), b))
    same_arr(t, "union_to_array array+iter", ref, union_to_array(a, unsafe(each(b))))
}

[test]
def test_union_by_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> union_by(b, $(x : int) => x % 10))
    same_arr(t, "union_by_to_array iter+array", ref, union_by_to_array(unsafe(each(a)), b, $(x : int) => x % 10))
    same_arr(t, "union_by_to_array array+iter", ref, union_by_to_array(a, unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_intersect_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> intersect(b))
    same_arr(t, "intersect_to_array iter+array", ref, intersect_to_array(unsafe(each(a)), b))
    same_arr(t, "intersect_to_array array+iter", ref, intersect_to_array(a, unsafe(each(b))))
}

[test]
def test_intersect_by_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> intersect_by(b, $(x : int) => x % 10))
    same_arr(t, "intersect_by_to_array iter+array", ref, intersect_by_to_array(unsafe(each(a)), b, $(x : int) => x % 10))
    same_arr(t, "intersect_by_to_array array+iter", ref, intersect_by_to_array(a, unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_except_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> except(b))
    same_arr(t, "except_to_array iter+array", ref, except_to_array(unsafe(each(a)), b))
    same_arr(t, "except_to_array array+iter", ref, except_to_array(a, unsafe(each(b))))
}

[test]
def test_except_by_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let ref <- (a |> except_by(b, $(x : int) => x % 10))
    same_arr(t, "except_by_to_array iter+array", ref, except_by_to_array(unsafe(each(a)), b, $(x : int) => x % 10))
    same_arr(t, "except_by_to_array array+iter", ref, except_by_to_array(a, unsafe(each(b)), $(x : int) => x % 10))
}

[test]
def test_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
    same_arr(t, "join_to_array iter+array", ref,
        join_to_array(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
    same_arr(t, "join_to_array array+iter", ref,
        join_to_array(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : R) => l.id + r.v))
}

[test]
def test_group_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (group_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
    same_arr(t, "group_join_to_array iter+array", ref,
        group_join_to_array(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
    same_arr(t, "group_join_to_array array+iter", ref,
        group_join_to_array(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, rs : array<R>) => l.id + length(rs)))
}

[test]
def test_left_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (left_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
    same_arr(t, "left_join_to_array iter+array", ref,
        left_join_to_array(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
    same_arr(t, "left_join_to_array array+iter", ref,
        left_join_to_array(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : L, r : Option<R>) => l.id + (r |> is_some ? 1 : 0)))
}

[test]
def test_right_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (right_join(a, b, $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
    same_arr(t, "right_join_to_array iter+array", ref,
        right_join_to_array(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
    same_arr(t, "right_join_to_array array+iter", ref,
        right_join_to_array(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k, $(l : Option<L>, r : R) => (l |> is_some ? 1 : 0) + r.v))
}

[test]
def test_full_outer_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (full_outer_join(a, b, $(l : L) => l.k, $(r : R) => r.k,
        $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
    same_arr(t, "full_outer_join_to_array iter+array", ref,
        full_outer_join_to_array(unsafe(each(a)), b, $(l : L) => l.k, $(r : R) => r.k,
            $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
    same_arr(t, "full_outer_join_to_array array+iter", ref,
        full_outer_join_to_array(a, unsafe(each(b)), $(l : L) => l.k, $(r : R) => r.k,
            $(l : Option<L>, r : Option<R>) => (l |> is_some ? 1 : 0) + (r |> is_some ? 1 : 0)))
}

[test]
def test_cross_join_to_array(t : T?) {
    let a <- fixture_l(); let b <- fixture_r()
    let ref <- (cross_join(a, b, $(l : L, r : R) => l.id * r.v))
    same_arr(t, "cross_join_to_array iter+array", ref,
        cross_join_to_array(unsafe(each(a)), b, $(l : L, r : R) => l.id * r.v))
    same_arr(t, "cross_join_to_array array+iter", ref,
        cross_join_to_array(a, unsafe(each(b)), $(l : L, r : R) => l.id * r.v))
}

[test]
def test_fold_join_consumer(t : T?) {
    // The reason these overloads exist: _fold lowers `_join |> count()` to join_to_array(iterator,
    // array, ...) internally — without the mixed overload this fails to resolve. Both directions.
    let a <- fixture_l(); let b <- fixture_r()
    let c1 = _fold(unsafe(each(a)) |> _join(b, $(l : L, r : R) => l.k == r.k, $(l : L, r : R) => l.id + r.v) |> count())
    let c2 = _fold(a |> _join(unsafe(each(b)), $(l : L, r : R) => l.k == r.k, $(l : L, r : R) => l.id + r.v) |> count())
    t |> equal(c1, 5)
    t |> equal(c2, 5)
}
