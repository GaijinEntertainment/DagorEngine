options gen2
require dastest/testing_boost

def make13(var a : int?) {
    *a = 13
    return a
}

def makeAll13(var a : int?; count : int) {
    for (i in range(count)) {
        unsafe(a[i]) = 13
    }
}

def addAll(var a : int?; count : int) {
    var c = 0
    for (i in range(count)) {
        c += unsafe(a[i])
    }
    return c
}

def ptrAdd(var a : int?; count : int) {
    var c = 0
    for (_i in range(count)) {
        c += *a
        unsafe { ++ a }   // nolint:STYLE025 - unsafe(++ a) does not authorize the lowered i_das_ptr_inc call
    }
    return c
}

def ptrSub(var a : int?; count : int) {
    var c = 0
    for (_i in range(count)) {
        c += *a
        unsafe { -- a }   // nolint:STYLE025 - unsafe(-- a) does not authorize the lowered i_das_ptr_dec call
    }
    return c
}

def ptrIncPostfix(var a : uint8?; count : int) {
    var c = 0
    for (_i in range(count)) {
        c += int(*a)
        unsafe { a++ }   // nolint:STYLE025 - unsafe(a++) does not authorize the lowered i_das_ptr_inc call
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
    t |> run("ptr ++ and -- walk the element stride") @(t : T?) {
        var x : int[10]
        for (i, j in x, range(10)) {
            i = j
        }
        var px = unsafe(addr(x[0]))
        var plast = unsafe(addr(x[9]))
        t |> success(!jit_enabled() || is_jit_function(@@ptrAdd))
        t |> success(!jit_enabled() || is_jit_function(@@ptrSub))
        t |> equal(ptrAdd(px, 10), 45)
        t |> equal(ptrSub(plast, 10), 45)
        t |> equal(ptrAdd(px, 3), 3)
        t |> equal(ptrSub(plast, 3), 24)
        var bytes : uint8[8]
        for (b, j in bytes, range(8)) {
            b = uint8(j * 3)
        }
        var pb = unsafe(addr(bytes[0]))
        t |> success(!jit_enabled() || is_jit_function(@@ptrIncPostfix))
        t |> equal(ptrIncPostfix(pb, 8), 84)
    }
}
