options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_totable(t : T?) {
    t |> run("basic totable") @(t : T?) {
        var query = to_table(
            [iterator for(x in 0..5); x],
            $(x : int) => x,
            $(x : int) => x * 10
        )
        var expected = { 0 => 0, 1 => 10, 2 => 20, 3 => 30, 4 => 40 }
        for (ek, ev in keys(expected), values(expected)) {
            let found = query.get(unique_key(ek), $(v) {
                t |> equal(ev, v)
            })
            t |> success(found)
        }
    }
    t |> run("complex type totable") @(t : T?) {
        var qcomplex = to_table(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            $(c : ComplexType) => (c.a[0], c.a[1]),
            $(c : ComplexType) => (c.a[0] * 2, c.a[1] * 2)
        )
        var expected = { 0 => 0, 1 => 10, 2 => 20, 3 => 30, 4 => 40 }
        for (ek, ev in keys(expected), values(expected)) {
            let k = unique_key((ek, ek * 10))
            let found = qcomplex.get(k, $(v) {
                t |> equal(v._0, ek * 2)
                t |> equal(v._1, ek * 2 * 10)
            })
            t |> success(found)
        }
    }
    t |> run("array totable") @(t : T?) {
        var qarray = to_table(
            [1, 2, 3, 4, 5],
            $(x : int) => x,
            $(x : int) => x * 10
        )
        var expected = { 1 => 10, 2 => 20, 3 => 30, 4 => 40, 5 => 50 }
        for (ek, ev in keys(expected), values(expected)) {
            let found = qarray.get(unique_key(ek), $(v) {
                t |> equal(ev, v)
            })
            t |> success(found)
        }
    }
}

[test]
def test_where(t : T?) {
    t |> run("basic where") @(t : T?) {
        var query = _where(
            [iterator for(x in 0..5); x],
            _ % 2 == 0
        )
        for (c, i in query, 0..3) {
            t |> equal(c, i * 2)
        }
    }
    t |> run("people where") @(t : T?) {
        var qp = _where(
            people.to_sequence(),
            _.age >= 30
        )
        var count = 0
        for (p in qp) {
            t |> success(p.age >= 30)
            count ++
        }
        t |> equal(count, 2)
    }
    t |> run("complex where") @(t : T?) {
        var qcomplex = _where(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0] % 2 == 0
        )
        for (c, i in qcomplex, 0..3) {
            t |> equal(c.a[0], i * 2)
        }
    }
    t |> run("basic where from array") @(t : T?) {
        var query = _where(
            [0, 1, 2, 3, 4],
            _ % 2 == 0
        )
        for (c, i in query, 0..3) {
            t |> equal(c, i * 2)
        }
    }
    t |> run("complex where from array") @(t : T?) {
        var qcomplex = _where(
            [ComplexType(a = [0, 0]), ComplexType(a = [1, 10]), ComplexType(a = [2, 20]), ComplexType(a = [3, 30]), ComplexType(a = [4, 40])],
            _.a[0] % 2 == 0
        )
        for (c, i in qcomplex, 0..3) {
            t |> equal(c.a[0], i * 2)
        }
    }
    t |> run("where to array") @(t : T?) {
        var query = _where_to_array(
            [iterator for(x in 0..5); x],
            _ % 2 == 0
        )
        for (c, i in query, 0..3) {
            t |> equal(c, i * 2)
        }
    }
}

[test]
def test_sequenceequal(t : T?) {
    t |> run("basic sequence equal") @(t : T?) {
        var query = sequence_equal(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..5); x]
        )
        t |> success(query)
    }
    t |> run("basic sequence not equal") @(t : T?) {
        var qneq = sequence_equal(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..5); x + 1]
        )
        t |> success(!qneq)
    }
    t |> run("complex sequence equal") @(t : T?) {
        var qcomplex = sequence_equal(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(qcomplex)
    }
    t |> run("array sequence equal") @(t : T?) {
        var qarray = sequence_equal(
            [0, 1, 2, 3, 4],
            [0, 1, 2, 3, 4]
        )
        t |> success(qarray)
    }
}

[test]
def test_sequenceequalby(t : T?) {
    t |> run("basic sequence equal by") @(t : T?) {
        var query = _sequence_equal_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..5); x],
            _
        )
        t |> success(query)
    }
    t |> run("basic sequence not equal by") @(t : T?) {
        var qneq = _sequence_equal_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..5); x + 1],
            _
        )
        t |> success(!qneq)
    }
    t |> run("complex sequence not equal by") @(t : T?) {
        var qcomplex = _sequence_equal_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 0..5); ComplexType(a = [x, x * 9])],
            _.a[0]
        )
        t |> success(qcomplex)
    }
    t |> run("array sequence equal by") @(t : T?) {
        var qarray = _sequence_equal_by(
            [0, 1, 2, 3, 4],
            [0, 1, 2, 3, 4],
            _
        )
        t |> success(qarray)
    }
}



