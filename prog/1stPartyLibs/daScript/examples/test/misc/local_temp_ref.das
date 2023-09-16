struct Foo
    a : int

var foo : Foo

def getFoo : Foo &
    unsafe
        return foo

def getFooT : Foo &#
    unsafe
        return reinterpret<Foo&#> foo

[export]
def main
    var v1 & = getFooT()    // NOT UNSAFE ANYMORE, v1 is Foo#&
    v1.a = 13
    print("v1.a = {v1.a}\n")
    unsafe
        var v2 & = getFoo()
        print("v2.a = {v2.a}\n")
        v2.a = 14
        print("v1.a = {v1.a}\n")

options log
