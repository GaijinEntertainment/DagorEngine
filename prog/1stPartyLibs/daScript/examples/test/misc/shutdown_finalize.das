options log

require daslib/static_let

struct Foo
    a : int

def Foo()
    print("in foo c-tor\n")
    var f : Foo
    f.a = 5
    return f

def operator delete(var f:Foo)
    print("killing foo\n")

[finalize]
def byebye()
    print("bytebye, world\n")

[export]
def main()
    static_let_finalize <|
        var x = new Foo()
    print("hello, world\n")
    print("x = {x}\n")

options log
