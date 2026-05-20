options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_any(t : T?) {
    t |> run("basic any") @(t : T?) {
        let query = any(
            [iterator for(x in 0..5); x]
        )
        t |> success(query)
    }
    t |> run("complex any") @(t : T?) {
        let qcomplex = any(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(qcomplex)
    }
    t |> run("empty any") @(t : T?) {
        let qempty = any(
            empty(type<int>)
        )
        t |> success(!qempty)
    }
    t |> run("any from array") @(t : T?) {
        let arr = [0, 1, 2, 3, 4]
        let qarray = any(arr)
        t |> success(qarray)
    }
    t |> run("any with predicate") @(t : T?) {
        let qpred = _any(
            [iterator for(x in 0..5); x],
            _ > 3
        )
        t |> success(qpred)
        let qpred2 = _any(
            [iterator for(x in 0..5); x],
            _ > 10
        )
        t |> success(!qpred2)
    }
}

[test]
def test_all(t : T?) {
    t |> run("basic all") @(t : T?) {
        let query = _all(
            [iterator for(x in 0..5); x],
            _ < 5
        )
        t |> success(query)
    }
    t |> run("complex all") @(t : T?) {
        let qcomplex = _all(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0] < 5
        )
        t |> success(qcomplex)
    }
    t |> run("empty all") @(t : T?) {
        let qempty = _all(
            empty(type<int>),
            _ < 5
        )
        t |> success(qempty)
    }
    t |> run("all from array") @(t : T?) {
        let arr = [0, 1, 2, 3, 4]
        let qarray = _all(
            arr,
            _ < 5
        )
        t |> success(qarray)
    }
}

[test]
def test_none(t : T?) {
    t |> run("basic none") @(t : T?) {
        let query = none(
            [iterator for(x in 0..5); x]
        )
        t |> success(!query)
    }
    t |> run("complex none") @(t : T?) {
        let qcomplex = none(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> success(!qcomplex)
    }
    t |> run("empty none") @(t : T?) {
        let qempty = none(
            empty(type<int>)
        )
        t |> success(qempty)
    }
    t |> run("none from array") @(t : T?) {
        let arr = [0, 1, 2, 3, 4]
        let qarray = none(arr)
        t |> success(!qarray)
    }
    t |> run("none from empty array") @(t : T?) {
        let emptyArr : array<int>
        let qemptyArr = none(emptyArr)
        t |> success(qemptyArr)
    }
    t |> run("none with predicate — match") @(t : T?) {
        let qpred = none(
            [iterator for(x in 0..5); x],
            $(x : int) => x > 10
        )
        t |> success(qpred)
    }
    t |> run("none with predicate — no match") @(t : T?) {
        let qpred = none(
            [iterator for(x in 0..5); x],
            $(x : int) => x > 3
        )
        t |> success(!qpred)
    }
    t |> run("none with predicate on array") @(t : T?) {
        let arr = [1, 2, 3, 4, 5]
        t |> success(none(arr, $(x : int) => x < 0))
        t |> success(!none(arr, $(x : int) => x > 2))
    }
    t |> run("_none call-macro — no match") @(t : T?) {
        let arr = [1, 2, 3, 4, 5]
        t |> success(_none(arr, _ < 0))
    }
    t |> run("_none call-macro — match") @(t : T?) {
        let arr = [1, 2, 3, 4, 5]
        t |> success(!_none(arr, _ > 2))
    }
    t |> run("_none call-macro on iterator") @(t : T?) {
        let q = _none(
            [iterator for(x in 0..5); x],
            _ > 10
        )
        t |> success(q)
    }
    t |> run("_none call-macro on empty") @(t : T?) {
        t |> success(_none(empty(type<int>), _ == 0))
    }
}

[test]
def test_in_not_in(t : T?) {
    t |> run("_in on array — present") @(t : T?) {
        let haystack = [1, 2, 3, 4, 5]
        t |> success(_in(3, haystack))
    }
    t |> run("_in on array — absent") @(t : T?) {
        let haystack = [1, 2, 3, 4, 5]
        t |> success(!_in(42, haystack))
    }
    t |> run("_in on iterator — present") @(t : T?) {
        t |> success(_in(2, [iterator for(x in 0..5); x]))
    }
    t |> run("_in on iterator — absent") @(t : T?) {
        t |> success(!_in(99, [iterator for(x in 0..5); x]))
    }
    t |> run("_not_in on array — absent") @(t : T?) {
        let haystack = [1, 2, 3, 4, 5]
        t |> success(_not_in(42, haystack))
    }
    t |> run("_not_in on array — present") @(t : T?) {
        let haystack = [1, 2, 3, 4, 5]
        t |> success(!_not_in(3, haystack))
    }
    t |> run("_in on empty iterator") @(t : T?) {
        t |> success(!_in(1, empty(type<int>)))
    }
    t |> run("_not_in on empty iterator") @(t : T?) {
        t |> success(_not_in(1, empty(type<int>)))
    }
}

[test]
def test_contains(t : T?) {
    t |> run("basic contains") @(t : T?) {
        let query = contains(
            [iterator for(x in 0..5); x],
            3
        )
        t |> success(query)
    }
    t |> run("complex contains") @(t : T?) {
        let qcomplex = contains(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [3, 30])
        )
        t |> success(qcomplex)
    }
}

