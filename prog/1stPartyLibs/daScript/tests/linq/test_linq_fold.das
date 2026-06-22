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
        var sorted = ["kiwi", "apple", "banana", "cherry", "blueberry"]    // nolint:LINT003
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
        var sorted = ["blueberry", "banana", "cherry", "apple", "kiwi"]    // nolint:LINT003
        for (i, v in 0..5, t8) {
            t |> equal(sorted[i], v)
        }
    }
}

[test]
def test_generic_fold_aggregates(t : T?) {
    t |> run("min fold") @(t : T?) {
        var t1 = ([5, 3, 8, 1, 4]    // nolint:LINT003 — `var` required for the `typename(t1) == "int"` assertion below (let adds const)
            .to_sequence()
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t1), "int")
        t |> equal(1, t1)
    }
    t |> run("min_by fold") @(t : T?) {
        var t2 = (["apple", "banana", "kiwi", "cherry", "blueberry"]    // nolint:LINT003
            .to_sequence()
            ._min_by(length(_))
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "string")
        t |> equal("kiwi", t2)
    }
    t |> run("max fold") @(t : T?) {
        var t2 = ([5, 3, 8, 1, 4]    // nolint:LINT003
            .to_sequence()
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(8, t2)
    }
    t |> run("max_by fold") @(t : T?) {
        var t2 = (["apple", "banana", "kiwi", "cherry", "blueberry"]    // nolint:LINT003
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
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]    // nolint:LINT003
            .to_sequence()
            .average()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "double")
        t |> equal(4.5lf, t2)
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
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]    // nolint:LINT003
            .to_sequence()
            .sum()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(36, t2)
    }
    t |> run("aggregate fold") @(t : T?) {
        var t2 = ([1, 2, 3, 4, 5, 6, 7, 8]    // nolint:LINT003
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
        var t1 = ([5, 3, 8, 1, 4]    // nolint:LINT003
            .to_sequence()
            .skip(2)
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t1), "int")
        t |> equal(1, t1)
    }
    t |> run("skip_while fold") @(t : T?) {
        var t2 = ([5, 3, 8, 1, 4]    // nolint:LINT003
            .to_sequence()
            ._skip_while(_ > 3)
            .min()
            ._fold()
        )
        t |> equal(typeinfo typename(t2), "int")
        t |> equal(1, t2)
    }
    t |> run("take fold") @(t : T?) {
        var t3 = ([5, 3, 8, 1, 4]    // nolint:LINT003
            .to_sequence()
            .take(3)
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t3), "int")
        t |> equal(8, t3)
    }
    t |> run("take_while fold") @(t : T?) {
        var t4 = ([5, 3, 8, 1, 4]    // nolint:LINT003
            .to_sequence()
            ._take_while(_ < 8)
            .max()
            ._fold()
        )
        t |> equal(typeinfo typename(t4), "int")
        t |> equal(5, t4)
    }
    t |> run("take range fold") @(t : T?) {
        var t5 = ([5, 3, 8, 1, 4]    // nolint:LINT003
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
        var expected = [6, 4, 2, 30, 27, 24]    // nolint:LINT003
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
        var expected = [6, 4, 2, 100]    // nolint:LINT003
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
        var expected = [100, 6, 4, 2]    // nolint:LINT003
        for (i, v in 0..4, q) {
            t |> equal(expected[i], v)
        }
    }
}

[test]
def test_any_all_contains(t : T?) {
    t |> run("fold any") @(t : T?) {
        var query = (    // nolint:LINT003
            [for (x in 0..5); x]
            ._any(_ > 3)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold any false") @(t : T?) {
        var qpred2 = (    // nolint:LINT003
            [iterator for(x in 0..5); x]
            ._any(_ > 10)
            ._fold()
        )
        t |> success(!qpred2)
    }
    t |> run("fold all") @(t : T?) {
        var query = (    // nolint:LINT003
            [iterator for(x in 0..5); x]
            ._all(_ < 5)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold all empty") @(t : T?) {
        var qempty = (    // nolint:LINT003
            empty(type<int>)
            ._all(_ < 5)
            ._fold()
        )
        t |> success(qempty)
    }
    t |> run("fold contains") @(t : T?) {
        var query = (    // nolint:LINT003
            [iterator for(x in 0..5); x]
            .contains(3)
            ._fold()
        )
        t |> success(query)
    }
    t |> run("fold complex contains") @(t : T?) {
        var qcomplex = (    // nolint:LINT003
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
            .contains(ComplexType(a = [3, 30]))
            ._fold()
        )
        t |> success(qcomplex)
    }
    t |> run("fold complex contains false") @(t : T?) {
        var qcomplex = (    // nolint:LINT003
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
        var expected = [0, 1, 2, 3, 9, 16, 25, 36, 49]    // nolint:LINT003
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
        var expected = [0, 1, 2, 3]    // nolint:LINT003
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
        var expected = [0, 1, 2, 3]    // nolint:LINT003
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
        var expected = [2, 3]    // nolint:LINT003
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
        var expected = [3, 4, 5]    // nolint:LINT003
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
        var expected = [3, 4, 5, 6]    // nolint:LINT003
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
        var t7 = (    // nolint:LINT003
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
        var t7 = (    // nolint:LINT003
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

[test]
def test_where_count_fold(t : T?) {
    // Guards the where + count fold rule in linq_boost (fold_where_count).
    // The fused loop emits a single counter pass with no intermediate filter array.
    t |> run("where + count: half match") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let c = _fold(each(arr)._where(_ > 5).count())
        t |> equal(typeinfo typename(c), "int const")
        t |> equal(5, c)
    }

    t |> run("where + count: zero matches") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let c = _fold(each(arr)._where(_ > 999).count())
        t |> equal(0, c)
    }

    t |> run("where + count: all match") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let c = _fold(each(arr)._where(_ > 0).count())
        t |> equal(5, c)
    }

    t |> run("where + count: empty source") @(t : T?) {
        let arr : array<int>
        let c = _fold(each(arr)._where(_ > 0).count())
        t |> equal(0, c)
    }
}

var g_proj_hits = 0

def projection_with_side_effect(x : int) : int {
    g_proj_hits ++
    return x * 2
}

[test]
def test_counter_lane_projection_side_effects(t : T?) {
    // Counter lane must evaluate the projection per matched element so user-visible
    // side effects fire — matches raw `count(select(src, f))` semantics. Tests guard
    // the projection-is-evaluated invariant after the Phase-2A planner fix.
    t |> run("select|count fires projection once per element") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        g_proj_hits = 0
        let c = _fold(each(arr)._select(projection_with_side_effect(_)).count())
        t |> equal(5, c)
        t |> equal(5, g_proj_hits)
    }
    t |> run("where|select|count fires projection only on matches") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        g_proj_hits = 0
        let c = _fold(each(arr)._where(_ > 5)._select(projection_with_side_effect(_)).count())
        t |> equal(5, c)
        t |> equal(5, g_proj_hits)
    }
}

// ===== Phase 2B Ring 1 — Accumulator lane (sum / min / max / average / long_count) =====

[test]
def test_sum_accumulator(t : T?) {
    t |> run("sum: empty") @(t : T?) {
        let arr : array<int>
        let s = _fold(each(arr).sum())
        t |> equal(0, s)
    }
    t |> run("sum: singleton") @(t : T?) {
        let arr <- [42]
        let s = _fold(each(arr).sum())
        t |> equal(42, s)
    }
    t |> run("sum: many") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr).sum())
        t |> equal(15, s)
    }
    t |> run("sum: where filter") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._where(_ > 2).sum())
        t |> equal(3 + 4 + 5, s)
    }
    t |> run("sum: select projection") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._select(_ * 10).sum())
        t |> equal(150, s)
    }
    t |> run("sum: where + select") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._where(_ > 2)._select(_ * 10).sum())
        t |> equal(120, s)
    }
}

[test]
def test_min_accumulator(t : T?) {
    t |> run("min: empty → default") @(t : T?) {
        let arr : array<int>
        let m = _fold(each(arr).min())
        // Matches linq.das min_impl: never assigned, returns default<T> = 0.
        t |> equal(0, m)
    }
    t |> run("min: singleton") @(t : T?) {
        let arr <- [7]
        let m = _fold(each(arr).min())
        t |> equal(7, m)
    }
    t |> run("min: many") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr).min())
        t |> equal(1, m)
    }
    t |> run("min: where filter") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr)._where(_ >= 3).min())
        t |> equal(3, m)
    }
    t |> run("min: select projection") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr)._select(_ * -1).min())
        t |> equal(-8, m)
    }
}

[test]
def test_max_accumulator(t : T?) {
    t |> run("max: empty → default") @(t : T?) {
        let arr : array<int>
        let m = _fold(each(arr).max())
        t |> equal(0, m)
    }
    t |> run("max: singleton") @(t : T?) {
        let arr <- [7]
        let m = _fold(each(arr).max())
        t |> equal(7, m)
    }
    t |> run("max: many") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr).max())
        t |> equal(8, m)
    }
    t |> run("max: where filter") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr)._where(_ <= 5).max())
        t |> equal(5, m)
    }
    t |> run("max: select projection") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        let m = _fold(each(arr)._select(_ * 2).max())
        t |> equal(16, m)
    }
}

[test]
def test_average_accumulator(t : T?) {
    t |> run("average: empty → 0.0lf (matches linq.das semantics)") @(t : T?) {
        let arr : array<int>
        let a = _fold(each(arr).average())
        t |> equal(0.0lf, a)
    }
    t |> run("average: singleton") @(t : T?) {
        let arr <- [42]
        let a = _fold(each(arr).average())
        t |> equal(42.0lf, a)
    }
    t |> run("average: many") @(t : T?) {
        let arr <- [2, 4, 6, 8]
        let a = _fold(each(arr).average())
        t |> equal(5.0lf, a)
    }
    t |> run("average: where filter") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let a = _fold(each(arr)._where(_ > 2).average())
        t |> equal(4.0lf, a)
    }
    t |> run("average: returns double for int source") @(t : T?) {
        let arr <- [1, 2, 3]
        let a = _fold(each(arr).average())
        t |> equal("double const", typeinfo typename(a))
        t |> equal(2.0lf, a)
    }
}

[test]
def test_long_count_accumulator(t : T?) {
    t |> run("long_count: empty") @(t : T?) {
        let arr : array<int>
        let c = _fold(each(arr).long_count())
        t |> equal(0l, c)
    }
    t |> run("long_count: many") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let c = _fold(each(arr).long_count())
        t |> equal(5l, c)
    }
    t |> run("long_count: where filter") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let c = _fold(each(arr)._where(_ > 2).long_count())
        t |> equal(3l, c)
    }
    t |> run("long_count: pure select hits length shortcut") @(t : T?) {
        // Pure projection + length-bearing source → planner elides loop, emits int64(length(src)).
        // Verified functionally here; AST shape test in test_linq_fold_ast.das.
        let arr <- [1, 2, 3, 4, 5]
        let c = _fold(each(arr)._select(_ * 2).long_count())
        t |> equal(5l, c)
    }
}
[test]
def test_first_early_exit(t : T?) {
    t |> run("first: singleton") @(t : T?) {
        let arr <- [42]
        let f = _fold(each(arr).first())
        t |> equal(42, f)
    }
    t |> run("first: many returns first") @(t : T?) {
        let arr <- [7, 8, 9]
        let f = _fold(each(arr).first())
        t |> equal(7, f)
    }
    t |> run("first: where matches returns first match") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let f = _fold(each(arr)._where(_ > 2).first())
        t |> equal(3, f)
    }
    t |> run("first: select returns projected first") @(t : T?) {
        let arr <- [1, 2, 3]
        let f = _fold(each(arr)._select(_ * 10).first())
        t |> equal(10, f)
    }
}

[test]
def test_first_or_default_early_exit(t : T?) {
    t |> run("first_or_default: empty returns default") @(t : T?) {
        let arr : array<int>
        let f = _fold(each(arr).first_or_default(99))
        t |> equal(99, f)
    }
    t |> run("first_or_default: non-empty returns first") @(t : T?) {
        let arr <- [7, 8, 9]
        let f = _fold(each(arr).first_or_default(99))
        t |> equal(7, f)
    }
    t |> run("first_or_default: where matches returns first match") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let f = _fold(each(arr)._where(_ > 3).first_or_default(0))
        t |> equal(4, f)
    }
    t |> run("first_or_default: where no match returns default") @(t : T?) {
        let arr <- [1, 2, 3]
        let f = _fold(each(arr)._where(_ > 100).first_or_default(-1))
        t |> equal(-1, f)
    }
}

[test]
def test_any_early_exit(t : T?) {
    t |> run("any: empty → false") @(t : T?) {
        let arr : array<int>
        let r = _fold(each(arr).any())
        t |> equal(false, r)
    }
    t |> run("any: non-empty (no pred) → true (shortcut path)") @(t : T?) {
        let arr <- [1]
        let r = _fold(each(arr).any())
        t |> equal(true, r)
    }
    t |> run("any: with where matching → true") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)._where(_ > 3).any())
        t |> equal(true, r)
    }
    t |> run("any: with where not matching → false") @(t : T?) {
        let arr <- [1, 2, 3]
        let r = _fold(each(arr)._where(_ > 100).any())
        t |> equal(false, r)
    }
    t |> run("any: with pred matching → true") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)._any(_ > 3))
        t |> equal(true, r)
    }
}

[test]
def test_all_early_exit(t : T?) {
    t |> run("all: empty → true (vacuous)") @(t : T?) {
        let arr : array<int>
        let r = _fold(each(arr)._all(_ > 0))
        t |> equal(true, r)
    }
    t |> run("all: all match → true") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)._all(_ > 0))
        t |> equal(true, r)
    }
    t |> run("all: one fails → false") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)._all(_ > 2))
        t |> equal(false, r)
    }
    t |> run("all: with upstream where") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // Only elements > 2 pass: 3, 4, 5. All of those > 0 → true.
        let r = _fold(each(arr)._where(_ > 2)._all(_ > 0))
        t |> equal(true, r)
    }
}

[test]
def test_contains_early_exit(t : T?) {
    t |> run("contains: empty → false") @(t : T?) {
        let arr : array<int>
        let r = _fold(each(arr).contains(3))
        t |> equal(false, r)
    }
    t |> run("contains: found → true") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr).contains(3))
        t |> equal(true, r)
    }
    t |> run("contains: not found → false") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr).contains(99))
        t |> equal(false, r)
    }
    t |> run("contains: select then compare against projected") @(t : T?) {
        let arr <- [1, 2, 3]
        // Projected values: [10, 20, 30]. contains(20) → true.
        let r = _fold(each(arr)._select(_ * 10).contains(20))
        t |> equal(true, r)
    }
}

[test]
def test_last_terminal_walk(t : T?) {
    t |> run("last: many returns final element") @(t : T?) {
        let arr <- [7, 8, 9]
        let l = _fold(each(arr).last())
        t |> equal(9, l)
    }
    t |> run("last: where matches returns final survivor") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let l = _fold(each(arr)._where(_ < 4).last())
        t |> equal(3, l)
    }
    t |> run("last: select projection returns projected last") @(t : T?) {
        let arr <- [1, 2, 3]
        let l = _fold(each(arr)._select(_ * 10).last())
        t |> equal(30, l)
    }
    t |> run("last: parity vs plain linq") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6]
        let spliced = _fold(each(arr)._where(_ % 2 == 0).last())
        let reference = arr |> where_($(x : int) => x % 2 == 0) |> last()
        t |> equal(reference, spliced)
    }
}

[test]
def test_last_or_default_terminal_walk(t : T?) {
    t |> run("last_or_default: empty returns default") @(t : T?) {
        let arr : array<int>
        let l = _fold(each(arr).last_or_default(99))
        t |> equal(99, l)
    }
    t |> run("last_or_default: non-empty returns last") @(t : T?) {
        let arr <- [7, 8, 9]
        let l = _fold(each(arr).last_or_default(99))
        t |> equal(9, l)
    }
    t |> run("last_or_default: where no match returns default") @(t : T?) {
        let arr <- [1, 2, 3]
        let l = _fold(each(arr)._where(_ > 100).last_or_default(-1))
        t |> equal(-1, l)
    }
    t |> run("last_or_default: where matches returns last survivor") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let l = _fold(each(arr)._where(_ < 4).last_or_default(-1))
        t |> equal(3, l)
    }
}

[test]
def test_single_terminal_walk(t : T?) {
    t |> run("single: exactly one match returns it") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._where(_ == 3).single())
        t |> equal(3, s)
    }
    t |> run("single: select projection on single element") @(t : T?) {
        let arr <- [4]
        let s = _fold(each(arr)._select(_ * 10).single())
        t |> equal(40, s)
    }
}

[test]
def test_single_or_default_terminal_walk(t : T?) {
    t |> run("single_or_default: empty returns default") @(t : T?) {
        let arr : array<int>
        let s = _fold(each(arr).single_or_default(99))
        t |> equal(99, s)
    }
    t |> run("single_or_default: exactly one match returns it") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._where(_ == 3).single_or_default(-1))
        t |> equal(3, s)
    }
    t |> run("single_or_default: more than one match returns default") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let s = _fold(each(arr)._where(_ > 2).single_or_default(-1))
        t |> equal(-1, s)
    }
    t |> run("single_or_default: where no match returns default") @(t : T?) {
        let arr <- [1, 2, 3]
        let s = _fold(each(arr)._where(_ > 100).single_or_default(-1))
        t |> equal(-1, s)
    }
}

[test]
def test_element_at_terminal_walk(t : T?) {
    t |> run("element_at: index 0 returns first") @(t : T?) {
        let arr <- [10, 20, 30]
        let e = _fold(each(arr).element_at(0))
        t |> equal(10, e)
    }
    t |> run("element_at: index N returns N-th") @(t : T?) {
        let arr <- [10, 20, 30, 40, 50]
        let e = _fold(each(arr).element_at(3))
        t |> equal(40, e)
    }
    t |> run("element_at: with where filter") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7]
        // Evens: [2,4,6]. element_at(1) → 4.
        let e = _fold(each(arr)._where(_ % 2 == 0).element_at(1))
        t |> equal(4, e)
    }
    t |> run("element_at: with select projection") @(t : T?) {
        let arr <- [1, 2, 3]
        // Projected: [10,20,30]. element_at(2) → 30.
        let e = _fold(each(arr)._select(_ * 10).element_at(2))
        t |> equal(30, e)
    }
    t |> run("element_at: parity vs plain linq") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7]
        let spliced = _fold(each(arr)._where(_ > 2).element_at(2))
        let reference = arr |> where_($(x : int) => x > 2) |> element_at(2)
        t |> equal(reference, spliced)
    }
}

[test]
def test_element_at_or_default_terminal_walk(t : T?) {
    t |> run("element_at_or_default: empty returns default") @(t : T?) {
        let arr : array<int>
        let e = _fold(each(arr).element_at_or_default(0))
        t |> equal(0, e)
    }
    t |> run("element_at_or_default: in-range returns it") @(t : T?) {
        let arr <- [10, 20, 30]
        let e = _fold(each(arr).element_at_or_default(1))
        t |> equal(20, e)
    }
    t |> run("element_at_or_default: out-of-range returns default") @(t : T?) {
        let arr <- [10, 20, 30]
        let e = _fold(each(arr).element_at_or_default(100))
        t |> equal(0, e)
    }
    t |> run("element_at_or_default: negative index returns default (no panic)") @(t : T?) {
        let arr <- [10, 20, 30]
        let e = _fold(each(arr).element_at_or_default(-5))
        t |> equal(0, e)
    }
}

[test]
def test_aggregate_terminal_walk(t : T?) {
    t |> run("aggregate: empty returns seed") @(t : T?) {
        let arr : array<int>
        let a = _fold(each(arr).aggregate(42, $(acc : int, x : int) => acc + x))
        t |> equal(42, a)
    }
    t |> run("aggregate: sum with multiplier") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let a = _fold(each(arr).aggregate(0, $(acc : int, x : int) => acc + x * 2))
        t |> equal(30, a)
    }
    t |> run("aggregate: with where filter") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // Odds: 1,3,5. sum-of-squares: 1+9+25 = 35.
        let a = _fold(each(arr)._where(_ % 2 == 1).aggregate(0, $(acc : int, x : int) => acc + x * x))
        t |> equal(35, a)
    }
    t |> run("aggregate: with select chain") @(t : T?) {
        let arr <- [1, 2, 3]
        // After select(*10): [10,20,30]. sum: 60.
        let a = _fold(each(arr)._select(_ * 10).aggregate(0, $(acc : int, x : int) => acc + x))
        t |> equal(60, a)
    }
    t |> run("aggregate: non-workhorse string seed") @(t : T?) {
        let arr <- [1, 2, 3]
        // String join — exercises the non-workhorse `<-` init/update/return path.
        let a = _fold(each(arr).aggregate("",
            $(acc : string, x : int) => acc == "" ? "{x}" : "{acc},{x}"))
        t |> equal("1,2,3", a)
    }
    t |> run("aggregate: parity vs plain linq") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6]
        let spliced = _fold(each(arr)._where(_ % 2 == 0).aggregate(1, $(acc : int, x : int) => acc * x))
        let reference = arr |> where_($(x : int) => x % 2 == 0) |> aggregate(1, $(acc : int, x : int) => acc * x)
        t |> equal(reference, spliced)
    }
}

[test]
def test_take_skip_counter_lane(t : T?) {
    t |> run("counter: where.take.count") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // Even values: 2,4,6,8,10 — take 3 → 3.
        let r = _fold(each(arr)._where(_ % 2 == 0).take(3).count())
        t |> equal(3, r)
    }
    t |> run("counter: where.skip.count") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // Even values: 2,4,6,8,10. skip 2 → 6,8,10 → 3.
        let r = _fold(each(arr)._where(_ % 2 == 0).skip(2).count())
        t |> equal(3, r)
    }
    t |> run("counter: where.skip.take.count") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // Even values: 2,4,6,8,10. skip 1 take 3 → 4,6,8 → 3.
        let r = _fold(each(arr)._where(_ % 2 == 0).skip(1).take(3).count())
        t |> equal(3, r)
    }
    t |> run("counter: take(0).count → 0") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr).take(0).count())
        t |> equal(0, r)
    }
    t |> run("counter: take(huge).count → all source") @(t : T?) {
        let arr <- [1, 2, 3]
        let r = _fold(each(arr).take(1000).count())
        t |> equal(3, r)
    }
    t |> run("counter: skip(huge).count → 0") @(t : T?) {
        let arr <- [1, 2, 3]
        let r = _fold(each(arr).skip(1000).count())
        t |> equal(0, r)
    }
}

[test]
def test_take_skip_accumulator_lane(t : T?) {
    t |> run("accumulator: select.take.sum") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // Projected: 10,20,30,40,50. take 3 → 10+20+30 = 60.
        let r = _fold(each(arr)._select(_ * 10).take(3).sum())
        t |> equal(60, r)
    }
    t |> run("accumulator: select.skip.take.average") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8]
        // Projected (* 10): 10,20,30,40,50,60,70,80. skip 2 take 4 → 30+40+50+60=180 / 4 = 45.
        let r = _fold(each(arr)._select(_ * 10).skip(2).take(4).average())
        t |> equal(45.0lf, r)
    }
    t |> run("accumulator: take.min") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        // take 3 → 5,3,8 → min=3.
        let r = _fold(each(arr).take(3).min())
        t |> equal(3, r)
    }
    t |> run("accumulator: where.take.long_count → int64") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // odd values: 1,3,5,7,9. take 3 → 3.
        let r = _fold(each(arr)._where(_ % 2 == 1).take(3).long_count())
        t |> equal(3l, r)
    }
    t |> run("accumulator: take(0).sum → 0") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr).take(0).sum())
        t |> equal(0, r)
    }
}

[test]
def test_take_skip_early_exit_lane(t : T?) {
    t |> run("early-exit: take.first") @(t : T?) {
        let arr <- [5, 3, 8, 1, 4]
        // take 3 → 5,3,8. first → 5.
        let r = _fold(each(arr).take(3).first())
        t |> equal(5, r)
    }
    t |> run("early-exit: where.take.any → true") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // even values: 2,4. take 5 → 2,4. any → true.
        let r = _fold(each(arr)._where(_ % 2 == 0).take(5).any())
        t |> equal(true, r)
    }
    t |> run("early-exit: where.skip.take.first → 6") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // even: 2,4,6,8,10. skip 2 take 3 → 6,8,10. first → 6.
        let r = _fold(each(arr)._where(_ % 2 == 0).skip(2).take(3).first())
        t |> equal(6, r)
    }
    t |> run("early-exit: take.all(odd) → false (even hits within take limit)") @(t : T?) {
        let arr <- [1, 3, 5, 2, 7]
        // take 5 → all five. all(odd) → false (2 fails).
        let r = _fold(each(arr).take(5).all($(x : int) => x % 2 == 1))
        t |> equal(false, r)
    }
    t |> run("early-exit: take(3).all(odd) → true (even excluded by take)") @(t : T?) {
        let arr <- [1, 3, 5, 2, 7]
        // take 3 → 1,3,5. all(odd) → true.
        let r = _fold(each(arr).take(3).all($(x : int) => x % 2 == 1))
        t |> equal(true, r)
    }
    t |> run("early-exit: take(0).any → false (empty)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r = _fold(each(arr).take(0).any())
        t |> equal(false, r)
    }
    t |> run("early-exit: take(0).all(p) → true (vacuous)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r = _fold(each(arr).take(0).all($(x : int) => x > 100))
        t |> equal(true, r)
    }
    t |> run("early-exit: take.first_or_default") @(t : T?) {
        let arr : array<int>
        let r = _fold(each(arr).take(5).first_or_default(-1))
        t |> equal(-1, r)
    }
    t |> run("early-exit: take.contains") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        // take 3 → 1,2,3. contains(4) → false (excluded by take).
        let r = _fold(each(arr).take(3).contains(4))
        t |> equal(false, r)
    }
}

[test]
def test_take_skip_array_lane(t : T?) {
    t |> run("array: take.to_array") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r <- _fold(each(arr).take(3).to_array())
        t |> equal(3, length(r))
        t |> equal(1, r[0])
        t |> equal(2, r[1])
        t |> equal(3, r[2])
    }
    t |> run("array: skip.take.to_array") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7]
        let r <- _fold(each(arr).skip(2).take(3).to_array())
        t |> equal(3, length(r))
        t |> equal(3, r[0])
        t |> equal(4, r[1])
        t |> equal(5, r[2])
    }
    t |> run("array: where.skip.take.to_array") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // even: 2,4,6,8,10. skip 1 take 3 → 4,6,8.
        let r <- _fold(each(arr)._where(_ % 2 == 0).skip(1).take(3).to_array())
        t |> equal(3, length(r))
        t |> equal(4, r[0])
        t |> equal(6, r[1])
        t |> equal(8, r[2])
    }
    t |> run("array: where.select.take.to_array") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5, 6, 7]
        // _ > 2 → 3,4,5,6,7. * 2 → 6,8,10,12,14. take 2 → 6,8.
        let r <- _fold(each(arr)._where(_ > 2)._select(_ * 2).take(2).to_array())
        t |> equal(2, length(r))
        t |> equal(6, r[0])
        t |> equal(8, r[1])
    }
    t |> run("array: take(0).to_array → empty") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).take(0).to_array())
        t |> equal(0, length(r))
    }
    t |> run("array: skip(huge).to_array → empty") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).skip(100).to_array())
        t |> equal(0, length(r))
    }
    // Pin non-positive N/K behavior — reference iterator semantics treat negative as
    // "take/skip nothing" (linq.das take_impl: `if (total <= 0) break`; skip_impl:
    // `if (total > 0) decrement-and-skip` so non-positive falls through).
    t |> run("array: take(-1).to_array → empty (matches take_impl)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).take(-1).to_array())
        t |> equal(0, length(r))
    }
    t |> run("array: take(0).to_array → empty (pins zero-N)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).take(0).to_array())
        t |> equal(0, length(r))
    }
    t |> run("array: skip(-1).to_array → full (non-positive skip is a no-op)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).skip(-1).to_array())
        t |> equal(3, length(r))
        t |> equal(1, r[0])
        t |> equal(2, r[1])
        t |> equal(3, r[2])
    }
    t |> run("array: skip(0).to_array → full (zero-K is a no-op)") @(t : T?) {
        let arr <- [1, 2, 3]
        let r <- _fold(each(arr).skip(0).to_array())
        t |> equal(3, length(r))
    }
}

[test]
def test_chained_non_workhorse_select(t : T?) {
    // Pre-Commit-2: planner rejected any chained _select whose previous projection was
    // non-workhorse (prevWorkhorse=false → return null), falling back to the iterator path.
    // Commit 2: chained-bind emission switched to `:=` (clone) — safe for every type — so
    // ComplexType intermediates now splice.
    t |> run("int → ComplexType → int → sum") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)
            ._select(ComplexType(a = [_ * 2]))    // int → ComplexType (non-workhorse)
            ._select(_.a[0])                       // ComplexType → int (reads field)
            .sum())
        // sum(2 + 4 + 6 + 8 + 10) = 30
        t |> equal(30, r)
    }
    t |> run("where then ComplexType intermediate then sum") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)
            ._where(_ >= 2)                        // predicate before selects
            ._select(ComplexType(a = [_ * 2, _ * 3]))
            ._select(_.a[0] + _.a[1])              // ComplexType → int (reads two fields)
            .sum())
        // filtered [2,3,4,5] → [(4+6),(6+9),(8+12),(10+15)] = [10,15,20,25] → sum = 70
        t |> equal(70, r)
    }
    t |> run("workhorse → ComplexType → workhorse → max") @(t : T?) {
        let arr <- [1, 2, 3, 4, 5]
        let r = _fold(each(arr)
            ._select(_ * 2)                        // int → int (workhorse)
            ._select(ComplexType(a = [_, _ + 1]))  // int → ComplexType (non-workhorse)
            ._select(_.a[1])                       // ComplexType → int
            .max())
        // _ * 2 → [2,4,6,8,10]; .a[1] → [3,5,7,9,11]; max = 11
        t |> equal(11, r)
    }
}

[test]
def test_iterator_source_bare_order(t : T?) {
    // Regression — splice path for bare order family on an iterator source. peel_each only
    // unwraps `each(<array>)`; `each(<iterator-yielding source>)` leaves an iterator on top.
    // Old planner unconditionally wrapped the emission with `to_sequence_move()` whenever
    // `expr._type.isIterator`, which compile-errored because daslib's `order_*(iter,…)`
    // overloads already return iterator and `to_sequence_move` is array-only.
    t |> run("each(range) |> order_by — splice emits iter-returning overload") @(tt : T?) {
        var query = _fold(each(range(0, 5))._order_by(-_))
        // range(0,5) = [0,1,2,3,4]; ordered by -v = [4,3,2,1,0]
        let expected = [4, 3, 2, 1, 0]
        for (v, i in query, 0 .. 5) {
            tt |> equal(expected[i], v)
        }
    }
}

[test]
def test_reverse_basic_parity(t : T?) {
    // Phase 3+ / R1: bare reverse + to_array splices into buffer + reverse_inplace.
    // Parity test — output element-by-element matches plain reverse().
    t |> run("reverse → to_array: 5-element array") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5]) |> reverse() |> to_array())
        let expected = [5, 4, 3, 2, 1]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("reverse → to_array: empty source") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(each(empty) |> reverse() |> to_array())
        tt |> equal(0, length(got))
    }
    t |> run("reverse → to_array: single-element source") @(tt : T?) {
        let got <- _fold(each([42]) |> reverse() |> to_array())
        tt |> equal(1, length(got))
        tt |> equal(42, got[0])
    }
}

[test]
def test_reverse_with_where_parity(t : T?) {
    // R2: where_ before reverse — buffer accumulates filtered survivors, then reverses.
    t |> run("where(>2).reverse: [1..5] → survivors [3,4,5] reversed = [5,4,3]") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> to_array())
        let expected = [5, 4, 3]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("where(false).reverse: empty survivor list → empty result") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._where(_ > 999) |> reverse() |> to_array())
        tt |> equal(0, length(got))
    }
    t |> run("where(true).reverse: identity filter equivalent to bare reverse") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._where(_ > 0) |> reverse() |> to_array())
        let expected = [5, 4, 3, 2, 1]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("where.where.reverse: fused predicates with &&") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._where(_ > 1)._where(_ < 5) |> reverse() |> to_array())
        let expected = [4, 3, 2]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
}

[test]
def test_reverse_with_select_parity(t : T?) {
    // R3: select before reverse — projected values land in the buffer, then reverse.
    t |> run("select(x*10).reverse: [1..5] → [10,20,30,40,50] reversed = [50,40,30,20,10]") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._select(_ * 10) |> reverse() |> to_array())
        let expected = [50, 40, 30, 20, 10]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("where.select.reverse: filter then project then reverse") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5])._where(_ > 2)._select(_ * 10) |> reverse() |> to_array())
        let expected = [50, 40, 30]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("select(non-workhorse ComplexType).reverse parity") @(tt : T?) {
        let got <- _fold(each([1, 2, 3])._select(ComplexType(a = [_ * 100, _ * 200])) |> reverse() |> to_array())
        tt |> equal(3, length(got))
        // [{a=[100,200]}, {a=[200,400]}, {a=[300,600]}] reversed
        tt |> equal(300, got[0].a[0])
        tt |> equal(200, got[1].a[0])
        tt |> equal(100, got[2].a[0])
    }
}

[test]
def test_reverse_with_take_parity(t : T?) {
    // R4: take(N) after reverse — resize the reversed buffer to keep first N.
    // Equivalent to "last N of source in reverse".
    t |> run("reverse.take(3): [1..8] → reversed [8,7,6,5,4,3,2,1] take 3 = [8,7,6]") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5, 6, 7, 8]) |> reverse() |> take(3) |> to_array())
        let expected = [8, 7, 6]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("reverse.take(0): empty result") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5]) |> reverse() |> take(0) |> to_array())
        tt |> equal(0, length(got))
    }
    t |> run("reverse.take(-1): non-positive N → empty (matches take_impl)") @(tt : T?) {
        let got <- _fold(each([1, 2, 3, 4, 5]) |> reverse() |> take(-1) |> to_array())
        tt |> equal(0, length(got))
    }
    t |> run("reverse.take(N > length): no truncation, just reversed") @(tt : T?) {
        let got <- _fold(each([1, 2, 3]) |> reverse() |> take(999) |> to_array())
        let expected = [3, 2, 1]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("where.reverse.take: filter then reverse-take last 2 survivors") @(tt : T?) {
        // survivors: [3,4,5,6]; reversed: [6,5,4,3]; take 2: [6,5]
        let got <- _fold(each([1, 2, 3, 4, 5, 6])._where(_ > 2) |> reverse() |> take(2) |> to_array())
        let expected = [6, 5]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
}

[test]
def test_distinct_fold_parity(t : T?) {
    // D1: bare distinct → to_array splices into streaming dedup. Verify identical
    // output to plain distinct(). Uses `.to_sequence()` (const-iterator source) instead
    // of `each(...)` (non-const-iterator) to dodge the iterator-variance mangler ICE
    // documented in iterator_variance_and_mangler_ice — distinct ships both flavors
    // and a non-const-flavored caller currently hash-collides.
    t |> run("distinct: [1,2,2,3,3,3] → [1,2,3]") @(tt : T?) {
        let got <- _fold([1, 2, 2, 3, 3, 3].to_sequence().distinct().to_array())
        let expected = [1, 2, 3]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("distinct preserves first-seen order") @(tt : T?) {
        let got <- _fold([3, 1, 4, 1, 5, 9, 2, 6, 5, 3].to_sequence().distinct().to_array())
        let expected = [3, 1, 4, 5, 9, 2, 6]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("distinct: empty source") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty.to_sequence().distinct().to_array())
        tt |> equal(0, length(got))
    }
    t |> run("distinct: all-unique source = identity") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5].to_sequence().distinct().to_array())
        tt |> equal(5, length(got))
        for (v, e in got, [1, 2, 3, 4, 5]) {
            tt |> equal(v, e)
        }
    }
    t |> run("distinct: all-same source = single element") @(tt : T?) {
        let got <- _fold([7, 7, 7, 7, 7].to_sequence().distinct().to_array())
        tt |> equal(1, length(got))
        tt |> equal(7, got[0])
    }
}

[test]
def test_distinct_extended_parity(t : T?) {
    // D2: where_ before distinct
    t |> run("where(>1).distinct: [1,2,3,2,4,1,5] → survivors [2,3,4,5] distinct") @(tt : T?) {
        let got <- _fold([1, 2, 3, 2, 4, 1, 5].to_sequence()._where(_ > 1).distinct().to_array())
        let expected = [2, 3, 4, 5]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    // D3: select before distinct
    t |> run("select(_%3).distinct: [1,2,3,4] → projected [1,2,0,1] distinct = [1,2,0]") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4].to_sequence()._select(_ % 3).distinct().to_array())
        let expected = [1, 2, 0]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    // D4: distinct_by — dedup keyed by mod-10. Keys: 0,1,2,1,3,2 → first per bucket
    t |> run("distinct_by(x%10): [10,21,32,11,23,42] → first per bucket [10,21,32,23]") @(tt : T?) {
        let got <- _fold([10, 21, 32, 11, 23, 42].to_sequence()._distinct_by(_ % 10).to_array())
        let expected = [10, 21, 32, 23]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    // D5: take(N) early-exit
    t |> run("distinct.take(3): [1,1,2,2,3,3,4,4,5,5] → first 3 distinct = [1,2,3]") @(tt : T?) {
        let got <- _fold([1, 1, 2, 2, 3, 3, 4, 4, 5, 5].to_sequence().distinct().take(3).to_array())
        let expected = [1, 2, 3]
        tt |> equal(length(got), length(expected))
        for (v, e in got, expected) {
            tt |> equal(v, e)
        }
    }
    t |> run("distinct.take(0): zero early-exit yields empty") @(tt : T?) {
        let got <- _fold([1, 2, 3].to_sequence().distinct().take(0).to_array())
        tt |> equal(0, length(got))
    }
    t |> run("distinct.take(-1): negative N → empty (matches take_impl)") @(tt : T?) {
        let got <- _fold([1, 2, 3].to_sequence().distinct().take(-1).to_array())
        tt |> equal(0, length(got))
    }
    // D6: count terminator
    t |> run("distinct.count: [1,2,2,3,3,3] → 3 distinct values") @(tt : T?) {
        tt |> equal(3, _fold([1, 2, 2, 3, 3, 3].to_sequence().distinct().count()))
    }
    t |> run("where.distinct.count: filter then count distinct") @(tt : T?) {
        tt |> equal(3, _fold([1, 2, 2, 3, 3, 3, 1].to_sequence()._where(_ > 0).distinct().count()))
    }
    t |> run("distinct.count on empty source: 0") @(tt : T?) {
        var empty : array<int>
        tt |> equal(0, _fold(empty.to_sequence().distinct().count()))
    }
    // D7: sum / long_count
    t |> run("distinct.sum: [1,2,2,3,3,3,4] → 1+2+3+4 = 10") @(tt : T?) {
        tt |> equal(10, _fold([1, 2, 2, 3, 3, 3, 4].to_sequence().distinct().sum()))
    }
    t |> run("distinct.long_count: [1,2,2,3,3,3] → int64 3") @(tt : T?) {
        tt |> equal(3l, _fold([1, 2, 2, 3, 3, 3].to_sequence().distinct().long_count()))
    }
    // D8: predicate-form terminators must NOT be silently dropped — plan_distinct guards arg count and cascades to tier 2.
    t |> run("distinct.count(pred) parity: [1,1,2,2,3,3,4,4,5,5].distinct.count(>2) → 3 (not 5)") @(tt : T?) {
        let arr = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5]
        let got = _fold(arr.to_sequence().distinct().count($(x : int) => x > 2))
        let raw = arr.to_sequence().distinct().count($(x : int) => x > 2)
        tt |> equal(got, raw, "predicate must be honored, not dropped")
        tt |> equal(got, 3)
    }
    t |> run("distinct.long_count(pred) parity: [1,1,2,2,3,3,4,4,5,5].distinct.long_count(>=3) → 3l") @(tt : T?) {
        let arr = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5]
        let got = _fold(arr.to_sequence().distinct().long_count($(x : int) => x >= 3))
        let raw = arr.to_sequence().distinct().long_count($(x : int) => x >= 3)
        tt |> equal(got, raw, "predicate must be honored, not dropped")
        tt |> equal(got, 3l)
    }
}

[test]
def test_group_by_count_fold_parity(t : T?) {
    // G2/G3: group_by + select(count) splice should produce same per-bucket counts
    // as the plain LINQ chain.
    t |> run("group_by(x%3).select(_._1|>length): [1..7] → counts of buckets") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select(_._1 |> length))
        // Source by mod 3: 0→{3,6}, 1→{1,4,7}, 2→{2,5}. Counts: 2, 3, 2 (table order varies).
        tt |> equal(3, length(got))
        var total = 0
        for (c in got) {
            total += c
        }
        tt |> equal(7, total)
    }
    t |> run("group_by + named tuple wrap preserves key + count") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)))
        tt |> equal(3, length(got))
        // Per-key expectations: 0→{3,6}=2, 1→{1,4,7}=3, 2→{2,5}=2. Tests bucket assignment, not just total.
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.N
        }
        tt |> equal(2, perKey[0])
        tt |> equal(3, perKey[1])
        tt |> equal(2, perKey[2])
    }
    t |> run("group_by on empty source → empty result") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by(_ % 3)._select(_._1 |> length))
        tt |> equal(0, length(got))
    }
    // G8: count terminator → number of buckets
    t |> run("group_by.count: number of distinct buckets") @(tt : T?) {
        tt |> equal(3, _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)).count()))
    }
    // G4: sum reducer — per-bucket sum of elements
    t |> run("group_by.select(_._1|>sum): bare-sum bucket reducer") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> sum))
        // Buckets: 0→{30,21,12}=63, 1→{10,31}=41, 2→{20,11}=31. Total = 135.
        tt |> equal(3, length(got))
        var totalSum = 0
        for (s in got) {
            totalSum += s
        }
        tt |> equal(135, totalSum)
    }
    t |> run("group_by + named tuple sum: preserves K + Sum") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Sum = _._1 |> sum)))
        tt |> equal(3, length(got))
        // Per-key expectations: 0→63, 1→41, 2→31. Tests bucket assignment, not just total.
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.Sum
        }
        tt |> equal(63, perKey[0])
        tt |> equal(41, perKey[1])
        tt |> equal(31, perKey[2])
    }
    // G5: long_count reducer — per-bucket int64 count
    t |> run("group_by.select(_._1|>long_count): per-bucket int64 count") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7]._group_by(_ % 3)._select(_._1 |> long_count))
        tt |> equal(3, length(got))
        var totalCount = 0l
        for (c in got) {
            totalCount += c
        }
        tt |> equal(7l, totalCount)
    }
}

[test]
def test_group_by_inner_select_sum_fold_parity(t : T?) {
    // G4 (PR-A1): inner-select-sum recognized via is_bucket_reducer_call's new arm.
    // Splice inlines the inner projection into the per-element accumulator update so the
    // bucket array is never materialized and the inner select-call disappears from the body.
    t |> run("bare inner-select-sum: per-bucket sum of projected values") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> select(@(x : int) => x * 2) |> sum))
        // Buckets: 0→{30,21,12} *2 = 126; 1→{10,31} *2 = 82; 2→{20,11} *2 = 62. Total = 270.
        tt |> equal(3, length(got))
        var total = 0
        for (s in got) {
            total += s
        }
        tt |> equal(270, total)
    }
    t |> run("named inner-select-sum: preserves K + projected sum") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, T = _._1 |> select(@(x : int) => x * 2) |> sum)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.T
        }
        tt |> equal(126, perKey[0])   // (30+21+12)*2
        tt |> equal(82,  perKey[1])   // (10+31)*2
        tt |> equal(62,  perKey[2])   // (20+11)*2
    }
    t |> run("inner-select-sum on empty source → empty result") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by(_ % 3)._select(_._1 |> select(@(x : int) => x * 2) |> sum))
        tt |> equal(0, length(got))
    }
    t |> run("inner-select-sum matches plain LINQ reference (parity)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12, 22, 32, 13]
        let foldGot <- _fold(arr._group_by(_ % 4)._select((K = _._0, T = _._1 |> select(@(x : int) => x + 1) |> sum)))
        let refGot  <- (arr._group_by(_ % 4)._select((K = _._0, T = _._1 |> select(@(x : int) => x + 1) |> sum)))
        tt |> equal(length(refGot), length(foldGot))
        var refMap : table<int; int>
        var foldMap : table<int; int>
        for (r in refGot) {
            refMap[r.K] = r.T
        }
        for (f in foldGot) {
            foldMap[f.K] = f.T
        }
        // Use ?[k] (safe lookup, no auto-insert) to avoid locking refMap while iterating its keys.
        for (k in keys(refMap)) {
            tt |> equal(refMap?[k] ?? -1, foldMap?[k] ?? -1)
        }
    }
}

[test]
def test_group_by_min_max_first_fold_parity(t : T?) {
    // PR-A2: min/max/first bare + named + inner-select reducers. Each splices into the
    // miss/hit emission with per-reducer init+update shapes (min/max compare; first no-op on hit).
    t |> run("bare min: per-bucket workhorse min") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> min))
        // Buckets: 0→{30,21,12}=12, 1→{10,31}=10, 2→{20,11}=11. Total of mins = 33.
        tt |> equal(3, length(got))
        var totalMin = 0
        for (m in got) {
            totalMin += m
        }
        tt |> equal(33, totalMin)
    }
    t |> run("named min: preserves K + Min") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Min = _._1 |> min)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.Min
        }
        tt |> equal(12, perKey[0])
        tt |> equal(10, perKey[1])
        tt |> equal(11, perKey[2])
    }
    t |> run("bare max: per-bucket workhorse max") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> max))
        // Buckets: 0→30, 1→31, 2→20. Sum = 81.
        tt |> equal(3, length(got))
        var totalMax = 0
        for (m in got) {
            totalMax += m
        }
        tt |> equal(81, totalMax)
    }
    t |> run("named max: preserves K + Max") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Max = _._1 |> max)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.Max
        }
        tt |> equal(30, perKey[0])
        tt |> equal(31, perKey[1])
        tt |> equal(20, perKey[2])
    }
    t |> run("bare first: per-bucket FIRST source element (first-key-wins)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> first))
        // Buckets, in source order: 1→{10,31}=10, 2→{20,11}=20, 0→{30,21,12}=30. Sum = 60.
        tt |> equal(3, length(got))
        var totalFirst = 0
        for (f in got) {
            totalFirst += f
        }
        tt |> equal(60, totalFirst)
    }
    t |> run("named first: preserves K + First (first source elem per key)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, First = _._1 |> first)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.First
        }
        tt |> equal(30, perKey[0])    // 30 is the first elem for key 0
        tt |> equal(10, perKey[1])    // 10 is the first elem for key 1
        tt |> equal(20, perKey[2])    // 20 is the first elem for key 2
    }
    t |> run("inner-select-min: bare min of projected values") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> select(@(x : int) => x * 2) |> min))
        // Buckets *2: 0→{60,42,24}=24, 1→{20,62}=20, 2→{40,22}=22. Sum = 66.
        tt |> equal(3, length(got))
        var totalMin = 0
        for (m in got) {
            totalMin += m
        }
        tt |> equal(66, totalMin)
    }
    t |> run("inner-select-max: named (K, M) with projected max") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, M = _._1 |> select(@(x : int) => x * 2) |> max)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.M
        }
        tt |> equal(60, perKey[0])    // max(30,21,12)*2 = 60
        tt |> equal(62, perKey[1])    // max(10,31)*2 = 62
        tt |> equal(40, perKey[2])    // max(20,11)*2 = 40
    }
    t |> run("inner-select-first: named (K, F) with projected first") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, F = _._1 |> select(@(x : int) => x * 2) |> first)))
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.F
        }
        tt |> equal(60, perKey[0])    // first elem 30, *2 = 60
        tt |> equal(20, perKey[1])    // first elem 10, *2 = 20
        tt |> equal(40, perKey[2])    // first elem 20, *2 = 40
    }
    t |> run("min/max/first on empty source → empty result") @(tt : T?) {
        var empty : array<int>
        let g1 <- _fold(empty._group_by(_ % 3)._select(_._1 |> min))
        tt |> equal(0, length(g1))
        let g2 <- _fold(empty._group_by(_ % 3)._select(_._1 |> max))
        tt |> equal(0, length(g2))
        let g3 <- _fold(empty._group_by(_ % 3)._select(_._1 |> first))
        tt |> equal(0, length(g3))
    }
    t |> run("min on struct (non-workhorse) Person.age via inner-select: parity with reference") @(tt : T?) {
        let foldGot <- _fold(_common::people._group_by(_.name)._select((K = _._0, MinAge = _._1 |> select(@(p : _common::Person) => p.age) |> min)))
        let refGot  <- (_common::people._group_by(_.name)._select((K = _._0, MinAge = _._1 |> select(@(p : _common::Person) => p.age) |> min)))
        tt |> equal(length(refGot), length(foldGot))
        var refMap  : table<string; int>
        var foldMap : table<string; int>
        for (r in refGot) {
            refMap[r.K] = r.MinAge
        }
        for (f in foldGot) {
            foldMap[f.K] = f.MinAge
        }
        for (k in keys(refMap)) {
            tt |> equal(refMap?[k] ?? -1, foldMap?[k] ?? -1)
        }
    }
}

[test]
def test_group_by_multi_reducer_fold_parity(t : T?) {
    // PR-A2 A.4: N-slot named tuples fuse multiple reducers into a single per-element pass.
    t |> run("3-slot (K, N=length, S=sum): two reducers per bucket") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, N = _._1 |> length, S = _._1 |> sum)))
        tt |> equal(3, length(got))
        var perKeyN : table<int; int>
        var perKeyS : table<int; int>
        for (item in got) {
            perKeyN[item.K] = item.N
            perKeyS[item.K] = item.S
        }
        // Per key: 0→{30,21,12} N=3 S=63; 1→{10,31} N=2 S=41; 2→{20,11} N=2 S=31.
        tt |> equal(3, perKeyN[0])
        tt |> equal(63, perKeyS[0])
        tt |> equal(2, perKeyN[1])
        tt |> equal(41, perKeyS[1])
        tt |> equal(2, perKeyN[2])
        tt |> equal(31, perKeyS[2])
    }
    t |> run("4-slot (K, N, S, MX): count + sum + max fused into one pass") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
            (K = _._0, N = _._1 |> length, S = _._1 |> sum, MX = _._1 |> max)))
        tt |> equal(3, length(got))
        // Compare slot-by-slot since named tuples lack a default operator==.
        var keysSeen = 0
        for (item in got) {
            if (item.K == 0) { tt |> equal(3, item.N); tt |> equal(63, item.S); tt |> equal(30, item.MX); keysSeen ++; }
            elif (item.K == 1) { tt |> equal(2, item.N); tt |> equal(41, item.S); tt |> equal(31, item.MX); keysSeen ++; }
            elif (item.K == 2) { tt |> equal(2, item.N); tt |> equal(31, item.S); tt |> equal(20, item.MX); keysSeen ++; }
        }
        tt |> equal(3, keysSeen)
    }
    t |> run("multi-reducer with inner-select: (K, N, Total = sum of *2)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
            (K = _._0, N = _._1 |> length, Total = _._1 |> select(@(x : int) => x * 2) |> sum)))
        tt |> equal(3, length(got))
        var keysSeen = 0
        for (item in got) {
            if (item.K == 0) { tt |> equal(3, item.N); tt |> equal(126, item.Total); keysSeen ++; }
            elif (item.K == 1) { tt |> equal(2, item.N); tt |> equal(82,  item.Total); keysSeen ++; }
            elif (item.K == 2) { tt |> equal(2, item.N); tt |> equal(62,  item.Total); keysSeen ++; }
        }
        tt |> equal(3, keysSeen)
    }
    t |> run("multi-reducer on empty source → empty result") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by(_ % 3)._select((K = _._0, N = _._1 |> length, S = _._1 |> sum)))
        tt |> equal(0, length(got))
    }
    t |> run("multi-reducer parity with reference (Person)") @(tt : T?) {
        let foldGot <- _fold(_common::people._group_by(_.name)._select(
            (K = _._0, N = _._1 |> length, MinAge = _._1 |> select(@(p : _common::Person) => p.age) |> min, MaxAge = _._1 |> select(@(p : _common::Person) => p.age) |> max)))
        let refGot  <- (_common::people._group_by(_.name)._select(
            (K = _._0, N = _._1 |> length, MinAge = _._1 |> select(@(p : _common::Person) => p.age) |> min, MaxAge = _._1 |> select(@(p : _common::Person) => p.age) |> max)))
        tt |> equal(length(refGot), length(foldGot))
        // Three separate tables — daslang tuples lack a default `==` operator.
        var refN, refMin, refMax : table<string; int>
        var foldN, foldMin, foldMax : table<string; int>
        for (r in refGot) {
            refN[r.K] = r.N; refMin[r.K] = r.MinAge; refMax[r.K] = r.MaxAge
        }
        for (f in foldGot) {
            foldN[f.K] = f.N; foldMin[f.K] = f.MinAge; foldMax[f.K] = f.MaxAge
        }
        for (k in keys(refN)) {
            tt |> equal(refN?[k] ?? -1,   foldN?[k] ?? -1)
            tt |> equal(refMin?[k] ?? -1, foldMin?[k] ?? -1)
            tt |> equal(refMax?[k] ?? -1, foldMax?[k] ?? -1)
        }
    }
}

[test]
def test_group_by_average_fold_parity(t : T?) {
    // PR-A2 follow-up: average reducer splices via 2-slot per-key accumulator
    // (sum : double + count : uint64) with post-process division at result-build.
    // Each bucket is non-empty by construction → no divide-by-zero guard needed.
    t |> run("bare _._1 |> average") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> average))
        // Buckets: 0→{30,21,12} avg=21.0; 1→{10,31} avg=20.5; 2→{20,11} avg=15.5
        tt |> equal(3, length(got))
        var sumAll = 0lf
        for (v in got) {
            sumAll += v
        }
        tt |> equal(57lf, sumAll)
    }
    t |> run("named (K=_._0, Avg=_._1|>average) — field name + key preserved") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Avg = _._1 |> average)))
        tt |> equal(3, length(got))
        var perKey : table<int; double>
        for (item in got) {
            perKey[item.K] = item.Avg
        }
        tt |> equal(21.0lf, perKey[0])
        tt |> equal(20.5lf, perKey[1])
        tt |> equal(15.5lf, perKey[2])
    }
    t |> run("inner-select-average — bare with inner projection") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> select(@(x : int) => x * 2) |> average))
        // Inner projects x→x*2. Bucket sums double, count stays the same → averages also double.
        tt |> equal(3, length(got))
        var perCount : table<int; int>
        for (v in got) {
            perCount[int(v)] = (perCount?[int(v)] ?? 0) + 1
        }
        // Avgs (doubled): 42.0, 41.0, 31.0 — all integers in this fixture so int round-trip is safe.
        tt |> equal(1, perCount[42])
        tt |> equal(1, perCount[41])
        tt |> equal(1, perCount[31])
    }
    t |> run("inner-select-average (named): (K, Avg = select(x*2) |> average)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Avg = _._1 |> select(@(x : int) => x * 2) |> average)))
        tt |> equal(3, length(got))
        var perKey : table<int; double>
        for (item in got) {
            perKey[item.K] = item.Avg
        }
        tt |> equal(42.0lf, perKey[0])
        tt |> equal(41.0lf, perKey[1])
        tt |> equal(31.0lf, perKey[2])
    }
    t |> run("multi-reducer with average: (K, N, S, Avg)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
            (K = _._0, N = _._1 |> length, S = _._1 |> sum, Avg = _._1 |> average)))
        tt |> equal(3, length(got))
        var keysSeen = 0
        for (item in got) {
            if (item.K == 0) { tt |> equal(3, item.N); tt |> equal(63, item.S); tt |> equal(21.0lf, item.Avg); keysSeen ++; }
            elif (item.K == 1) { tt |> equal(2, item.N); tt |> equal(41, item.S); tt |> equal(20.5lf, item.Avg); keysSeen ++; }
            elif (item.K == 2) { tt |> equal(2, item.N); tt |> equal(31, item.S); tt |> equal(15.5lf, item.Avg); keysSeen ++; }
        }
        tt |> equal(3, keysSeen)
    }
    t |> run("multi-reducer with 2 averages: (K, AvgRaw, AvgDoubled)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(
            (K = _._0, AvgRaw = _._1 |> average, AvgDoubled = _._1 |> select(@(x : int) => x * 2) |> average)))
        tt |> equal(3, length(got))
        for (item in got) {
            // Doubled inner-select gives 2× raw average.
            tt |> equal(item.AvgRaw * 2.0lf, item.AvgDoubled)
        }
    }
    t |> run("average on empty source → empty result") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by(_ % 3)._select(_._1 |> average))
        tt |> equal(0, length(got))
    }
    t |> run("average parity with reference plain LINQ") @(tt : T?) {
        let foldGot <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Avg = _._1 |> average)))
        let refGot  <- ([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((K = _._0, Avg = _._1 |> average)))
        tt |> equal(length(refGot), length(foldGot))
        var refAvg, foldAvg : table<int; double>
        for (r in refGot) {
            refAvg[r.K] = r.Avg
        }
        for (f in foldGot) {
            foldAvg[f.K] = f.Avg
        }
        for (k in keys(refAvg)) {
            tt |> equal(refAvg?[k] ?? -1.0lf, foldAvg?[k] ?? -1.0lf)
        }
    }
    t |> run("float source — average over array<float>") @(tt : T?) {
        let got <- _fold([1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f]._group_by(int(_) % 2)._select((K = _._0, Avg = _._1 |> average)))
        tt |> equal(2, length(got))
        var perKey : table<int; double>
        for (item in got) {
            perKey[item.K] = item.Avg
        }
        // Bucket 0: {2,4,6}=12/3=4.0; bucket 1: {1,3,5}=9/3=3.0.
        tt |> equal(4.0lf, perKey[0])
        tt |> equal(3.0lf, perKey[1])
    }
    t |> run("positional tuple with average: (_._0, _._1|>average) — no field names") @(tt : T?) {
        // Recognizer accepts positional tuples (ExprMakeTuple with bind._0 at slot 0); the
        // result-build synthesis must not assume argNames is populated (it's empty for
        // positional). Use length(argTypes) for slot count, only fill recordNames when names
        // are present.
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select((_._0, _._1 |> average)))
        tt |> equal(3, length(got))
        var perKey : table<int; double>
        for (item in got) {
            perKey[item._0] = item._1
        }
        tt |> equal(21.0lf, perKey[0])
        tt |> equal(20.5lf, perKey[1])
        tt |> equal(15.5lf, perKey[2])
    }
}

[test]
def test_group_by_upstream_fusion_fold_parity(t : T?) {
    // PR-B: upstream where_/select fuses into the per-element loop. Where guards the
    // table update; select binds projected values that the key/reducer chain references.
    t |> run("where → group_by → count: only survivors enter the table") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ % 2 == 0)._group_by(_ % 3)._select(_._1 |> length))
        // Survivors: [2,4,6,8,10]; buckets: 0→{6}, 1→{4,10}, 2→{2,8}. Counts: 1,2,2 = 5.
        tt |> equal(3, length(got))
        var total = 0
        for (c in got) {
            total += c
        }
        tt |> equal(5, total)
    }
    t |> run("select → group_by → sum: key on projected value") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._select(_ * 2)._group_by(_ % 3)._select((K = _._0, S = _._1 |> sum)))
        // Projected: [20,40,60,22,42,62,24]. Mod 3: 20%3=2, 40%3=1, 60%3=0, 22%3=1, 42%3=0, 62%3=2, 24%3=0.
        // Buckets: 0→{60,42,24}=126; 1→{40,22}=62; 2→{20,62}=82.
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.S
        }
        tt |> equal(126, perKey[0])
        tt |> equal(62,  perKey[1])
        tt |> equal(82,  perKey[2])
    }
    t |> run("where → select → group_by → sum: filter then project") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ > 3)._select(_ * 10)._group_by(_ % 3)._select((K = _._0, S = _._1 |> sum)))
        // Survivors: [4,5,6,7,8,9,10]; projected: [40,50,60,70,80,90,100]; mod 3: 40%3=1, 50%3=2, 60%3=0, 70%3=1, 80%3=2, 90%3=0, 100%3=1.
        // Buckets: 0→{60,90}=150; 1→{40,70,100}=210; 2→{50,80}=130.
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.S
        }
        tt |> equal(150, perKey[0])
        tt |> equal(210, perKey[1])
        tt |> equal(130, perKey[2])
    }
    t |> run("select → where → group_by → sum: project then filter") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._select(_ * 2)._where(_ > 30)._group_by(_ % 3)._select((K = _._0, S = _._1 |> sum)))
        // Projected: [20,40,60,22,42,62,24]; survivors (>30): [40,60,42,62]; mod 3: 1, 0, 0, 2.
        // Buckets: 0→{60,42}=102; 1→{40}=40; 2→{62}=62.
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.S
        }
        tt |> equal(102, perKey[0])
        tt |> equal(40,  perKey[1])
        tt |> equal(62,  perKey[2])
    }
    t |> run("chained selects → group_by: v0 → v1 → ... → final bind") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5, 6, 7]._select(_ * 2)._select(_ + 1)._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)))
        // [1..7] * 2: [2,4,6,8,10,12,14]; +1: [3,5,7,9,11,13,15]; mod 3: 0,2,1,0,2,1,0.
        // Buckets: 0→{3,9,15}=3; 1→{7,13}=2; 2→{5,11}=2.
        tt |> equal(3, length(got))
        var perKey : table<int; int>
        for (item in got) {
            perKey[item.K] = item.N
        }
        tt |> equal(3, perKey[0])
        tt |> equal(2, perKey[1])
        tt |> equal(2, perKey[2])
    }
    t |> run("upstream where + named tuple reducer on Person.age") @(tt : T?) {
        let foldGot <- _fold(_common::people._where(_.age >= 25)._group_by(_.name)._select((K = _._0, N = _._1 |> length, S = _._1 |> select(@(p : _common::Person) => p.age) |> sum)))
        let refGot  <- (_common::people._where(_.age >= 25)._group_by(_.name)._select((K = _._0, N = _._1 |> length, S = _._1 |> select(@(p : _common::Person) => p.age) |> sum)))
        tt |> equal(length(refGot), length(foldGot))
        var refN, refS, foldN, foldS : table<string; int>
        for (r in refGot) {
            refN[r.K] = r.N; refS[r.K] = r.S
        }
        for (f in foldGot) {
            foldN[f.K] = f.N; foldS[f.K] = f.S
        }
        for (k in keys(refN)) {
            tt |> equal(refN?[k] ?? -1, foldN?[k] ?? -1)
            tt |> equal(refS?[k] ?? -1, foldS?[k] ?? -1)
        }
    }
    t |> run("upstream where filters out all → empty result") @(tt : T?) {
        let got <- _fold([1, 2, 3]._where(_ > 100)._group_by(_ % 3)._select(_._1 |> length))
        tt |> equal(0, length(got))
    }
    t |> run("upstream where + count terminator: number of distinct survivor keys") @(tt : T?) {
        tt |> equal(3, _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ % 2 == 0)._group_by(_ % 3)._select((K = _._0, N = _._1 |> length)).count()))
    }
}

[test]
def test_group_by_upstream_select_after_where_is_lazy(t : T?) {
    // PR-B correctness guard: a select that appears AFTER a where must execute ONLY for
    // where-survivors. Eager evaluation of all upstream selects before any where guard would
    // run the projection for filtered-out elements — wrong per reference LINQ semantics, and
    // breaks programs whose projection depends on the filter (e.g. `where(_!=0).select(10/_)`).
    t |> run("where → select with side-effect: select fires only on where-survivors") @(tt : T?) {
        g_proj_hits = 0
        let got <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._where(_ > 5)._select(projection_with_side_effect(_))._group_by(_ % 3)._select(_._1 |> length))
        // Survivors: [6,7,8,9,10] = 5 elements. Lazy select fires once per survivor.
        tt |> equal(5, g_proj_hits, "select-after-where: projection runs only on where survivors")
        var total = 0
        for (c in got) {
            total += c
        }
        tt |> equal(5, total)
    }
    t |> run("select → where → select: only the FIRST select runs unconditionally; second runs only on where-survivors") @(tt : T?) {
        g_proj_hits = 0
        // First select doubles; where filters projected > 10; second select counts only survivors.
        let got <- _fold([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]._select(_ * 2)._where(_ > 10)._select(projection_with_side_effect(_))._group_by(_ % 3)._select(_._1 |> length))
        // After *2: [2,4,6,8,10,12,14,16,18,20]; >10 keeps [12,14,16,18,20] = 5 survivors.
        tt |> equal(5, g_proj_hits, "second select runs only on where survivors")
        var total = 0
        for (c in got) {
            total += c
        }
        tt |> equal(5, total)
    }
}

[test]
def test_group_by_having_fold_parity(t : T?) {
    // PR-D: `_group_by_lazy(k) |> _having(p) |> _select(group_proj) [|> count]` — having filters
    // groups at result-build time. The predicate references _._0 (key) and/or reducer calls on _._1
    // that match an existing select-side reducer slot; everything else cascades to tier 2.
    t |> run("bare-form length-match: filter buckets by size") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let got <- _fold(nums._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)._select(_._1 |> length))
        // Buckets %3: 0=[3,6,9](3), 1=[1,4,7,10](4), 2=[2,5,8](3). Keep len>=4 → bucket 1 only.
        var total = 0
        for (c in got) {
            total += c
        }
        tt |> equal(4, total)
        var cnt = 0
        for (_x in got) {
            cnt ++
        }
        tt |> equal(1, cnt)
    }
    t |> run("named-tuple form: length-match preserves output shape") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let got <- _fold(nums._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(1, seen |> length)
        tt |> equal(4, seen[1])
    }
    t |> run("count terminator with having") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        let c = _fold(nums._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)._select((K = _._0, N = _._1 |> length)) |> count())
        tt |> equal(1, c)
    }
    t |> run("predicate references key (_._0)") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // Keep buckets where length >= key. key 0 → len 3 ≥ 0 ✓; key 1 → 4 ≥ 1 ✓; key 2 → 3 ≥ 2 ✓ (all three pass).
        let got <- _fold(nums._group_by_lazy(_ % 3)._having(_._1 |> length >= _._0)._select((K = _._0, N = _._1 |> length)))
        var cnt = 0
        for (_x in got) {
            cnt ++
        }
        tt |> equal(3, cnt)
    }
    t |> run("all buckets filtered out") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5]
        let got <- _fold(nums._group_by_lazy(_ % 5)._having(_._1 |> length > 1)._select(_._1 |> length))
        var cnt = 0
        for (_x in got) {
            cnt ++
        }
        tt |> equal(0, cnt)
    }
    t |> run("empty source") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by_lazy(_)._having(_._1 |> length > 0)._select(_._1 |> length))
        var cnt = 0
        for (_x in got) {
            cnt ++
        }
        tt |> equal(0, cnt)
    }
    t |> run("upstream where + having: both fuse") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // where >2 then group %3: 0=[3,6,9](3), 1=[4,7,10](3), 2=[5,8](2); having len>=3 → 0,1.
        let got <- _fold(nums._where(_ > 2)._group_by_lazy(_ % 3)._having(_._1 |> length >= 3)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(3, seen[1])
    }
    t |> run("inner-select-sum reducer matches in having") @(tt : T?) {
        // Both select and having reference _._1 |> select(...) |> sum on the same projection.
        let arr = [1, 2, 3, 4, 5, 6, 7, 8]
        // Buckets %3: 0=[3,6](sum=9), 1=[1,4,7](sum=12), 2=[2,5,8](sum=15). Having sum>10 → 1,2.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having(_._1 |> select($(x : int) => x * 1) |> sum > 10)._select((K = _._0, S = _._1 |> select($(x : int) => x * 1) |> sum)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.S
        }
        tt |> equal(2, seen |> length)
        tt |> equal(12, seen[1])
        tt |> equal(15, seen[2])
    }
}

[test]
def test_group_by_count_predicate_cascades(t : T?) {
    // Regression for Copilot review on PR #2775: the recognizer pops `.count()` as the bare-count terminator (→ length(tab)) — but `count(src, predicate)` is a valid linq.das overload, and popping that form silently drops the predicate. Recognizer must check `length(arguments) == 1` and otherwise cascade.
    t |> run("count(predicate) over group-by output cascades and honors the predicate") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7]
        // Buckets %3: 0=[3,6] N=2, 1=[1,4,7] N=3, 2=[2,5] N=2. Pred `_.N >= 3` → matches only bucket 1 → result 1.
        let c = _fold(nums._group_by(_ % 3)._select((K = _._0, N = _._1 |> length))._count(_.N >= 2))
        // All 3 buckets have N >= 2 → result 3.
        tt |> equal(3, c)
        let c2 = _fold(nums._group_by(_ % 3)._select((K = _._0, N = _._1 |> length))._count(_.N >= 3))
        // Only bucket 1 has N >= 3 → result 1.
        tt |> equal(1, c2)
    }
}

[test]
def test_group_by_having_unhandled_node_cascades(t : T?) {
    // PR-D safety: any unhandled node type that references `hb` (ExprAt for `_._1[0]`,
    // ExprInvoke from a non-peelable lambda, etc.) must NOT silently leak `hbName` into
    // the result-build splice — instead the planner bails and the chain cascades.
    t |> run("ExprAt on bucket (_._1[0]) cascades to tier 2") @(tt : T?) {
        let nums = [1, 2, 3, 4, 5, 6]
        // _._1[0] is ExprAt indexing into the bucket array. Without the post-rewrite scan
        // the bare `hb._1[0]` reference would survive into the splice (compile-time
        // failure at the splice site rather than a graceful cascade).
        let got <- _fold(nums._group_by_lazy(_ % 2)._having(_._1[0] == 1)._select((K = _._0, N = _._1 |> length)))
        // Bucket 0=[2,4,6] first=2≠1 fails; bucket 1=[1,3,5] first=1 passes.
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(1, seen |> length)
        tt |> equal(3, seen[1])
    }
}

[test]
def test_group_by_having_two_same_named_inner_select_diff_lambdas(t : T?) {
    // PR-E + Copilot review: when having has TWO same-named inner-select reducers with
    // DIFFERENT lambdas, naive dedup-by-name would route both predicate terms to the same
    // accumulator slot — silently wrong. Pinning correct semantics.
    t |> run("two hidden inner-select sums (different lambdas) compute distinctly") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6, 7, 8]
        // Bucket %3: 0=[3,6]: sum(x*2)=18, sum(x*3)=27, total=45.
        //            1=[1,4,7]: sum(x*2)=24, sum(x*3)=36, total=60.
        //            2=[2,5,8]: sum(x*2)=30, sum(x*3)=45, total=75.
        // Having: sum(x*2) + sum(x*3) > 50 → buckets 1 (60>50) and 2 (75>50) pass; bucket 0 (45) fails.
        let got <- _fold(arr._group_by_lazy(_ % 3)
                            ._having((_._1 |> select($(x : int) => x * 2) |> sum)
                                + (_._1 |> select($(x : int) => x * 3) |> sum) > 50)
                            ._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[1])
        tt |> equal(3, seen[2])
    }
}

[test]
def test_group_by_having_bare_form_with_hidden_cascades(t : T?) {
    // PR-E bail: bare-form group_proj combined with a having reducer that needs a hidden slot
    // falls back to tier 2 (the bare table layout uses `tuple<KeyT; AccT>` synthesized inside a
    // qmacro with embedded `typedecl(invoke(...))` for the key type, which we can't extend with
    // a dynamic count of additional acc slots). Semantics must still be correct via cascade.
    t |> run("bare-form select with hidden sum in having cascades") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6]
        // Buckets %2: 0=[2,4,6](sum=12, len=3), 1=[1,3,5](sum=9, len=3). Having sum > 10 → 0.
        let got <- _fold(arr._group_by_lazy(_ % 2)._having(_._1 |> sum > 10)._select(_._1 |> length))
        var cnt = 0
        var total = 0
        for (v in got) {
            cnt ++
            total += v
        }
        tt |> equal(1, cnt)
        tt |> equal(3, total)
    }
}

[test]
def test_reverse_count_parity(t : T?) {
    // R5: reverse |> count — counter loop, no reverse work.
    t |> run("reverse.count: identity") @(tt : T?) {
        tt |> equal(5, _fold(each([1, 2, 3, 4, 5]) |> reverse() |> count()))
    }
    t |> run("where(>2).reverse.count: counter over survivors") @(tt : T?) {
        tt |> equal(3, _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> count()))
    }
    t |> run("reverse.count on empty source: 0") @(tt : T?) {
        var empty : array<int>
        tt |> equal(0, _fold(each(empty) |> reverse() |> count()))
    }
    t |> run("where(false).reverse.count: 0 survivors") @(tt : T?) {
        tt |> equal(0, _fold(each([1, 2, 3, 4, 5])._where(_ > 999) |> reverse() |> count()))
    }
}

[test]
def test_reverse_first_parity(t : T?) {
    // R5: reverse |> first = LAST survivor in source order.
    t |> run("reverse.first: [1..5] → 5 (last element)") @(tt : T?) {
        tt |> equal(5, _fold(each([1, 2, 3, 4, 5]) |> reverse() |> first()))
    }
    t |> run("where(>2).reverse.first: 5 (last survivor)") @(tt : T?) {
        tt |> equal(5, _fold(each([1, 2, 3, 4, 5])._where(_ > 2) |> reverse() |> first()))
    }
    t |> run("select(*10).reverse.first: 50 (last projected)") @(tt : T?) {
        tt |> equal(50, _fold(each([1, 2, 3, 4, 5])._select(_ * 10) |> reverse() |> first()))
    }
    t |> run("where.select.reverse.first: 60 (last survivor projected)") @(tt : T?) {
        // survivors: [3,4,5,6]; projected*10: [30,40,50,60]; last = 60
        tt |> equal(60, _fold(each([1, 2, 3, 4, 5, 6])._where(_ > 2)._select(_ * 10) |> reverse() |> first()))
    }
}

[test]
def test_reverse_first_or_default_parity(t : T?) {
    // R5: reverse |> first_or_default — return default if no survivor, else last survivor.
    t |> run("non-empty source returns last survivor") @(tt : T?) {
        tt |> equal(5, _fold(each([1, 2, 3, 4, 5]) |> reverse() |> first_or_default(-1)))
    }
    t |> run("filter rejects all → default") @(tt : T?) {
        tt |> equal(-1, _fold(each([1, 2, 3, 4, 5])._where(_ > 999) |> reverse() |> first_or_default(-1)))
    }
    t |> run("empty source → default") @(tt : T?) {
        var empty : array<int>
        tt |> equal(42, _fold(each(empty) |> reverse() |> first_or_default(42)))
    }
}

[test]
def test_group_by_having_hidden_slot_fold_parity(t : T?) {
    // PR-E: having references a reducer not present in select — splice synthesizes a hidden
    // accumulator slot in the internal table value type; result-build projects only the
    // user-visible slots.
    t |> run("hidden sum: having sum>N, select length") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6]
        // Buckets %2: 0=[2,4,6](sum=12, len=3), 1=[1,3,5](sum=9, len=3). Having sum > 10 → 0.
        let got <- _fold(arr._group_by_lazy(_ % 2)._having(_._1 |> sum > 10)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(1, seen |> length)
        tt |> equal(3, seen[0])
    }
    t |> run("hidden min: having min<K, select length") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12, 22, 32]
        // Buckets %3: 0=[30,21,12](min=12), 1=[10,31,22](min=10), 2=[20,11,32](min=11). Having min<12 → 1,2.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having(_._1 |> min < 12)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[1])
        tt |> equal(3, seen[2])
    }
    t |> run("hidden max: having max>=N, select length") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        // Buckets %3: 0=[30,21,12](max=30), 1=[10,31](max=31), 2=[20,11](max=20). Having max>=30 → 0,1.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having(_._1 |> max >= 30)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(2, seen[1])
    }
    t |> run("hidden average: having average>F, select length") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        // Buckets %3: 0=[30,21,12](avg=21.0), 1=[10,31](avg=20.5), 2=[20,11](avg=15.5). Having avg>16 → 0,1.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having(_._1 |> average > 16.0lf)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(2, seen[1])
    }
    t |> run("hidden sum + visible length used in same predicate") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6]
        // 0=[2,4,6](sum=12, len=3), 1=[1,3,5](sum=9, len=3). Having sum>=9 && len>=3 → both pass.
        let got <- _fold(arr._group_by_lazy(_ % 2)._having((_._1 |> sum) >= 9 && (_._1 |> length) >= 3)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(3, seen[1])
    }
    t |> run("two hidden reducers: sum + min, select length") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        // 0=[30,21,12](sum=63, min=12), 1=[10,31](sum=41, min=10), 2=[20,11](sum=31, min=11). Having sum>40 && min<15 → 0,1.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having((_._1 |> sum) > 40 && (_._1 |> min) < 15)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(2, seen[1])
    }
    t |> run("hidden inner-select sum (lambda peelable)") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6, 7, 8]
        // Buckets %3: 0=[3,6] doubled=[6,12](sum=18), 1=[1,4,7] doubled=[2,8,14](sum=24), 2=[2,5,8] doubled=[4,10,16](sum=30). Having sum>20 → 1,2.
        let got <- _fold(arr._group_by_lazy(_ % 3)._having(_._1 |> select($(x : int) => x * 2) |> sum > 20)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[1])
        tt |> equal(3, seen[2])
    }
    t |> run("count terminator with hidden sum") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6]
        // 0=[2,4,6](sum=12), 1=[1,3,5](sum=9). Having sum>10 → 1 bucket.
        let c = _fold(arr._group_by_lazy(_ % 2)._having(_._1 |> sum > 10)._select((K = _._0, N = _._1 |> length)) |> count())
        tt |> equal(1, c)
    }
    t |> run("upstream where + hidden sum in having") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        // where >2 then group %3: 0=[3,6,9](sum=18), 1=[4,7,10](sum=21), 2=[5,8](sum=13). Having sum>=18 → 0,1.
        let got <- _fold(arr._where(_ > 2)._group_by_lazy(_ % 3)._having(_._1 |> sum >= 18)._select((K = _._0, N = _._1 |> length)))
        var seen : table<int; int>
        for (kv in got) {
            seen[kv.K] = kv.N
        }
        tt |> equal(2, seen |> length)
        tt |> equal(3, seen[0])
        tt |> equal(3, seen[1])
    }
    t |> run("empty source") @(tt : T?) {
        var empty : array<int>
        let got <- _fold(empty._group_by_lazy(_)._having(_._1 |> sum > 0)._select((K = _._0, N = _._1 |> length)))
        var cnt = 0
        for (_x in got) {
            cnt ++
        }
        tt |> equal(0, cnt)
    }
}

[test]
def test_top_n_mid_chain_iterator_source(t : T?) {
    // Regression — `top_n_*` registered in linqCalls but always returns array regardless of
    // input shape, so no `_to_array` variant exists. `fold_linq_default`'s first-position
    // iterator-source rewrite must skip the `_to_array` suffix for these. Hits when the user
    // hand-pipes `top_n_*(...)` mid-chain (it isn't a comprehension form, so this is the
    // only way to land here, but the path is real and the bug was a compile error.)
    t |> run("each(range) |> top_n_by |> _select — top_n_by stays unrewritten") @(tt : T?) {
        var query = _fold(each(range(0, 10)) |> top_n_by(3, @@(v : int) => -v) |> _select(_ + 100))
        // top_n_by smallest 3 by -v ⇒ largest 3 by v = [9, 8, 7]; + 100 ⇒ [109, 108, 107]
        let expected = [109, 108, 107]
        for (v, i in query, 0 .. 3) {
            tt |> equal(expected[i], v)
        }
    }
}

[test]
def test_take_while_skip_while_splice_parity(t : T?) {
    t |> run("take_while array lane") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).to_array())
        let expected = [5, 3]
        tt |> equal(2, length(got))
        for (v, i in got, 0 .. 2) {
            tt |> equal(expected[i], v)
        }
    }
    t |> run("take_while count lane") @(tt : T?) {
        let n = _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).count())
        tt |> equal(2, n)
    }
    t |> run("take_while sum (accumulator)") @(tt : T?) {
        let s = _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).sum())
        tt |> equal(8, s)
    }
    t |> run("take_while max (accumulator)") @(tt : T?) {
        let m = _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).max())
        tt |> equal(5, m)
    }
    t |> run("take_while first (early-exit)") @(tt : T?) {
        let f = _fold(each([5, 3, 8, 1, 4])._take_while(_ < 8).first())
        tt |> equal(5, f)
    }
    t |> run("take_while first_or_default with no survivor") @(tt : T?) {
        // First source element fails pred → take_while breaks before any emit → first_or_default(99).
        let f = _fold(each([10, 5, 3])._take_while(_ < 8).first_or_default(99))
        tt |> equal(99, f)
    }
    t |> run("take_while any") @(tt : T?) {
        let a = _fold(each([5, 3, 8])._take_while(_ < 8).any())
        tt |> equal(true, a)
        let b = _fold(each([10, 5, 3])._take_while(_ < 8).any())
        tt |> equal(false, b)
    }
    t |> run("take_while pred-always-true emits whole source") @(tt : T?) {
        let n = _fold(each([1, 2, 3])._take_while(_ < 100).count())
        tt |> equal(3, n)
    }
    t |> run("take_while pred-always-false emits nothing") @(tt : T?) {
        let n = _fold(each([1, 2, 3])._take_while(_ > 100).count())
        tt |> equal(0, n)
    }
    t |> run("take_while empty source") @(tt : T?) {
        var empty : array<int>
        let n = _fold(each(empty)._take_while(_ < 100).count())
        tt |> equal(0, n)
    }
    t |> run("skip_while array lane") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3).to_array())
        let expected = [3, 8, 1, 4]
        tt |> equal(4, length(got))
        for (v, i in got, 0 .. 4) {
            tt |> equal(expected[i], v)
        }
    }
    t |> run("skip_while count lane") @(tt : T?) {
        let n = _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3).count())
        tt |> equal(4, n)
    }
    t |> run("skip_while min (accumulator)") @(tt : T?) {
        let m = _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3).min())
        tt |> equal(1, m)
    }
    t |> run("skip_while pred-always-true skips whole source") @(tt : T?) {
        let n = _fold(each([1, 2, 3])._skip_while(_ < 100).count())
        tt |> equal(0, n)
    }
    t |> run("skip_while pred-always-false emits everything") @(tt : T?) {
        let n = _fold(each([1, 2, 3])._skip_while(_ > 100).count())
        tt |> equal(3, n)
    }
    t |> run("skip_while empty source") @(tt : T?) {
        var empty : array<int>
        let n = _fold(each(empty)._skip_while(_ < 100).count())
        tt |> equal(0, n)
    }
    t |> run("skip_while + take_while compose") @(tt : T?) {
        // Skip leading >3 (skip 5), then take while <8 (emit 3, break on 8).
        let got <- _fold(each([5, 3, 8, 1, 4])._skip_while(_ > 3)._take_while(_ < 8).to_array())
        tt |> equal(1, length(got))
        tt |> equal(3, got[0])
    }
    t |> run("where + take_while compose") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1, 4])._where(_ != 3)._take_while(_ < 8).to_array())
        tt |> equal(1, length(got))
        tt |> equal(5, got[0])
    }
    t |> run("skip + take_while compose") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1, 4]).skip(1)._take_while(_ < 8).to_array())
        tt |> equal(1, length(got))
        tt |> equal(3, got[0])
    }
    t |> run("take_while + take compose — take_while breaks first") @(tt : T?) {
        // Source: [5, 3, 1, 4, 8]; take_while<8 keeps [5, 3, 1, 4]; take(2) limits to [5, 3].
        let got <- _fold(each([5, 3, 1, 4, 8])._take_while(_ < 8).take(2).to_array())
        tt |> equal(2, length(got))
        tt |> equal(5, got[0])
        tt |> equal(3, got[1])
    }
    t |> run("take_while + take compose — take(N) hits first") @(tt : T?) {
        // Source: [5, 3, 1, 4, 8]; take_while<8 keeps [5, 3, 1, 4]; take(10) is wider → output = take_while output.
        let got <- _fold(each([5, 3, 1, 4, 8])._take_while(_ < 8).take(10).to_array())
        tt |> equal(4, length(got))
    }
    t |> run("skip_while + skip compose") @(tt : T?) {
        // Source: [5, 5, 3, 1, 4]; skip_while>3 skips [5, 5] then emits [3, 1, 4]; skip(1) drops 3; output [1, 4].
        // Canonical order is skip then skip_while, so this writes skip BEFORE skip_while in the chain.
        let got <- _fold(each([5, 5, 3, 1, 4]).skip(1)._skip_while(_ > 3).to_array())
        // skip(1) drops first 5 → [5, 3, 1, 4]; skip_while>3 skips [5] then emits [3, 1, 4].
        tt |> equal(3, length(got))
        tt |> equal(3, got[0])
        tt |> equal(1, got[1])
        tt |> equal(4, got[2])
    }
}

[test]
def test_take_while_skip_while_cascade_bails(t : T?) {
    // Cascades to tier 2 (still semantically correct, just no splice). These confirm the
    // bail conditions don't break observable behavior.
    t |> run("select before take_while cascades — values still match reference") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1])._select(_ * 2)._take_while(_ < 16).to_array())
        let expected = [10, 6]
        tt |> equal(2, length(got))
        for (v, i in got, 0 .. 2) {
            tt |> equal(expected[i], v)
        }
    }
    t |> run("select before skip_while cascades — values still match reference") @(tt : T?) {
        let got <- _fold(each([5, 3, 8, 1])._select(_ * 2)._skip_while(_ > 6).to_array())
        let expected = [6, 16, 2]
        tt |> equal(3, length(got))
        for (v, i in got, 0 .. 3) {
            tt |> equal(expected[i], v)
        }
    }
    t |> run("multiple take_while cascades — only first one spliced semantics") @(tt : T?) {
        // Reference would AND the two breaks; splice rejects (multiple take_while). Values:
        // take_while<8 keeps [5, 3]; take_while<4 of those keeps [] (3 is the first to fail).
        // Actually [5,3]: 5<4 false → break → [].
        let got <- _fold(each([5, 3, 1, 8])._take_while(_ < 8)._take_while(_ < 4).to_array())
        tt |> equal(0, length(got))
    }
}

[test]
def test_fold_order_by_first(t : T?) {
    t |> run("order_by + first → min_by") @(tt : T?) {
        let v = _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by(_).first())
        tt |> equal(2, v, "minimum element after order_by")
    }
    t |> run("order_by_descending + first → max_by") @(tt : T?) {
        let v = _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._order_by_descending(_).first())
        tt |> equal(30, v, "maximum element after order_by_descending")
    }
    t |> run("where + order_by + first → min on prefilter buf") @(tt : T?) {
        let v = _fold(each([10, 20, 5, 8, 30, 15, 2, 25])._where(_ > 5)._order_by(_).first())
        tt |> equal(8, v, "min of (10,20,8,30,15,25) = 8")
    }
    t |> run("order_by + first_or_default — empty source returns default") @(tt : T?) {
        let empty : array<int>
        let v = _fold(each(empty)._order_by(_).first_or_default(-1))
        tt |> equal(-1, v, "empty source → default")
    }
    t |> run("order_by + first_or_default — non-empty returns min") @(tt : T?) {
        let v = _fold(each([10, 5, 20])._order_by(_).first_or_default(-1))
        tt |> equal(5, v, "non-empty source → min element")
    }
    t |> run("where + order_by + first_or_default — empty after filter") @(tt : T?) {
        let v = _fold(each([1, 2, 3])._where(_ > 100)._order_by(_).first_or_default(-1))
        tt |> equal(-1, v, "filter excludes all → default")
    }
    t |> run("order_by + first on empty array panics") @(tt : T?) {
        let empty : array<int>
        var didPanic = false
        try {
            let v = _fold(each(empty)._order_by(_).first())
            tt |> success(false, "expected panic; got {v}")
        } recover {
            didPanic = true
        }
        tt |> success(didPanic, "first() on empty array panicked")
    }
    t |> run("where + order_by + first filtered-empty panics") @(tt : T?) {
        var didPanic = false
        try {
            let v = _fold(each([1, 2, 3])._where(_ > 100)._order_by(_).first())
            tt |> success(false, "expected panic; got {v}")
        } recover {
            didPanic = true
        }
        tt |> success(didPanic, "first() on filtered-empty array panicked")
    }
    // ── order(arr, cmp) + first — splice MUST honor the custom comparator (was emitting bare min/max, ignoring cmp)
    t |> run("order(arr, cmp) + first preserves custom comparator (descending cmp → max)") @(tt : T?) {
        let v = _fold(each([10, 5, 20, 3]) |> order($(a : int, b : int) => a > b) |> first())
        tt |> equal(20, v, "cmp orders descending → first returns max")
    }
    t |> run("order_descending(arr, cmp) + first preserves custom comparator") @(tt : T?) {
        // order_descending swaps cmp args, so cmp `a > b` becomes effectively `b > a` (ascending) → first → min
        let v = _fold(each([10, 5, 20, 3]) |> order_descending($(a : int, b : int) => a > b) |> first())
        tt |> equal(3, v, "order_descending negates cmp → first is min")
    }
    t |> run("order(arr, cmp) + take preserves custom comparator (pre-existing recognizer hole)") @(tt : T?) {
        // cmp orders descending → take(2) returns top 2
        let result <- _fold(each([10, 5, 20, 3, 25, 8]) |> order($(a : int, b : int) => a > b) |> take(2) |> to_array())
        tt |> equal(2, length(result), "take 2 of descending order")
        tt |> equal(25, result[0], "first is max (25)")
        tt |> equal(20, result[1], "second is next-max (20)")
    }
}
