options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_reverse(t : T?) {
    t |> run("basic reverse") @(t : T?) {
        var query = reverse(
            [iterator for(x in 0..5); x]
        )
        for (c, i in query, 0..5) {
            t |> equal(c, 4 - i)
        }
    }
    t |> run("complex type reverse") @(t : T?) {
        var qcomplex = reverse(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([4 - i, (4 - i) * 10]))
        }
    }
}

[test]
def test_orderby(t : T?) {
    t |> run("basic orderby") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = _order_by(
            numbers.to_sequence(),
            _
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in query_n, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
    t |> run("complex orderby") @(t : T?) {
        var query = _order_by(
            people.to_sequence(),
            (_.name, _.age)
        )
        for (p, i in query, 0..5) {
            t |> equal(p.name, sorted[i].name)
            t |> equal(p.age, sorted[i].age)
        }
    }
    t |> run("reverse orderby") @(t : T?) {
        var rquery = _order_by_descending(
            people.to_sequence(),
            (_.name, _.age)
        )
        for (p, i in rquery, 0..5) {
            t |> equal(p.name, sorted_r[i].name)
            t |> equal(p.age, sorted_r[i].age)
        }
    }
    t |> run("complex orderby") @(t : T?) {
        var qcomplex = _order_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("reverse complex orderby") @(t : T?) {
        var qcomplex_r = _order_by_descending(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        for (c, i in qcomplex_r, 0..5) {
            t |> success(c.a.Equal([4 - i, (4 - i) * 10]))
        }
    }
    t |> run("orderby to array") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = _order_by_to_array(
            numbers.to_sequence(),
            _
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in query_n, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
    t |> run("orderby in place") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        order_by_inplace(
            numbers,
            $(_ : int) => _
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in numbers, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
}

[test]
def test_order(t : T?) {
    t |> run("basic order") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = order(
            numbers.to_sequence(),
            $(a, b) => a < b
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in query_n, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
    t |> run("complex order") @(t : T?) {
        var query = order(
            people.to_sequence(),
            $(a, b) => less((a.name, a.age), (b.name, b.age))
        )
        for (p, i in query, 0..5) {
            t |> equal(p.name, sorted[i].name)
            t |> equal(p.age, sorted[i].age)
        }
    }
    t |> run("reverse order") @(t : T?) {
        var rquery = order_descending(
            people.to_sequence(),
            $(a, b) => less((a.name, a.age), (b.name, b.age))
        )
        for (p, i in rquery, 0..5) {
            t |> equal(p.name, sorted_r[i].name)
            t |> equal(p.age, sorted_r[i].age)
        }
    }
    t |> run("complex order") @(t : T?) {
        var qcomplex = order(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            $(a, b) => less((a.a[0], a.a[1]), (b.a[0], b.a[1]))
        )
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("reverse complex order") @(t : T?) {
        var qcomplex_r = order_descending(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            $(a, b) => less((a.a[0], a.a[1]), (b.a[0], b.a[1]))
        )
        for (c, i in qcomplex_r, 0..5) {
            t |> success(c.a.Equal([4 - i, (4 - i) * 10]))
        }
    }
    t |> run("order to array") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = order_to_array(
            numbers.to_sequence(),
            $(a, b) => a < b
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in query_n, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
    t |> run("order in place") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        order_inplace(
            numbers,
            $(a, b) => a < b
        )
        var sorted_n = [1, 3, 4, 5, 8]
        for (n, i in numbers, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
}

[test]
def test_order_descending_default(t : T?) {
    t |> run("order descending default key") @(t : T?) {
        var numbers = [5, 3, 8, 1, 4]
        var query = order_descending(numbers)
        var expected = [8, 5, 4, 3, 1]
        for (n, e in query, expected) {
            t |> equal(n, e)
        }
    }
    t |> run("order descending default to array") @(t : T?) {
        var result = order_descending_to_array(
            [iterator for(x in range(5)); [5, 3, 8, 1, 4][x]]
        )
        static_assert(typeinfo is_array(result))
        var expected = [8, 5, 4, 3, 1]
        for (n, e in result, expected) {
            t |> equal(n, e)
        }
    }
    t |> run("order descending default inplace") @(t : T?) {
        var numbers = [5, 3, 8, 1, 4]
        order_descending_inplace(numbers)
        var expected = [8, 5, 4, 3, 1]
        for (n, e in numbers, expected) {
            t |> equal(n, e)
        }
    }
    t |> run("order descending default with _fold") @(t : T?) {
        var result = (
            [5, 3, 8, 1, 4]
            .order_descending()
            ._fold()
        )
        t |> equal(typeinfo typename(result), "array<int>")
        var expected = [8, 5, 4, 3, 1]
        for (n, e in result, expected) {
            t |> equal(n, e)
        }
    }
}

[test]
def test_unique(t : T?) {
    t |> run("basic unique") @(t : T?) {
        var numbers = [
            1, 1, 1, 2, 2, 3, 4, 5, 1, 4
        ]
        var query_n = unique(numbers)
        var expected_n = [1, 2, 3, 4, 5, 1, 4]
        t |> equal(length(query_n), length(expected_n))
        for (q, e in query_n, expected_n) {
            t |> equal(q, e)
        }
    }
    t |> run("complex unique") @(t : T?) {
        var complex_numbers = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [3, 30]),
            ComplexType(a = [1, 10])
        ]
        var query_c = unique(complex_numbers)
        var expected_c = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [3, 30]),
            ComplexType(a = [1, 10])
        ]
        t |> equal(length(query_c), length(expected_c))
        for (q, e in query_c, expected_c) {
            t |> success(q == e)
        }
    }
}

[test]
def test_unique_by(t : T?) {
    t |> run("basic unique by") @(t : T?) {
        var numbers = [
            1, 1, 1, 2, 2, 3, 4, 5, 1, 4
        ]
        var query_n = _unique_by(
            numbers,
            _
        )
        var expected_n = [1, 2, 3, 4, 5, 1, 4]
        t |> equal(length(query_n), length(expected_n))
        for (q, e in query_n, expected_n) {
            t |> equal(q, e)
        }
    }
    t |> run("complex unique by") @(t : T?) {
        var complex_numbers = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [3, 30]),
            ComplexType(a = [1, 10])
        ]
        var query_c = _unique_by(
            complex_numbers,
            _.a[0]
        )
        var expected_c = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [3, 30]),
            ComplexType(a = [1, 10])
        ]
        t |> equal(length(query_c), length(expected_c))
        for (q, e in query_c, expected_c) {
            t |> success(q == e)
        }
    }
}
