options gen2
require dastest/testing_boost public

// Tests CMRES (copy-move-return-semantics) invoke patterns:
// regular functions, hybrid functions, function pointers, blocks, and lambdas
// returning structs by value.

struct Foo {
    a, b : int
}

def func : Foo {
    return Foo(a = 1, b = 2)
}

def funcC : Foo const {
    return Foo(a = 1, b = 2)
}

def funcA(t : int) : Foo {
    return Foo(a = t, b = t + 1)
}

def funcAC(t : int) : Foo const {
    return Foo(a = t, b = t + 1)
}

[hybrid]
def hfunc : Foo {
    return Foo(a = 1, b = 2)
}

[hybrid]
def hfuncC : Foo const {
    return Foo(a = 1, b = 2)
}

[hybrid]
def hfuncA(t : int) : Foo {
    return Foo(a = t, b = t + 1)
}

[hybrid]
def hfuncAC(t : int) : Foo const {
    return Foo(a = t, b = t + 1)
}

def check(t : T?; f : Foo) {
    t |> equal(f.a, 1)
    t |> equal(f.b, 2)
}

def checkInvoke(t : T?; obj) {
    check(t, invoke(obj))
}

def checkInvoke(t : T?; arg; obj) {
    check(t, invoke(obj, arg))
}

[test]
def test_invoke_cmres(t : T?) {
    t |> run("direct function calls") @(t : T?) {
        check(t, func())
        check(t, funcA(1))
        check(t, funcC())
        check(t, funcAC(1))
    }
    t |> run("hybrid function calls") @(t : T?) {
        check(t, hfunc())
        check(t, hfuncA(1))
        check(t, hfuncC())
        check(t, hfuncAC(1))
    }
    t |> run("invoke function pointers") @(t : T?) {
        checkInvoke(t, @@func)
        checkInvoke(t, 1, @@funcA)
        checkInvoke(t, @@funcC)
        checkInvoke(t, 1, @@funcAC)
    }
    t |> run("invoke blocks") @(t : T?) {
        checkInvoke(t) <| $() : Foo {
            return Foo(a = 1, b = 2)
        }
        checkInvoke(t) <| $() : Foo const {
            return Foo(a = 1, b = 2)
        }
        checkInvoke(t, 1) <| $(tt : int) : Foo {
            return Foo(a = tt, b = tt + 1)
        }
        checkInvoke(t, 1) <| $(tt : int) : Foo const {
            return Foo(a = tt, b = tt + 1)
        }
    }
    t |> run("invoke lambdas") @(t : T?) {
        checkInvoke(t) <| @() : Foo {
            return Foo(a = 1, b = 2)
        }
        checkInvoke(t) <| @() : Foo const {
            return Foo(a = 1, b = 2)
        }
        checkInvoke(t, 1) <| @(tt : int) : Foo {
            return Foo(a = tt, b = tt + 1)
        }
        checkInvoke(t, 1) <| @(tt : int) : Foo const {
            return Foo(a = tt, b = tt + 1)
        }
    }
}
