options gen2
require dastest/testing_boost

def make13(var a : int?) {
    *a = 13
    return a
}

def makeAll13(var a : int?; count : int) {
    for (i in range(count)) {
        unsafe {
            a[i] = 13
        }
    }
}

def addAll(var a : int?; count : int) {
    var c = 0
    for (i in range(count)) {
        unsafe {
            c += a[i]
        }
    }
    return c
}

def ptrAdd(var a : int?; count : int) {
    var c = 0
    for (i in range(count)) {
        c += *a
        unsafe {
            ++ a
        }
    }
    return c
}

[test]
def test_ref(t : T?) {
    t |> run("ptr2ref") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@make13))
        var x = -21
        var px = unsafe(addr(x))
        var ppx = make13(px)
        t |> equal(x, 13)
        t |> equal(px, ppx)
        t |> equal(*px, 13)
    }
    t |> run("ptr []") @(t : T?) {
        var x : int[10]
        var px = unsafe(addr(x[0]))
        t |> success(!jit_enabled() || is_jit_function(@@makeAll13))
        t |> success(!jit_enabled() || is_jit_function(@@addAll))
        makeAll13(px, 10)
        for (i in x) {
            t |> equal(i, 13)
        }
        t |> equal(addAll(px, 10), 130)
    }
    t |> run("ptr ++") @(t : T?) {
        var x : int[10]
        var px = unsafe(addr(x[0]))
        for (i, j in x, range(10)) {
            i = j
        }
        t |> success(!jit_enabled() || is_jit_function(@@addAll))
        t |> equal(addAll(px, 10), 45)
    }
}
