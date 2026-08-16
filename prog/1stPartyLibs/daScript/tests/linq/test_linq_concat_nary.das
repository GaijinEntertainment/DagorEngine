options gen2
require daslib/linq
require daslib/linq_boost
require dastest/testing_boost public

require _common


// N-ary concat (3..8 sources). The binary forms live in test_linq_concat.das;
// this file exercises only the multi-source overloads and their fold fusion.

[test]
def test_concat3(t : T?) {
    t |> run("3 iterators -> iterator") @(t : T?) {
        var query = concat(
            [iterator for(x in 0..3); x],
            [iterator for(x in 3..6); x],
            [iterator for(x in 6..9); x]
        )
        for (c, i in query, 0..9) {
            t |> equal(c, i)
        }
    }
    t |> run("3 iterators -> array") @(t : T?) {
        let qarray = concat_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 3..6); x],
            [iterator for(x in 6..9); x]
        )
        t |> success(qarray.Equal([0, 1, 2, 3, 4, 5, 6, 7, 8]))
    }
    t |> run("3 arrays -> array") @(t : T?) {
        let a = [for (x in 0..3); x]
        let b = [for (x in 3..6); x]
        let c = [for (x in 6..9); x]
        let result = concat(a, b, c)
        t |> success(result.Equal([0, 1, 2, 3, 4, 5, 6, 7, 8]))
    }
    t |> run("3 complex iterators") @(t : T?) {
        var qcomplex = concat(
            [iterator for(x in 0..3); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..6); ComplexType(a = [x, x * 10])],
            [iterator for(x in 6..9); ComplexType(a = [x, x * 10])]
        )
        for (c, i in qcomplex, 0..9) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("concat_inplace 3 arrays") @(t : T?) {
        var a = [for (x in 0..3); x]
        concat_inplace(a, [for (x in 3..6); x], [for (x in 6..9); x])
        t |> success(a.Equal([0, 1, 2, 3, 4, 5, 6, 7, 8]))
    }
}

[test]
def test_concat_high_arity(t : T?) {
    t |> run("5 arrays -> array") @(t : T?) {
        let result = concat(
            [for (x in 0..2); x], [for (x in 2..4); x], [for (x in 4..6); x],
            [for (x in 6..8); x], [for (x in 8..10); x]
        )
        t |> success(result.Equal([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]))
    }
    t |> run("5 iterators -> iterator") @(t : T?) {
        var query = concat(
            [iterator for(x in 0..2); x], [iterator for(x in 2..4); x], [iterator for(x in 4..6); x],
            [iterator for(x in 6..8); x], [iterator for(x in 8..10); x]
        )
        for (c, i in query, 0..10) {
            t |> equal(c, i)
        }
    }
    t |> run("8 arrays -> array") @(t : T?) {
        let result = concat(
            [for (x in 0..1); x], [for (x in 1..2); x], [for (x in 2..3); x], [for (x in 3..4); x],
            [for (x in 4..5); x], [for (x in 5..6); x], [for (x in 6..7); x], [for (x in 7..8); x]
        )
        t |> success(result.Equal([0, 1, 2, 3, 4, 5, 6, 7]))
    }
    t |> run("8 iterators -> array") @(t : T?) {
        let result = concat_to_array(
            [iterator for(x in 0..1); x], [iterator for(x in 1..2); x], [iterator for(x in 2..3); x], [iterator for(x in 3..4); x],
            [iterator for(x in 4..5); x], [iterator for(x in 5..6); x], [iterator for(x in 6..7); x], [iterator for(x in 7..8); x]
        )
        t |> success(result.Equal([0, 1, 2, 3, 4, 5, 6, 7]))
    }
    t |> run("concat_inplace 8 arrays") @(t : T?) {
        var a = [for (x in 0..1); x]
        concat_inplace(a,
            [for (x in 1..2); x], [for (x in 2..3); x], [for (x in 3..4); x],
            [for (x in 4..5); x], [for (x in 5..6); x], [for (x in 6..7); x], [for (x in 7..8); x]
        )
        t |> success(a.Equal([0, 1, 2, 3, 4, 5, 6, 7]))
    }
}

[test]
def test_concat3_fold(t : T?) {
    t |> run("3-source concat fold") @(t : T?) {
        // Each source is its own sub-pipeline — exercises recursive folding of args 0, 1, 2.
        var q <- (
            concat(
                [1, 2, 3, 4, 5]._where(_ <= 2)._select(_ * 2)
            ,
                [6, 7, 8]._where(_ >= 7)
            ,
                [10, 20, 30]._select(_ + 1)
            )
            ._fold()
        )
        t |> equal(typeinfo typename(q), "array<int>")
        t |> equal(length(q), 7)
        let expected = [2, 4, 7, 8, 11, 21, 31]    // nolint:LINT003
        for (i, v in 0..7, q) {
            t |> equal(expected[i], v)
        }
    }
}
