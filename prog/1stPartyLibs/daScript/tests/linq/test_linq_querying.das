options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_any(t : T?) {
    t |> run("basic any") @(t : T?) {
        var query = any(
            [iterator for(x in 0..5); x]
        )
        t |> success(query)
    }
    t |> run("complex any") @(t : T?) {
        var qcomplex = any(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(qcomplex)
    }
    t |> run("empty any") @(t : T?) {
        var qempty = any(
            empty(type<int>)
        )
        t |> success(!qempty)
    }
    t |> run("any from array") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        var qarray = any(arr)
        t |> success(qarray)
    }
    t |> run("any with predicate") @(t : T?) {
        var qpred = _any(
            [iterator for(x in 0..5); x],
            _ > 3
        )
        t |> success(qpred)
        var qpred2 = _any(
            [iterator for(x in 0..5); x],
            _ > 10
        )
        t |> success(!qpred2)
    }
}

[test]
def test_all(t : T?) {
    t |> run("basic all") @(t : T?) {
        var query = _all(
            [iterator for(x in 0..5); x],
            _ < 5
        )
        t |> success(query)
    }
    t |> run("complex all") @(t : T?) {
        var qcomplex = _all(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0] < 5
        )
        t |> success(qcomplex)
    }
    t |> run("empty all") @(t : T?) {
        var qempty = _all(
            empty(type<int>),
            _ < 5
        )
        t |> success(qempty)
    }
    t |> run("all from array") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        var qarray = _all(
            arr,
            _ < 5
        )
        t |> success(qarray)
    }
}

[test]
def test_contains(t : T?) {
    t |> run("basic contains") @(t : T?) {
        var query = contains(
            [iterator for(x in 0..5); x],
            3
        )
        t |> success(query)
    }
    t |> run("complex contains") @(t : T?) {
        var qcomplex = contains(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [3, 30])
        )
        t |> success(qcomplex)
    }
}

