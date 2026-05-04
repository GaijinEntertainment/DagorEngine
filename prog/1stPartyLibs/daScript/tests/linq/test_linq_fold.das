options gen2
require daslib/linq
require dastest/testing_boost public
require math
require strings

require daslib/linq_boost

require _common

[test]
def test_comprehension_fold(t : T?) {
    t |> run("basic comprehension") @(t : T?) {
        var t1 <- ([1, 2, 3, 4, 5].to_sequence()
            ._where(_ <= 3)
            .to_array()
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t1), "array<int>")
        for (i, v in 0..3, t1) {
            t |> equal(i + 1, v)
        }
    }

    t |> run("comprehension with select") @(t : T?) {
        var t2 <- ([1, 2, 3, 4, 5].to_sequence()
            ._where(_ <= 3)
            ._select(_ * 2)
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t2), "iterator<int>")
        for (i, v in 0..3, t2) {
            t |> equal((i + 1) * 2, v)
        }
    }

    t |> run("comprehension with select to array") @(t : T?) {
        var t3 <- ([1, 2, 3, 4, 5].to_sequence()
            ._select(_ * 2)
            .to_array()
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t3), "array<int>")
        for (i, v in 1..6, t3) {
            t |> equal(i * 2, v)
        }
    }

    t |> run("selct-where comprehension") @(t : T?) {
        var t4 <- ([1, 2, 3, 4, 5].to_sequence()
            ._select(_ * 2)
            ._where(_ <= 6)
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t4), "iterator<int>")
        for (i, v in 0..3, t4) {
            t |> equal((i + 1) * 2, v)
        }
    }

    t |> run("select-where from array comprehension") @(t : T?) {
        var t5 <- ([1, 2, 3, 4, 5]
            ._select(_ * 2)
            ._where(_ <= 6)
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t5), "array<int>")
        for (i, v in 0..3, t5) {
            t |> equal((i + 1) * 2, v)
        }
    }

    // returns array of ComplexType
    t |> run("ComplexType select-where comprehension") @(t : T?) {
        var t6 <- ([1, 2, 3, 4, 5].to_sequence()
            ._select(ComplexType(a = [_ * 2, _ * 3]))
            ._where(_.a[0] <= 6)
            .to_array()
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t6), "array<_common::ComplexType>")
        for (i, v in 0..3, t6) {
            t |> success(v.a.Equal([(i + 1) * 2, (i + 1) * 3]))
        }
    }
    // return iterator of ComplexType
    t |> run("ComplexType where-select comprehension") @(t : T?) {
        var t7 <- ([1, 2, 3, 4, 5].to_sequence()
            ._select(ComplexType(a = [_ * 2, _ * 3]))
            ._where(_.a[0] <= 3)
            ._fold()) // note - if fold fails, it won't compile
        t |> equal(typeinfo typename(t7), "iterator<_common::ComplexType>")
        for (i, v in 0..3, t7) {
            t |> success(v.a.Equal([(i + 1) * 2, (i + 1) * 3]))
        }
    }
}

[test]
def test_generic_fold(t : T?) {
    t |> run("default array to array") @(t : T?) {
        var t1 <- ([1, 2, 3, 4, 5]
            .reverse()
            ._where(_ <= 3)
            ._select(_ * 2)
            ._fold()
            )
        t |> equal(typeinfo typename(t1), "array<int>")
        for (i, v in 0..3, t1) {
            t |> equal((3 - i) * 2, v)
        }
    }
    t |> run("default iterator to iterator") @(t : T?) {
        var t2 <- ([1, 2, 3, 4, 5].to_sequence()
            .reverse()
            ._where(_ <= 3)
            ._select(_ * 2)
            ._fold()
            )
        t |> equal(typeinfo typename(t2), "iterator<int>")
        for (i, v in 0..3, t2) {
            t |> equal((3 - i) * 2, v)
        }
    }
    t |> run("reverse_to_array") @(t : T?) {
        var iterv <- [iterator for(x in 1..6); x]
        var t2a <- (iterv
            .reverse()
            ._where(_ <= 3)
            ._select(_ * 2)
            ._fold()
            )
        t |> equal(typeinfo typename(t2a), "iterator<int>")
        for (i, v in 0..3, t2a) {
            t |> equal((3 - i) * 2, v)
        }
    }
    t |> run("default array to iterator") @(t : T?) {
        var t3 <- ([1, 2, 3, 4, 5]
            .to_sequence()
            .reverse()
            ._where(_ <= 3)
            ._select(_ * 2)
            ._fold()
            )
        t |> equal(typeinfo typename(t3), "iterator<int>")
        for (i, v in 0..3, t3) {
            t |> equal((3 - i) * 2, v)
        }
    }
    t |> run("default iterator to array") @(t : T?) {
        var t4 <- ([1, 2, 3, 4, 5].to_sequence()
            .reverse()
            ._where(_ <= 3)
            ._select(_ * 2)
            .to_array()
            ._fold()
            )
        t |> equal(typeinfo typename(t4), "array<int>")
        for (i, v in 0..3, t4) {
            t |> equal((3 - i) * 2, v)
        }
    }
}

[test]
def test_generic_fold_order(t : T?) {
    t |> run("order fold") @(t : T?) {
        var t5 <- ([5, 3, 1, 4, 2]
            .to_sequence()
            .order($(a, b) => a < b)
            ._fold()
            )
        t |> equal(typeinfo typename(t5), "iterator<int>")
        for (i, v in 0..5, t5) {
            t |> equal(i + 1, v)
        }
    }
    t |> run("order_descending fold") @(t : T?) {
        var t6 <- ([5, 3, 1, 4, 2]
            .to_sequence()
            .order_descending($(a, b) => a < b)
            ._fold()
            )
        t |> equal(typeinfo typename(t6), "iterator<int>")
        for (i, v in 0..5, t6) {
            t |> equal(5 - i, v)
        }
    }
    t |> run("order_by fold") @(t : T?) {
        var t7 = (["apple", "banana", "kiwi", "cherry", "blueberry"]
            .to_sequence()
            ._order_by(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(t7), "iterator<string>")
        var sorted = ["kiwi", "apple", "banana", "cherry", "blueberry"]
        for (i, v in 0..5, t7) {
            t |> equal(sorted[i], v)
        }
    }
    t |> run("order_by_descending fold") @(t : T?) {
        var t8 = (["apple", "banana", "kiwi", "cherry", "blueberry"]
            ._order_by_descending(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(t8), "array<string>")
        var sorted = ["blueberry", "banana", "cherry", "apple", "kiwi"]
        for (i, v in 0..5, t8) {
            t |> equal(sorted[i], v)
        }
    }
}

[test]
def test_generic_fold_aggregates(t : T?) {
    t |> run("min fold") @(t : T?) {
        var t1 = ([5, 3, 8, 1, 4]
            .to_sequence()
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t1), "int")
        t |> equal(1, t1)
    }
    t |> run("min_by fold") @(t : T?) {
        var t2 = (["apple", "banana", "kiwi", "cherry", "blueberry"]
            .to_sequence()
            ._min_by(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "string")
        t |> equal("kiwi", t2)
    }
    t |> run("max fold") @(t : T?) {
        var t2 = ([5, 3, 8, 1, 4]
            .to_sequence()
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(8, t2)
    }
    t |> run("max_by fold") @(t : T?) {
        var t2 = (["apple", "banana", "kiwi", "cherry", "blueberry"]
            .to_sequence()
            ._max_by(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "string")
        t |> equal("blueberry", t2)
    }
    t |> run("min_max fold") @(t : T?) {
        var (tmin, tmax) = ([5, 3, 8, 1, 4]
            .to_sequence()
            .min_max()
            ._fold()
        )
        t |> equal(typeinfo typename(tmin), "int")
        t |> equal(typeinfo typename(tmax), "int")
        t |> equal(1, tmin)
        t |> equal(8, tmax)
    }
    t |> run("min_max_by fold") @(t : T?) {
        var (tmin, tmax) = (["apple", "banana", "kiwi", "cherry", "blueberry"]
            .to_sequence()
            ._min_max_by(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(tmin), "string")
        t |> equal(typeinfo typename(tmax), "string")
        t |> equal("kiwi", tmin)
        t |> equal("blueberry", tmax)
    }
    t |> run("average fold") @(t : T?) {
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]
            .to_sequence()
            .average()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(4, t2)
    }
    t |> run("min_max_avarage fold") @(t : T?) {
        var (tmin, tmax, tavg) = ([1, 2, 3, 4, 5, 6, 7, 8]
            .to_sequence()
            .min_max_average()
            ._fold()
        )
        t |> equal(typeinfo typename(tmin), "int")
        t |> equal(typeinfo typename(tmax), "int")
        t |> equal(typeinfo typename(tavg), "int")
        t |> equal(1, tmin)
        t |> equal(8, tmax)
        t |> equal(4, tavg)
    }
    t |> run("sum fold") @(t : T?) {
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]
            .to_sequence()
            .sum()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(36, t2)
    }
    t |> run("aggregate fold") @(t : T?) {
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]
            .to_sequence()
            .aggregate("", $(a, b) => "{a},{b}")
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "string")
        t |> equal(",1,2,3,4,5,6,7,8", t2)
    }
}

[test]
def test_generic_fold_skip_and_take(t : T?) {
    t |> run("skip fold") @(t : T?) {
        var t1 = ([5, 3, 8, 1, 4]
            .to_sequence()
            .skip(2)
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t1), "int")
        t |> equal(1, t1)
    }
    t |> run("skip_while fold") @(t : T?) {
        var t2 = ([5, 3, 8, 1, 4]
            .to_sequence()
            ._skip_while(_ > 3)
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(1, t2)
    }
    t |> run("take fold") @(t : T?) {
        var t3 = ([5, 3, 8, 1, 4]
            .to_sequence()
            .take(3)
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t3), "int")
        t |> equal(8, t3)
    }
    t |> run("take_while fold") @(t : T?) {
        var t4 = ([5, 3, 8, 1, 4]
            .to_sequence()
            ._take_while(_ < 8)
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t4), "int")
        t |> equal(5, t4)
    }
    t |> run("take range fold") @(t : T?) {
        var t5 = ([5, 3, 8, 1, 4]
            .to_sequence()
            .take(1..4)
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t5), "int")
        t |> equal(8, t5)
    }
}

[test]
def test_concat(t : T?) {
    t |> run("basic concat") @(t : T?) {
        var q <- (
            concat(
                [1, 2, 3, 4, 5]
                .reverse()
                ._where(_ <= 3)
                ._select(_ * 2)
            ,
                [6, 7, 8, 9, 10]
                .reverse()
                ._where(_ >= 8)
                ._select(_ * 3)
            )
            ._fold()
        )
        t |> equal(typeinfo typename(q), "array<int>")
        t |> equal(length(q), 6)
        var expected = [6, 4, 2, 30, 27, 24]
        for (i, v in 0..6, q) {
            t |> equal(expected[i], v)
        }
    }
    t |> run("basic append") @(t : T?) {
        var q <- (
            append(
                [1, 2, 3, 4, 5]
                .reverse()
                ._where(_ <= 3)
                ._select(_ * 2)
            ,
                100
            )
            ._fold()
        )
        t |> equal(typeinfo typename(q), "array<int>")
        t |> equal(length(q), 4)
        var expected = [6, 4, 2, 100]
        for (i, v in 0..4, q) {
            t |> equal(expected[i], v)
        }
    }
    t |> run("basic prepend") @(t : T?) {
        var q <- (
            prepend(
                [1, 2, 3, 4, 5]
                .reverse()
                ._where(_ <= 3)
                ._select(_ * 2)
            ,
                100
            )
            ._fold()
        )
        t |> equal(typeinfo typename(q), "array<int>")
        t |> equal(length(q), 4)
        var expected = [100, 6, 4, 2]
        for (i, v in 0..4, q) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_any_all_contains(t : T?) {
    t |> run("fold any") @(t : T?) {
        var query = (
            [for (x in 0..5); x]
            ._any(_ > 3)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold any false") @(t : T?) {
        var qpred2 = (
            [iterator for(x in 0..5); x]
            ._any(_ > 10)
            ._fold()
        )
        t |> success(!qpred2)
    }
    t |> run("fold all") @(t : T?) {
        var query = (
            [iterator for(x in 0..5); x]
            ._all(_ < 5)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold all empty") @(t : T?) {
        var qempty = (
            empty(type<int>)
            ._all(_ < 5)
            ._fold()
        )
        t |> success(qempty)
    }
    t |> run("fold contains") @(t : T?) {
        var query = (
            [iterator for(x in 0..5); x]
            .contains(3)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold complex contains") @(t : T?) {
        var qcomplex = (
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
            .contains(ComplexType(a = [3, 30]))
            ._fold()
        )
        t |> success(qcomplex)
    }
    t |> run("fold complex contains false") @(t : T?) {
        var qcomplex = (
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
            .contains(ComplexType(a = [3, 31]))
            ._fold()
        )
        t |> success(!qcomplex)
    }
}

[test]
def test_distinct_and_distinct_by(t : T?) {
    t |> run("basic distinct") @(t : T?) {
        var query = distinct(
            [iterator for(x in 0..5); x]
        )._fold()
        for (c, i in query, 0..5) {
            t |> equal(c, i)
        }
    }
    t |> run("complex distinct") @(t : T?) {
        var qcomplex = distinct(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )._fold()
        for (c, i in qcomplex, 0..5) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("distinc by") @(t : T?) {
        var qdistinctby = (
            [iterator for(x in 0..5); ComplexType(a = [x % 3, x * 10])]
            ._distinct_by(_.a[0])
            ._fold()
        )
        var found = [false, false, false]
        for (c in qdistinctby) {
            t |> success(!found[c.a[0]])
            found[c.a[0]] = true
        }
        for (i in 0..3) {
            t |> success(found[i])
        }
    }
}

[test]
def test_unique(t : T?) {
    t |> run("basic unique") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_ % 4)
            .union(
                [3, 4, 5, 6, 7]
                ._select(_ * _)
            )
            ._order_by(_) // sort
            ._fold()
        )
        var expected = [0, 1, 2, 3, 9, 16, 25, 36, 49]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
    t |> run("basic unique by") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_ % 4)
            ._union_by(
                [3, 4, 5, 6, 7]
                ._select(_ * _),
                _ % 4
            )
            ._order_by(_) // sort
            ._fold()
        )
        var expected = [0, 1, 2, 3]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_except(t : T?) {
    t |> run("basic except") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_ % 4)
            .except(
                [3, 4, 5, 6, 7]
                ._select(_ * _)
            )
            ._order_by(_) // sort
            .distinct()
            ._fold()
        )
        var expected = [0, 1, 2, 3]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
    t |> run("basic except by") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_ % 4)
            ._except_by(
                [3, 4, 5, 6, 7]
                ._select(_ * _),
                _ % 4
            )
            ._order_by(_) // sort
            .distinct()
            ._fold()
        )
        var expected = [2, 3]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_intersect(t : T?) {
    t |> run("basic intersect") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_)
            .intersect(
                [3, 4, 5, 6, 7]
                ._select(_)
            )
            ._order_by(_) // sort
            ._fold()
        )
        var expected = [3, 4, 5]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
    t |> run("basic intersect by") @(t : T?) {
        var t1 <- (
            [1, 2, 3, 4, 5]
            ._select(_)
            ._intersect_by(
                [3, 4, 5, 6, 7]
                ._select(_),
                _ % 4
            )
            ._order_by(_) // sort
            ._fold()
        )
        var expected = [3, 4, 5, 6]
        t |> equal(length(t1), length(expected))
        for (i, v in 0..length(expected), t1) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_chunk(t : T?) {
    t |> run("basic chunk") @(t : T?) {
        var query = (
            [for (x in 0..7); x]
            .chunk(2)
            ._select(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(query), "array<int>")
        let expected = [2, 2, 2, 1]
        t |> equal(length(expected), length(query))
        for (i, v in 0..length(expected), query) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_zip(t : T?) {
    t |> run("basic zip") @(t : T?) {
        var query = (
            [for (x in 0..6); x]
            ._select(_ * _)
            .zip(
                [for (y in 10..16); y]
                ._select(_)
            )
            ._fold()
        )
        t |> equal(typeinfo typename(query), "array<tuple<int;int>>")
        let expected = [(0, 10), (1, 11), (4, 12), (9, 13), (16, 14), (25, 15)]
        t |> equal(length(expected), length(query))
        for (i, v in 0..length(expected), query) {
            t |> equal(expected[i]._0, v._0)
            t |> equal(expected[i]._1, v._1)
        }
    }
    t |> run("zip with predicate") @(t : T?) {
        var query = (
            [for (x in 0..6); x]
            .zip(
                [for (y in 10..16); y]
                ._select(_),
                $(a, b : int) => a + b
            )
            ._fold()
        )
        t |> equal(typeinfo typename(query), "array<int>")
        let expected = [10, 12, 14, 16, 18, 20]
        t |> equal(length(expected), length(query))
        for (i, v in 0..length(expected), query) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_order_distinct(t : T?) {
    t |> run("basic order distinct") @(t : T?) {
        var t7 = (
            [5, 4, 4, 3, 4, 2, 1, 1, 4, 3]
            ._select(_ * 10)
            .order()
            .distinct()
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t7), "int")
        t |> equal(10, t7)
    }
    t |> run("first position order distinct") @(t : T?) {
        var t7 = (
            [5, 4, 4, 3, 4, 2, 1, 1, 4, 3]
            .order()
            .distinct()
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t7), "int")
        t |> equal(1, t7)
    }
}

