options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq

require _common

[test]
def test_elementat(t : T?) {
    t |> run("basic element at") @(t : T?) {
        var query = element_at(
            [iterator for(x in 0..5); x],
            2
        )
        t |> equal(query, 2)
    }
    t |> run("complex element at") @(t : T?) {
        var qcomplex = element_at(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            3
        )
        t |> equal(qcomplex.a[0], 3)
        t |> equal(qcomplex.a[1], 30)
    }
    t |> run("from array element at") @(t : T?) {
        var qarray = element_at(
            [1, 2, 3, 4, 5],
            1
        )
        t |> equal(qarray, 2)
    }
}

[test]
def test_elementatordefault(t : T?) {
    t |> run("basic element at or default") @(t : T?) {
        var query = element_at_or_default(
            [iterator for(x in 0..5); x],
            2
        )
        t |> equal(query, 2)
    }
    t |> run("complex element at or default") @(t : T?) {
        var qcomplex = element_at_or_default(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            3
        )
        t |> equal(qcomplex.a[0], 3)
        t |> equal(qcomplex.a[1], 30)
    }
    t |> run("from array element at or default") @(t : T?) {
        var qarray = element_at_or_default(
            [1, 2, 3, 4, 5],
            1
        )
        t |> equal(qarray, 2)
    }
}

[test]
def test_first(t : T?) {
    t |> run("basic first") @(t : T?) {
        var query = first(
            [iterator for(x in 0..5); x]
        )
        t |> equal(query, 0)
    }
    t |> run("complex first") @(t : T?) {
        var qcomplex = first(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> equal(qcomplex.a[0], 0)
        t |> equal(qcomplex.a[1], 0)
    }
    t |> run("from array first") @(t : T?) {
        var qarray = first(
            [1, 2, 3, 4, 5]
        )
        t |> equal(qarray, 1)
    }
}

[test]
def test_firstordefault(t : T?) {
    t |> run("basic first or default") @(t : T?) {
        var query = first_or_default(
            [iterator for(x in 0..5); x],
            -1
        )
        t |> equal(query, 0)
    }
    t |> run("complex first or default") @(t : T?) {
        var qcomplex = first_or_default(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [0, 0])
        )
        t |> equal(qcomplex.a[0], 0)
        t |> equal(qcomplex.a[1], 0)
    }
    t |> run("default first or default") @(t : T?) {
        var qdefault = first_or_default(
            [iterator for(x in 0..0); x],
            -1
        )
        t |> equal(qdefault, -1)
    }
    t |> run("complex first or default") @(t : T?) {
        var qcomplex = first_or_default(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [0, 0])
        )
        t |> equal(qcomplex.a[0], 0)
        t |> equal(qcomplex.a[1], 0)
    }
    t |> run("from array first or default") @(t : T?) {
        var qarray = first_or_default(
            [1, 2, 3, 4, 5],
            -1
        )
        t |> equal(qarray, 1)
    }
}

[test]
def test_last(t : T?) {
    t |> run("basic last") @(t : T?) {
        var query = last(
            [iterator for(x in 0..5); x]
        )
        t |> equal(query, 4)
    }
    t |> run("complex last") @(t : T?) {
        var qcomplex = last(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        t |> equal(qcomplex.a[0], 4)
        t |> equal(qcomplex.a[1], 40)
    }
    t |> run("from array last") @(t : T?) {
        var qarray = last(
            [1, 2, 3, 4, 5]
        )
        t |> equal(qarray, 5)
    }
}

[test]
def test_lastordefault(t : T?) {
    t |> run("basic last or default") @(t : T?) {
        var query = last_or_default(
            [iterator for(x in 0..5); x],
            -1
        )
        t |> equal(query, 4)
    }
    t |> run("default last or default") @(t : T?) {
        var qdefault = last_or_default(
            [iterator for(x in 0..0); x],
            -1
        )
        t |> equal(qdefault, -1)
    }
    t |> run("complex last or default") @(t : T?) {
        var qcomplex = last_or_default(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            ComplexType(a = [0, 0])
        )
        t |> equal(qcomplex.a[0], 4)
        t |> equal(qcomplex.a[1], 40)
    }
    t |> run("from array last or default") @(t : T?) {
        var qarray = last_or_default(
            [1, 2, 3, 4, 5],
            -1
        )
        t |> equal(qarray, 5)
    }
}

[test]
def test_single(t : T?) {
    t |> run("basic single") @(t : T?) {
        var query = single(
            [iterator for(x in 0..1); x]
        )
        t |> equal(query, 0)
    }
    t |> run("basic single not") @(t : T?) {
        var qone = single(
            [iterator for(x in 1..2); x]
        )
        t |> equal(qone, 1)
    }
    t |> run("complex single") @(t : T?) {
        var qcomplex = single(
            [iterator for(x in 0..1); ComplexType(a = [x, x * 10])]
        )
        t |> equal(qcomplex.a[0], 0)
        t |> equal(qcomplex.a[1], 0)
    }
    t |> run("from array single") @(t : T?) {
        var qarray = single(
            [1]
        )
        t |> equal(qarray, 1)
    }
}

[test]
def test_singleordefault(t : T?) {
    t |> run("basic single or default") @(t : T?) {
        var query = single_or_default(
            [iterator for(x in 0..5); x],
            -1
        )
        t |> equal(query, -1)
    }
    t |> run("basic single or default not") @(t : T?) {
        var qone = single_or_default(
            [iterator for(x in 0..1); x],
            -1
        )
        t |> equal(qone, 0)
    }
    t |> run("complex single or default not") @(t : T?) {
        var qcomplex = single_or_default(
            [iterator for(x in 2..3); ComplexType(a = [x, x * 10])],
            ComplexType(a = [-1, -1])
        )
        t |> equal(qcomplex.a[0], 2)
        t |> equal(qcomplex.a[1], 20)
    }
    t |> run("from array single or default not") @(t : T?) {
        var qarray = single_or_default(
            [1],
            -1
        )
        t |> equal(qarray, 1)
    }
}

