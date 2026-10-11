// Bracket/ctor literal lowering must construct non-trivial elements identically in every tier.
options gen2
require dastest/testing_boost public
require daslib/coroutines

var g_a_steps = 0
var g_b_steps = 0

[coroutine]
def step_co_a() {   // nolint:STYLE038 length is the [coroutine] expansion, not the source body
    g_a_steps ++
    co_continue()
    g_a_steps ++
}

[coroutine]
def step_co_b() {   // nolint:STYLE038 length is the [coroutine] expansion, not the source body
    g_b_steps ++
    co_continue()
    g_b_steps ++
    co_continue()
    g_b_steps ++
}

def make_iota() : int[4] {
    var r : int[4]
    for (i in range(4)) {
        r[i] = i + 1
    }
    return r
}

struct IntsPair {
    comprehended : array<int>
    moved : array<int>
}

variant IntsOrTag {
    ints : array<int>
    tag : int
}

[test]
def test_literal_iterator_elements(t : T?) {
    t |> run("moved-in iterator elements arrive live") @(t : T?) {
        g_a_steps = 0
        g_b_steps = 0
        var tasks <- [ <- step_co_a(), <- step_co_b()]
        cr_run_all(tasks)
        t |> equal(g_a_steps, 2)
        t |> equal(g_b_steps, 3)
    }
}

[test]
def test_literal_fixed_array_elements(t : T?) {
    t |> run("fixed-array elements carry their contents") @(t : T?) {
        var arr <- [make_iota(), make_iota()]
        var total = 0
        for (fa in arr) {
            for (x in fa) {
                total += x
            }
        }
        t |> equal(total, 20)
        delete arr
    }
}

[test]
def test_ctor_comprehension_move_init(t : T?) {
    t |> run("a comprehension moved into a constructor argument keeps its elements") @(t : T?) {
        var src <- [1, 2, 3]
        var other <- [7, 8]
        var pair = IntsPair(comprehended <- [for (x in src); x * 10], moved <- other)
        t |> equal(length(pair.comprehended), 3)
        t |> equal(pair.comprehended[0], 10)
        t |> equal(pair.comprehended[2], 30)
        t |> equal(length(pair.moved), 2)
        t |> equal(pair.moved[1], 8)
        delete pair
        delete src
    }
}

[test]
def test_comprehension_literal_elements(t : T?) {
    t |> run("comprehension elements land intact in an array literal") @(t : T?) {
        var src <- [1, 2, 3]
        var arr <- [ [for(x in src); x * 10], [for (x in src); x * 100]]
        t |> equal(length(arr), 2)
        t |> equal(length(arr[0]), 3)
        t |> equal(length(arr[1]), 3)
        t |> equal(arr[0][2], 30)
        t |> equal(arr[1][2], 300)
        delete arr
        delete src
    }
    t |> run("a comprehension element lands intact in a tuple literal") @(t : T?) {
        var src <- [1, 2, 3]
        var tup <- ([for (x in src); x * 10], 7)
        t |> equal(length(tup._0), 3)
        t |> equal(tup._0[1], 20)
        t |> equal(tup._1, 7)
        delete tup
        delete src
    }
    t |> run("a comprehension element lands intact in a variant literal") @(t : T?) {
        var src <- [1, 2, 3]
        var v <- IntsOrTag(ints <- [for (x in src); x * 10])
        t |> success(v is ints, "the variant holds the ints alternative")
        t |> equal(length(v as ints), 3)
        t |> equal((v as ints)[0], 10)
        delete v
        delete src
    }
}
