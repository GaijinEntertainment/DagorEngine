//options log = true
//options log_aot = true
//options log_nodes = true
//options log_stack = true
//options log_cpp = true

struct Foo
    a, b : int

def func : Foo
    return [[Foo a = 1, b = 2]]

def funcC : Foo const
    return [[Foo a = 1, b = 2]]

def funcA ( t : int ) : Foo
    return [[Foo a = t, b = t + 1]]

def funcAC ( t : int ) : Foo const
    return [[Foo a = t, b = t + 1]]

[hybrid]
def hfunc : Foo
    return [[Foo a = 1, b = 2]]

[hybrid]
def hfuncC : Foo const
    return [[Foo a = 1, b = 2]]

[hybrid]
def hfuncA ( t : int ) : Foo
    return [[Foo a = t, b = t + 1]]

[hybrid]
def hfuncAC ( t : int ) : Foo const
    return [[Foo a = t, b = t + 1]]

[sideeffects]
def check(t:Foo)
    assert(t.a==1 && t.b==2)

[sideeffects]
def checkInvoke(obj)
    check(invoke(obj))

[sideeffects]
def checkInvoke(arg;obj)
    check(invoke(obj,arg))

[export]
def test
    // function as is
    check(func())
    check(funcA(1))
    check(funcC())
    check(funcAC(1))
    // hybrid function as is
    check(hfunc())
    check(hfuncA(1))
    check(hfuncC())
    check(hfuncAC(1))
    // invoke function
    checkInvoke(@@func)
    checkInvoke(1,@@funcA)
    checkInvoke(@@funcC)
    checkInvoke(1,@@funcAC)
    // invoke block
    checkInvoke() <| $() : Foo
        return [[Foo a = 1, b = 2]]
    checkInvoke() <| $() : Foo const
        return [[Foo a = 1, b = 2]]
    checkInvoke(1) <| $( t : int ) : Foo
        return [[Foo a = t, b = t + 1]]
    checkInvoke(1) <| $( t : int ) : Foo const
        return [[Foo a = t, b = t + 1]]
    // invoke lambda
    checkInvoke() <| @() : Foo
        return [[Foo a = 1, b = 2]]
    checkInvoke() <| @() : Foo const
        return [[Foo a = 1, b = 2]]
    checkInvoke(1) <| @( t : int ) : Foo
        return [[Foo a = t, b = t + 1]]
    checkInvoke(1) <| @( t : int ) : Foo const
        return [[Foo a = t, b = t + 1]]
    return true
