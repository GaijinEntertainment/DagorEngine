// Tests goto/label control flow: basic, nested, for-loop, while-loop, finally, computed goto
options gen2
require dastest/testing_boost public

[sideeffects]
def basic_goto(tt : T?) {
    var fail = false
    var arr : int[10]
    var i = 0
    label 0:
    arr[i] = i
    i ++
    if (i < 10) {
        goto label 0
        fail = true
        tt |> failure("unreachable code")
    }
    for (t in range(10)) {
        tt |> equal(arr[t], t)
    }
    tt |> success(!fail)
}

[sideeffects]
def nested_goto(tt : T?) {
    var fail = false
    var arr : int[10]
    var i = 0
    label 0:
    arr[i] = i
    i ++
    if (i < 10) {
        if (i < 10) {
            goto label 0
            fail = true
        }
        fail = true
    }
    for (t in range(10)) {
        tt |> equal(arr[t], t)
    }
}

[sideeffects]
def for_goto(tt : T?) {
    var fail = false
    var arr : int[10]
    for (j in range(10)) {
        var i = 0
        label 0:
        arr[i] = i + j
        i ++
        if (i < 10) {
            goto label 0
            fail = true
        }
        for (t in range(10)) {
            tt |> equal(arr[t], t + j)
        }
        tt |> success(!fail)
    }
    for (t in range(10)) {
        tt |> equal(arr[t], t + 9)
    }
    tt |> success(!fail)
}

[sideeffects]
def while_goto(tt : T?) {
    var fail = false
    var arr : int[10]
    var j = 0
    while (j < 10) {
        var i = 0
        label 0:
        arr[i] = i + j
        i ++
        if (i < 10) {
            goto label 0
            fail = true
        }
        for (t in range(10)) {
            tt |> equal(arr[t], t + j)
        }
        tt |> success(!fail)
        j ++
    }
    for (t in range(10)) {
        tt |> equal(arr[t], t + 9)
    }
    tt |> success(!fail)
}

[sideeffects, no_jit]
def finally_goto(tt : T?; yes : bool = true) {
    var fail = true
    if (yes) {
        fail = true
        goto label 0
    } finally {
        fail = false
    }
    label 0:
    tt |> success(!fail)
}

[sideeffects]
def gotoN(n : int) {
    var res = -1
    goto n
    label 0:
    res = 0
    goto label 3
    label 1:
    res = 1
    goto label 3
    label 2:
    res = 2
    goto label 3
    label 3:
    return res
}

[test]
def test_labels(t : T?) {
    t |> run("basic goto") @(t : T?) { basic_goto(t); }
    t |> run("nested goto") @(t : T?) { nested_goto(t); }
    t |> run("goto inside for-loop") @(t : T?) { for_goto(t); }
    t |> run("goto inside while-loop") @(t : T?) { while_goto(t); }
    t |> run("goto with finally") @(t : T?) { finally_goto(t); }
    t |> run("computed goto") @(t : T?) {
        t |> equal(gotoN(0), 0)
        t |> equal(gotoN(1), 1)
        t |> equal(gotoN(2), 2)
        t |> equal(gotoN(3), -1)
    }
}
