options gen2
require daslib/linq
require dastest/testing_boost public
require math
require strings

require daslib/linq_boost

require _common

[test]
def test_count(t : T?) {
    t |> run("basic count") @(t : T?) {
        var total = count(
            [iterator for(x in 0..5); x]
        )
        t |> equal(total, 5) // its 5, because the range is 0..5 and range iteration goes up to but not including
    }
    t |> run("long count") @(t : T?) {
        var ltotal = long_count(
            [iterator for(x in 0..5); x]
        )
        t |> equal(ltotal, 5l)
    }
    t |> run("complex long count") @(t : T?) {
        var totalComplex = long_count(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> equal(totalComplex, 5l)
    }
    t |> run("count from array") @(t : T?) {
        var arr = [for (x in 0..5); x]
        var arrayTotal = count(arr)
        t |> equal(arrayTotal, 5)
    }
    t |> run("long count from array") @(t : T?) {
        var arr = [for (x in 0..5); x]
        var arrayLongTotal = long_count(arr)
        t |> equal(arrayLongTotal, 5l)
    }
    t |> run("count with predicate from iterator") @(t : T?) {
        var total = count(
            [iterator for(x in 0..10); x],
            $(v) => v > 5
        )
        t |> equal(total, 4)  // 6,7,8,9
    }
    t |> run("count with predicate from array") @(t : T?) {
        var arr = [for (x in 0..10); x]
        var total = count(arr, $(v) => v % 2 == 0)
        t |> equal(total, 5)  // 0,2,4,6,8
    }
    t |> run("count with _count shorthand") @(t : T?) {
        var total = _count(
            [iterator for(x in 0..10); x],
            _ > 5
        )
        t |> equal(total, 4)
    }
    t |> run("count predicate none match") @(t : T?) {
        var arr = [1, 2, 3]
        var total = count(arr, $(v) => v > 10)
        t |> equal(total, 0)
    }
    t |> run("count predicate all match") @(t : T?) {
        var arr = [1, 2, 3]
        var total = count(arr, $(v) => v > 0)
        t |> equal(total, 3)
    }
}

[test]
def test_min(t : T?) {
    t |> run("basic min") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = min(
            numbers.to_sequence()
        )
        t |> equal(query_n, 1)
    }
    t |> run("complex min by") @(t : T?) {
        var query = _min_by(
            people.to_sequence(),
            (_.name, _.age)
        )
        t |> equal(query.name, "Alice")
        t |> equal(query.age, 25)
    }
    t |> run("complex min by") @(t : T?) {
        var rquery = _min_by(
            people.to_sequence(),
            (_.age, _.name)
        )
        t |> equal(rquery.name, "Bob")
        t |> equal(rquery.age, 20)
    }
    t |> run("complex min by") @(t : T?) {
        var qcomplex = _min_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        t |> success(qcomplex.a.Equal([0, 0]))
    }
    t |> run("min from array of strings") @(t : T?) {
        var strings = ["banana", "apple", "cherry", "date"]
        var minString = min(strings)
        t |> equal(minString, "apple")
    }
    t |> run("min by from array of strings") @(t : T?) {
        var strings = ["banana", "apple", "cherry", "date", "late"]
        var minString = _min_by(strings, (length(_), _))
        t |> equal(minString, "date")
    }
}

[test]
def test_max(t : T?) {
    t |> run("basic max") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var query_n = max(
            numbers.to_sequence()
        )
        t |> equal(query_n, 8)
    }
    t |> run("complex max by") @(t : T?) {
        var query = _max_by(
            people.to_sequence(),
            (_.name, _.age)
        )
        t |> equal(query.name, "Charlie")
        t |> equal(query.age, 35)
    }
    t |> run("complex max by") @(t : T?) {
        var rquery = _max_by(
            people.to_sequence(),
            (_.age, _.name)
        )
        t |> equal(rquery.name, "Charlie")
        t |> equal(rquery.age, 35)
    }
    t |> run("complex max by") @(t : T?) {
        var qcomplex = _max_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        t |> success(qcomplex.a.Equal([4, 40]))
    }
    t |> run("max from array of strings") @(t : T?) {
        var strings = ["banana", "apple", "cherry", "date"]
        var maxString = max(strings.to_sequence())
        t |> equal(maxString, "date")
    }
    t |> run("max by from array of strings") @(t : T?) {
        var strings = ["banana", "apple", "cherry", "date", "late"]
        var maxString = _max_by(strings, (length(_), _))
        t |> equal(maxString, "cherry")
    }
}

[test]
def test_min_max(t : T?) {
    t |> run("basic min_max") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var (minVal, maxVal) = min_max(
            numbers.to_sequence()
        )
        t |> equal(minVal, 1)
        t |> equal(maxVal, 8)
    }
    t |> run("complex min_max by") @(t : T?) {
        var (minPerson, maxPerson) = _min_max_by(
            people.to_sequence(),
            (_.age, _.name)
        )
        t |> equal(minPerson.name, "Bob")
        t |> equal(minPerson.age, 20)
        t |> equal(maxPerson.name, "Charlie")
        t |> equal(maxPerson.age, 35)
    }
    t |> run("complex min_max by") @(t : T?) {
        var (minComplex, maxComplex) = _min_max_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        t |> success(minComplex.a.Equal([0, 0]))
        t |> success(maxComplex.a.Equal([4, 40]))
    }
    t |> run("min_max from array of strings") @(t : T?) {
        var strings = ["banana", "apple", "cherry", "date"]
        var (minString, maxString) = min_max(strings.to_sequence())
        t |> equal(minString, "apple")
        t |> equal(maxString, "date")
    }
}

[test]
def test_min_max_average(t : T?) {
    t |> run("basic min_max_average") @(t : T?) {
        var numbers = [
            5, 3, 8, 1, 4
        ]
        var (minVal, maxVal, avgVal) = min_max_average(
            numbers.to_sequence()
        )
        t |> equal(minVal, 1)
        t |> equal(maxVal, 8)
        t |> equal(avgVal, 4)
    }
    t |> run("complex min_max_average by") @(t : T?) {
        var (minComplex, maxComplex, avgComplex) = _min_max_average_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        t |> success(minComplex.a.Equal([0, 0]))
        t |> success(maxComplex.a.Equal([4, 40]))
        t |> success(avgComplex.a.Equal([2, 20]))
    }
}

[test]
def test_sum(t : T?) {
    t |> run("basic sum") @(t : T?) {
        var query = sum(
            [iterator for(x in 0..5); x]
        )
        t |> equal(query, 10)
    }
    t |> run("complex sum") @(t : T?) {
        var qcomplex = sum(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(qcomplex.a.Equal([10, 100]))
    }
    t |> run("float sum from array") @(t : T?) {
        var queryf = sum(
            [for (x in 0..5); float(x)]
        )
        t |> equal(queryf, 10.0)
    }
}

[test]
def test_average(t : T?) {
    t |> run("basic average") @(t : T?) {
        var query = average(
            [iterator for(x in 0..5); x]
        )
        t |> equal(query, 2)
    }
    t |> run("complex average") @(t : T?) {
        var queryf = average(
            [iterator for(x in 0..5); float(x)]
        )
        t |> equal(queryf, 2.0)
    }
    t |> run("complex average") @(t : T?) {
        var qcomplex = average(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(qcomplex.a.Equal([2, 20]))
    }
    t |> run("average from array of floats") @(t : T?) {
        var floats = [1.0, 2.0, 3.0, 4.0, 5.0]
        var avgFloat = average(floats.to_sequence())
        t |> equal(avgFloat, 3.0)
    }
}

[test]
def test_aggregate(t : T?) {
    t |> run("basic aggregate") @(t : T?) {
        var query = aggregate(
            [iterator for(x in 0..5); x],
            0,
            $(acc, x) => acc + x
        )
        t |> equal(query, 10)
    }
    t |> run("complex aggregate") @(t : T?) {
        var qcomplex = aggregate(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [0, 0]),
            $(acc, x) => ComplexType(a = [acc.a[0] + x.a[0], acc.a[1] + x.a[1]])
        )
        t |> equal(qcomplex.a[0], 10)
        t |> equal(qcomplex.a[1], 100)
    }
    t |> run("string aggregate") @(t : T?) {
        var strAgg = aggregate(
            ["hello", " ", "world"],
            "",
            $(acc, x) => acc + x
        )
        t |> equal(strAgg, "hello world")
    }
    t |> run("type change aggregate") @(t : T?) {
        var query = aggregate(
            [for (x in 0..5); x],
            "",
            $(acc, x) => "{acc}{x}"
        )
        t |> equal(query, "01234")
    }
}
