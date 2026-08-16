options gen2
require dastest/testing_boost public

struct Foo {
    x : int = 7
    y : int = 3
}

[test]
def test_resize_inits(t : T?) {
    t |> run("resize constructs new elements") @(t : T?) {
        var inscope arr : array<Foo>
        arr |> resize(3)
        t |> equal(arr[0].x, 7)
        t |> equal(arr[2].x, 7)
        t |> equal(arr[2].y, 3)
    }
    t |> run("resize int64 constructs too") @(t : T?) {
        var inscope arr : array<Foo>
        arr |> resize(int64(3))
        t |> equal(arr[2].x, 7)
    }
    t |> run("shrink then grow re-initializes the regrown range") @(t : T?) {
        var inscope arr : array<Foo>
        arr |> resize(5)
        arr[4].x = 99
        arr |> resize(2)
        arr |> resize(6)
        t |> equal(arr[4].x, 7)
        t |> equal(arr[5].x, 7)
    }
    t |> run("trivial elements still zero") @(t : T?) {
        var inscope arr : array<int>
        arr |> resize(4)
        t |> equal(arr[3], 0)
    }
}

[test]
def test_resize_and_init(t : T?) {
    t |> run("trivial type zeroes without a loop") @(t : T?) {
        var inscope arr : array<int>
        arr |> resize_and_init(3)
        t |> equal(arr[2], 0)
    }
    t |> run("initValue overload") @(t : T?) {
        var inscope arr : array<int>
        arr |> resize_and_init(3, 42)
        t |> equal(arr[0], 42)
        t |> equal(arr[2], 42)
    }
    t |> run("init-carrying type") @(t : T?) {
        var inscope arr : array<Foo>
        arr |> resize_and_init(2)
        t |> equal(arr[1].x, 7)
    }
}
