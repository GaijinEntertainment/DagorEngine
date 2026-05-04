options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_distinct(t : T?) {
    t |> run("basic distinct") @(t : T?) {
        var query = distinct(
            [iterator for(x in 0..5); x]
        )
        for (c, i in query, 0..5) {
            t |> equal(c, i)
        }
    }
    t |> run("complex distinct") @(t : T?) {
        var qcomplex = distinct(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("distinct from array") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 4, 3, 2, 1, 0]
        var qarray = distinct(arr)
        for (c, i in qarray, 0..4) {
            t |> equal(c, i)
        }
    }
    t |> run("distinct empty") @(t : T?) {
        var qempty = distinct(
            empty(type<int>)
        )
        var found = false
        for (it in qempty) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("distinct to array") @(t : T?) {
        var qtoarray = distinct_to_array(
            [iterator for(x in 0..5); x]
        )
        for (c, i in qtoarray, 0..5) {
            t |> equal(c, i)
        }
    }
    t |> run("distinct in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 4, 3, 2, 1, 0]
        distinct_inplace(arr)
        for (c, i in arr, 0..4) {
            t |> equal(c, i)
        }
    }
}

[test]
def test_distinctby(t : T?) {
    t |> run("basic distinct by") @(t : T?) {
        var query = _distinct_by(
            people.to_sequence(),
            _.name
        )
        var distinct_names = ["Alice", "Bob", "Charlie"]
        for (qn, dn in query, distinct_names) {
            t |> equal(qn.name, dn)
        }
    }
    t |> run("complex distinct by") @(t : T?) {
        var qcomplex = _distinct_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("distinct by empty") @(t : T?) {
        var qempty = _distinct_by(
            empty(type<int>),
            _
        )
        var found = false
        for (it in qempty) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("distinct by to array") @(t : T?) {
        var qtoarray = distinct_by_to_array(
            [iterator for(x in 0..5); x],
            $(_ : int) => _
        )
        for (c, i in qtoarray, 0..5) {
            t |> equal(c, i)
        }
    }
    t |> run("distinct by in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 4, 3, 2, 1, 0]
        distinct_by_inplace(arr, $(_ : int) => _)
        for (c, i in arr, 0..4) {
            t |> equal(c, i)
        }
    }
}

[test]
def test_union(t : T?) {
    t |> run("basic union") @(t : T?) {
        var query = union(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex union") @(t : T?) {
        var qcomplex = union(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])]
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20),
            (3, 30),
            (4, 40),
            (5, 50),
            (6, 60),
            (7, 70),
            (8, 80)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("union to array") @(t : T?) {
        var qtoarray = union_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_unionby(t : T?) {
    t |> run("basic union by") @(t : T?) {
        var query = _union_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            _
        )
        var expected = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex union by") @(t : T?) {
        var qcomplex = _union_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])],
            _.a[0]
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20),
            (3, 30),
            (4, 40),
            (5, 50),
            (6, 60),
            (7, 70),
            (8, 80)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("union by to array") @(t : T?) {
        var qtoarray = union_by_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            $(_ : int) => _
        )
        var expected = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_except(t : T?) {
    t |> run("basic except") @(t : T?) {
        var query = except(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [0, 1, 2]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex except") @(t : T?) {
        var qcomplex = except(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])]
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("except from array") @(t : T?) {
        var arr1 = [0, 1, 2, 3, 4, 5]
        var arr2 = [3, 4, 5, 6, 7, 8]
        var qarray = except(arr1, arr2)
        var expected = [0, 1, 2]
        for (c, e in qarray, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("except empty") @(t : T?) {
        var qempty = except(
            empty(type<int>),
            [iterator for(x in 0..5); x]
        )
        var found = false
        for (it in qempty) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("except to array") @(t : T?) {
        var qtoarray = except_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [0, 1, 2]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_exceptby(t : T?) {
    t |> run("basic except by") @(t : T?) {
        var query = _except_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            _
        )
        var expected = [0, 1, 2]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex except by") @(t : T?) {
        var qcomplex = _except_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])],
            _.a[0]
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("except by to array") @(t : T?) {
        var qtoarray = _except_by_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            _
        )
        var expected = [0, 1, 2]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_intersect(t : T?) {
    t |> run("basic intersect") @(t : T?) {
        var query = intersect(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [3, 4, 5]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex intersect") @(t : T?) {
        var qcomplex = intersect(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])]
        )
        var expected_complex = [
            (3, 30),
            (4, 40),
            (5, 50)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("intersect from array") @(t : T?) {
        var arr1 = [0, 1, 2, 3, 4, 5]
        var arr2 = [3, 4, 5, 6, 7, 8]
        var qarray = intersect(arr1, arr2)
        var expected = [3, 4, 5]
        for (c, e in qarray, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("intersect empty") @(t : T?) {
        var qempty = intersect(
            empty(type<int>),
            [iterator for(x in 0..5); x]
        )
        var found = false
        for (it in qempty) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("intersect to array") @(t : T?) {
        var qtoarray = intersect_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x]
        )
        var expected = [3, 4, 5]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}

[test]
def test_intersectby(t : T?) {
    t |> run("basic intersect by") @(t : T?) {
        var query = _intersect_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            _
        )
        var expected = [3, 4, 5]
        for (c, e in query, expected) {
            t |> equal(c, e)
        }
    }
    t |> run("complex intersect by") @(t : T?) {
        var qcomplex = _intersect_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 3..8); ComplexType(a = [x, x * 10])],
            _.a[0]
        )
        var expected_complex = [
            (3, 30),
            (4, 40),
            (5, 50)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("intersect by to array") @(t : T?) {
        var qtoarray = _intersect_by_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 3..8); x],
            _
        )
        var expected = [3, 4, 5]
        for (c, e in qtoarray, expected) {
            t |> equal(c, e)
        }
    }
}
