// Test: async macros under infer_time_folding = false — the lint/LSP/MCP compile policy.
// The await yield and assign steps branch at MACRO time, so no dead arm survives to
// type-check: a non-copyable awaited payload compiles (the copy arm would reject it),
// and a non-void routine's wait yield compiles (the bool arm would reject it).

options gen2
options infer_time_folding = false

require dastest/testing_boost public
require daslib/async_boost

[async]
def make_arr(n : int) : array<int> {
    await_next_frame()
    var a : array<int>
    for (i in range(n)) {
        a |> push(i * 10)
    }
    yield <- a
}

[async]
def add_one(x : int) : int {
    await_next_frame()
    yield x + 1
}

[async]
def consume_let() : int {
    let a <- await <| make_arr(3)
    yield a[0] + a[1] + a[2]
}

[async]
def consume_assign() : int {
    var a : array<int>
    a <- await <| make_arr(2)
    yield a[0] + a[1]
}

[async]
def consume_copy(x : int) : int {
    var v = 0
    v = await <| add_one(x)
    yield v * 2
}

def drain(var it : iterator<variant<res : int; wait : bool>>) : int {
    var last = 0
    for (step in it) {
        if (step is res) {
            last = step as res
        }
    }
    return last
}

[test]
def test_async_no_folding(t : T?) {
    t |> run("non-copyable await via let <-") @(t : T?) {
        var it <- consume_let()
        t |> equal(drain(it), 30)
    }
    t |> run("non-copyable await via assignment <-") @(t : T?) {
        var it <- consume_assign()
        t |> equal(drain(it), 10)
    }
    t |> run("non-void copy await") @(t : T?) {
        var it <- consume_copy(5)
        t |> equal(drain(it), 12)
    }
}
