options gen2
require daslib/linq public
require daslib/linq_boost public
require daslib/linq_fold
require dastest/testing_boost public

// Mixed-source zip overloads: zip / zip_to_array accept ONE array and ONE iterator source, not just
// both-array or both-iterator. Each test runs the op three ways — all-array (ground truth),
// (iterator, array), and (array, iterator) — and asserts the mixed results match. The iterator side
// is produced with each(...). `a` is longer than `b` so every lane also exercises lockstep stop at
// the shorter source. The _fold lane exercises the fold's array-rewrite, which lowers an
// iterator-sourced zip to zip_to_array(iterator, array) — the path that needed the mixed overload.

def fixture_a() : array<int> {
    return <- [10, 20, 30, 40]
}
def fixture_b() : array<int> {
    return <- [1, 2, 3]
}

// Compare two tuple<int;int> arrays element-wise. All zip lanes share zip_impl, so order is
// identical — a direct compare (not a checksum) is valid. auto(TT) dodges the exact tuple spelling.
def same_pairs(t : T?; name : string; refArr : array<auto(TT)>; gotArr : array<TT>) {
    t |> equal(length(gotArr), length(refArr), name)
    if (length(gotArr) == length(refArr)) {
        for (i in range(length(refArr))) {
            t |> equal(gotArr[i]._0, refArr[i]._0, name)
            t |> equal(gotArr[i]._1, refArr[i]._1, name)
        }
    }
}

def same_ints(t : T?; name : string; refArr : array<int>; gotArr : array<int>) {
    t |> equal(length(gotArr), length(refArr), name)
    if (length(gotArr) == length(refArr)) {
        for (i in range(length(refArr))) {
            t |> equal(gotArr[i], refArr[i], name)
        }
    }
}

[test]
def test_mixed_zip(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let refArr <- zip(a, b)
    var m1 <- zip(unsafe(each(a)), b) |> to_array()
    var m2 <- zip(a, unsafe(each(b))) |> to_array()
    same_pairs(t, "zip iter+array", refArr, m1)
    same_pairs(t, "zip array+iter", refArr, m2)
    delete m1; delete m2
}

[test]
def test_mixed_zip_selector(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let refSum <- [for (p in zip(a, b)); p._0 + p._1]
    var g1 <- zip(unsafe(each(a)), b, $(x : int; y : int) => x + y) |> to_array()
    var g2 <- zip(a, unsafe(each(b)), $(x : int; y : int) => x + y) |> to_array()
    same_ints(t, "zip selector iter+array", refSum, g1)
    same_ints(t, "zip selector array+iter", refSum, g2)
    delete g1; delete g2
}

[test]
def test_mixed_zip_to_array(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let refArr <- zip(a, b)
    var m1 <- zip_to_array(unsafe(each(a)), b)
    var m2 <- zip_to_array(a, unsafe(each(b)))
    same_pairs(t, "zip_to_array iter+array", refArr, m1)
    same_pairs(t, "zip_to_array array+iter", refArr, m2)
    delete m1; delete m2
}

[test]
def test_mixed_zip_to_array_selector(t : T?) {
    let a <- fixture_a(); let b <- fixture_b()
    let refSum <- [for (p in zip(a, b)); p._0 + p._1]
    var g1 <- zip_to_array(unsafe(each(a)), b, $(x : int; y : int) => x + y)
    var g2 <- zip_to_array(a, unsafe(each(b)), $(x : int; y : int) => x + y)
    same_ints(t, "zip_to_array selector iter+array", refSum, g1)
    same_ints(t, "zip_to_array selector array+iter", refSum, g2)
    delete g1; delete g2
}

[test]
def test_mixed_zip_fold(t : T?) {
    // _fold over an iterator-sourced zip with a bare-array operand: the fold's array-rewrite lowers
    // this to zip_to_array(iterator, array). count stops at min(4,3)=3; sum = (10+1)+(20+2)+(30+3)=66.
    let a <- fixture_a(); let b <- fixture_b()
    let cnt = _fold(zip(unsafe(each(a)), b).count())
    let s = _fold(zip(unsafe(each(a)), b)._select(_._0 + _._1).sum())
    t |> equal(cnt, 3, "zip fold count")
    t |> equal(s, 66, "zip fold sum")
    let cnt2 = _fold(zip(a, unsafe(each(b))).count())
    t |> equal(cnt2, 3, "zip fold count array+iter")
}
