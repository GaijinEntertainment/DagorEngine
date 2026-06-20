options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost
require daslib/sort_boost

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
        let sorted_n = [1, 3, 4, 5, 8]
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
        let sorted_n = [1, 3, 4, 5, 8]
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
        let sorted_n = [1, 3, 4, 5, 8]
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
        let sorted_n = [1, 3, 4, 5, 8]
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
        let sorted_n = [1, 3, 4, 5, 8]
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
        let sorted_n = [1, 3, 4, 5, 8]
        for (n, i in numbers, 0..4) {
            t |> equal(n, sorted_n[i])
        }
    }
}

[test]
def test_order_descending_default(t : T?) {
    t |> run("order descending default key") @(t : T?) {
        let numbers = [5, 3, 8, 1, 4]
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
        let numbers = [
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
        let complex_numbers = [
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
        let numbers = [
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
        let complex_numbers = [
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

[test]
def test_partial_sort(t : T?) {
    t |> run("basic partial_sort first N ascending") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7, 4, 6, 0]
        sort_boost::partial_sort(a, 4)
        let expected = [0, 1, 2, 3]
        for (v, e in a, expected) {
            t |> equal(v, e)
        }
        for (i in range(4, length(a))) {
            t |> success(a[i] >= 3)
        }
    }
    t |> run("partial_sort with custom comparator (descending)") @(t : T?) {
        var a <- [5, 2, 8, 1, 9]
        sort_boost::partial_sort(a, 3) $(x, y : int) : bool {
            return x > y
        }
        t |> equal(a[0], 9)
        t |> equal(a[1], 8)
        t |> equal(a[2], 5)
    }
    t |> run("partial_sort N=0 is no-op") @(t : T?) {
        var a <- [5, 2, 8, 1, 9]
        var orig <- [5, 2, 8, 1, 9]
        sort_boost::partial_sort(a, 0)
        for (v, e in a, orig) {
            t |> equal(v, e)
        }
    }
    t |> run("partial_sort N>length clamps to full sort") @(t : T?) {
        var a <- [5, 2, 8, 1, 9]
        sort_boost::partial_sort(a, 100)
        let expected = [1, 2, 5, 8, 9]
        for (v, e in a, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("partial_sort on user struct via key comparator") @(t : T?) {
        var a <- [
            ComplexType(a = [5, 100]),
            ComplexType(a = [2, 101]),
            ComplexType(a = [8, 102]),
            ComplexType(a = [1, 103])
        ]
        sort_boost::partial_sort(a, 2) $(x, y : ComplexType) : bool {
            return x.a[0] < y.a[0]
        }
        t |> equal(a[0].a[0], 1)
        t |> equal(a[0].a[1], 103)
        t |> equal(a[1].a[0], 2)
        t |> equal(a[1].a[1], 101)
    }
}

[test]
def test_nth_element(t : T?) {
    t |> run("kth-smallest at position k") @(t : T?) {
        var a <- [7, 3, 1, 9, 5, 8, 0, 4, 2]
        sort_boost::nth_element(a, 4)
        t |> equal(a[4], 4)
        for (i in range(4)) {
            t |> success(a[i] <= 4)
        }
        for (i in range(5, length(a))) {
            t |> success(a[i] >= 4)
        }
    }
    t |> run("nth_element k=0 puts min at front") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3]
        sort_boost::nth_element(a, 0)
        t |> equal(a[0], 1)
    }
    t |> run("nth_element k out of range is no-op") @(t : T?) {
        var a <- [5, 2, 8, 1, 9]
        var orig <- [5, 2, 8, 1, 9]
        sort_boost::nth_element(a, 100)
        for (v, e in a, orig) {
            t |> equal(v, e)
        }
    }
    t |> run("nth_element with key comparator on user struct") @(t : T?) {
        var a <- [
            ComplexType(a = [5, 100]),
            ComplexType(a = [2, 101]),
            ComplexType(a = [8, 102]),
            ComplexType(a = [1, 103]),
            ComplexType(a = [9, 104])
        ]
        sort_boost::nth_element(a, 2) $(x, y : ComplexType) : bool {
            return x.a[0] < y.a[0]
        }
        t |> equal(a[2].a[0], 5)
        t |> equal(a[2].a[1], 100)
    }
}

[test]
def test_top_n_by(t : T?) {
    t |> run("top_n_by on array source — N smallest ascending") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_by(src, 3, @@(v : int -&) => v)
        let expected = [2, 5, 8]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by on iterator source — heap-during-scan") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_by(src.to_sequence(), 3, @@(v : int -&) => v)
        let expected = [2, 5, 8]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by N=0 yields empty") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_by(src, 0, @@(v : int -&) => v)
        t |> equal(length(got), 0)
    }
    t |> run("top_n_by N > length yields all elements sorted") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_by(src, 100, @@(v : int -&) => v)
        let expected = [5, 10, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by N=1 picks single minimum") @(t : T?) {
        let src <- [7, 3, 1, 9, 5]
        let got <- top_n_by(src, 1, @@(v : int -&) => v)
        t |> equal(length(got), 1)
        t |> equal(got[0], 1)
    }
    t |> run("top_n_by on user struct by field key") @(t : T?) {
        let src <- [
            ComplexType(a = [10, 0]),
            ComplexType(a = [5, 0]),
            ComplexType(a = [25, 0]),
            ComplexType(a = [2, 0]),
            ComplexType(a = [15, 0])
        ]
        let got <- top_n_by(src, 3, @@(c : ComplexType) => c.a[0])
        t |> equal(length(got), 3)
        t |> equal(got[0].a[0], 2)
        t |> equal(got[1].a[0], 5)
        t |> equal(got[2].a[0], 10)
    }
}

[test]
def test_top_n_by_descending(t : T?) {
    t |> run("top_n_by_descending on array source — N largest descending") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_by_descending(src, 3, @@(v : int -&) => v)
        let expected = [30, 25, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by_descending on iterator source — bounded min-heap") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_by_descending(src.to_sequence(), 3, @@(v : int -&) => v)
        let expected = [30, 25, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by_descending N=0 yields empty") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_by_descending(src, 0, @@(v : int -&) => v)
        t |> equal(length(got), 0)
    }
    t |> run("top_n_by_descending N<0 yields empty") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_by_descending(src, -1, @@(v : int -&) => v)
        t |> equal(length(got), 0)
    }
    t |> run("top_n_by_descending N > length yields all elements sorted descending") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_by_descending(src, 100, @@(v : int -&) => v)
        let expected = [20, 10, 5]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_by_descending N=1 picks single maximum") @(t : T?) {
        let src <- [7, 3, 1, 9, 5]
        let got <- top_n_by_descending(src, 1, @@(v : int -&) => v)
        t |> equal(length(got), 1)
        t |> equal(got[0], 9)
    }
    t |> run("top_n_by_descending on iterator source — empty") @(t : T?) {
        let src : array<int>
        let got <- top_n_by_descending(src.to_sequence(), 3, @@(v : int -&) => v)
        t |> equal(length(got), 0)
    }
    t |> run("top_n_by_descending on user struct by field key") @(t : T?) {
        let src <- [
            ComplexType(a = [10, 0]),
            ComplexType(a = [5, 0]),
            ComplexType(a = [25, 0]),
            ComplexType(a = [2, 0]),
            ComplexType(a = [15, 0])
        ]
        let got <- top_n_by_descending(src, 3, @@(c : ComplexType) => c.a[0])
        t |> equal(length(got), 3)
        t |> equal(got[0].a[0], 25)
        t |> equal(got[1].a[0], 15)
        t |> equal(got[2].a[0], 10)
    }
    t |> run("top_n_by_descending parity vs sort+take direct reference") @(t : T?) {
        let src <- [42, 7, 13, 88, 21, 4, 99, 56, 33, 71]
        let got <- top_n_by_descending(src, 4, @@(v : int -&) => v)
        // hand-rolled reference: clone, sort descending, take first 4
        var refSorted : array<int>
        refSorted := src
        sort(refSorted, $(a, b) => a > b)
        t |> equal(length(got), 4)
        for (i in 0..4) {
            t |> equal(got[i], refSorted[i])
        }
    }
}

[test]
def test_top_n_descending(t : T?) {
    t |> run("top_n_descending on array source — N largest descending") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_descending(src, 3)
        let expected = [30, 25, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_descending on iterator source") @(t : T?) {
        let src <- [10, 20, 5, 8, 30, 15, 2, 25]
        let got <- top_n_descending(src.to_sequence(), 3)
        let expected = [30, 25, 20]
        t |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            t |> equal(v, e)
        }
    }
    t |> run("top_n_descending N=0 yields empty") @(t : T?) {
        let src <- [10, 20, 5]
        let got <- top_n_descending(src, 0)
        t |> equal(length(got), 0)
    }
    t |> run("top_n_descending on floats") @(t : T?) {
        let src <- [3.14, 1.41, 2.71, 0.57, 1.61]
        let got <- top_n_descending(src, 2)
        t |> equal(length(got), 2)
        t |> equal(got[0], 3.14)
        t |> equal(got[1], 2.71)
    }
}

[test]
def test_heap_ops(t : T?) {
    t |> run("make_heap → max at root") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7]
        sort_boost::make_heap(a)
        t |> equal(a[0], 9)
    }
    t |> run("make_heap → repeated pop yields descending sequence") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7, 4, 6, 0]
        sort_boost::make_heap(a)
        var popped : array<int>
        while (length(a) > 0) {
            sort_boost::pop_heap(a)
            popped |> push(a[length(a) - 1])
            a |> resize(length(a) - 1)
        }
        t |> equal(length(popped), 10)
        for (i in range(1, length(popped))) {
            t |> success(popped[i] <= popped[i - 1])
        }
    }
    t |> run("push_heap append-then-sift") @(t : T?) {
        var a <- [9, 5, 7, 1, 3, 6]
        sort_boost::make_heap(a)
        t |> equal(a[0], 9)
        a |> push(12)
        sort_boost::push_heap(a)
        t |> equal(a[0], 12)
    }
    t |> run("bounded-N streaming heap (top_n_by usage pattern)") @(t : T?) {
        let N = 3
        let src <- [7, 3, 5, 1, 9, 2, 8, 4, 6, 0]
        var buf : array<int>
        buf |> reserve(N)
        for (x in src) {
            if (length(buf) < N) {
                buf |> push(x)
                sort_boost::push_heap(buf)
            } elif (x < buf[0]) {
                sort_boost::pop_heap(buf)
                buf[length(buf) - 1] = x
                sort_boost::push_heap(buf)
            }
        }
        sort(buf)
        let expected = [0, 1, 2]
        for (v, e in buf, expected) {
            t |> equal(v, e)
        }
    }
}

[test]
def test_q_macros(t : T?) {
    //! Smoke tests for the qpartial_sort / qnth_element / qmake_heap /
    //! qpush_heap / qpop_heap dispatcher macros. Verifies the macro expansion
    //! routes to the right wrapper for ``array<T>`` sources.
    t |> run("qpartial_sort on array") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7, 4, 6, 0]
        sort_boost::qpartial_sort(a, 3) <| $(x, y : int) : bool {
            return x < y
        }
        t |> equal(a[0], 0)
        t |> equal(a[1], 1)
        t |> equal(a[2], 2)
    }
    t |> run("qnth_element on array") @(t : T?) {
        var a <- [7, 3, 1, 9, 5, 8, 0, 4, 2]
        sort_boost::qnth_element(a, 4) <| $(x, y : int) : bool {
            return x < y
        }
        t |> equal(a[4], 4)
    }
    t |> run("qmake_heap on array") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7]
        sort_boost::qmake_heap(a) <| $(x, y : int) : bool {
            return x < y
        }
        t |> equal(a[0], 9)
    }
    t |> run("qpush_heap append-then-sift") @(t : T?) {
        var a <- [9, 5, 7, 1, 3, 6]
        sort_boost::qmake_heap(a) <| $(x, y : int) : bool {
            return x < y
        }
        a |> push(12)
        sort_boost::qpush_heap(a) <| $(x, y : int) : bool {
            return x < y
        }
        t |> equal(a[0], 12)
    }
    t |> run("qpop_heap moves max to end") @(t : T?) {
        var a <- [5, 2, 8, 1, 9, 3, 7]
        sort_boost::qmake_heap(a) <| $(x, y : int) : bool {
            return x < y
        }
        sort_boost::qpop_heap(a) <| $(x, y : int) : bool {
            return x < y
        }
        t |> equal(a[length(a) - 1], 9)
    }
}
