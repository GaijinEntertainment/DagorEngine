struct Foo
    a : int
    b : float

def Foo ( p : Foo? )
    var self := *p
    return <- self

[export]
def main
    let a = new [[Foo a=1,b=2.]]
    let b = new Foo(a)
    print("a={a}\nb={b}\n")
