// Tests finally blocks: for-loop, while-loop, if-break, function, block
options gen2
require dastest/testing_boost public

var ONE = 1

[sideeffects]
def one(tt : T?) {
    ONE = 1
    tt |> equal(ONE, 1)
    return ONE
}

[sideeffects]
def test_if_finally(tt : T?) {
    var t : int[10]
    var count = 0
    for (x in range(10)) {
        t[x] = x + 1
        if (x == 5) {
            break
        } finally {
            t[9] = 13
            count ++
        }
    }
    tt |> equal(count, 1)
    tt |> equal(t[9], 13)
}

[sideeffects]
def test_for_finally(tt : T?) {
    var t : int[10]
    var count = 0
    for (x in range(10)) {
        let uno = one(tt)
        t[x] = x + uno
        if (x == 5) {
            break
        }
    } finally {
        t[9] = 13
        count += uno
    }
    tt |> equal(count, 1)
    tt |> equal(t[9], 13)
}

[sideeffects]
def test_while_finally(tt : T?) {
    var t : int[10]
    var count = 0
    var x = 0
    while (x != 10) {
        let uno = one(tt)
        t[x] = x + uno
        if (x == 5) {
            break
        }
        ++x
    } finally {
        t[9] = 12 + uno
        count ++
    }
    tt |> equal(count, 1)
    tt |> equal(t[9], 13)
}

var glob = 0
var glob_count = 0

def func_finally {
    return 12
} finally {
    glob = 13
    glob_count ++
}

def test_func_finally(tt : T?) {
    glob = 0
    glob_count = 0
    let t = func_finally()
    tt |> equal(t, 12)
    tt |> equal(glob, 13)
    tt |> equal(glob_count, 1)
}

[sideeffects]
def test_block_finally(tt : T?) {
    var t = 0
    invoke() <| $() { t = 1; } finally { t = 2; }
    tt |> equal(t, 2)
}

[test]
def test_finally(t : T?) {
    t |> run("if-break finally") @(t : T?) { test_if_finally(t); }
    t |> run("for-loop finally") @(t : T?) { test_for_finally(t); }
    t |> run("while-loop finally") @(t : T?) { test_while_finally(t); }
    t |> run("function finally") @(t : T?) { test_func_finally(t); }
    t |> run("block finally") @(t : T?) { test_block_finally(t); }
}
