options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq

require _common

[test]
def test_defaultempty(t : T?) {
    t |> run("basic default empty") @(t : T?) {
        var query = default_empty(
            [iterator for(x in 0..5); x]
        )
        var expected = [0, 1, 2, 3, 4, 5]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("empty default") @(t : T?) {
        var q2 = default_empty(
            [iterator for(x in 0..0); x]
        )
        var expected2 = [0]
        for (c, e in q2, expected2) {
            t |> equal(c, e)
        }
    }
    t |> run("complex default") @(t : T?) {
        var qcomplex = default_empty(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20),
            (3, 30),
            (4, 40),
            (5, 50)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
}

[test]
def test_empty(t : T?) {
    t |> run("basic empty") @(t : T?) {
        var query = empty(type<int>)
        var found = false
        for (it in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("complex empty") @(t : T?) {
        var qcomplex = empty(type<ComplexType>)
        var found_complex = false
        for (it in qcomplex) {
            found_complex = true
        }
        t |> success(!found_complex)
    }
}

[test]
def test_range(t : T?) {
    t |> run("basic range_sequence") @(t : T?) {
        var query = range_sequence(0, 5)
        var expected = [0, 1, 2, 3, 4, 5]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_repeat(t : T?) {
    t |> run("basic repeat") @(t : T?) {
        var query = repeat(42, 5)
        var expected = [42, 42, 42, 42, 42]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex repeat") @(t : T?) {
        var qcomplex = repeat(ComplexType(a = [1, 2]), 3)
        var expected_complex = [
            (1, 2),
            (1, 2),
            (1, 2)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
}
