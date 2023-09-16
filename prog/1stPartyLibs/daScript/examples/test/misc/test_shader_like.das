require daslib/validate_code
require daslib/lpipe

options shader_like

// this fails
var arr : array<int>    // 40104: global variable arr requires heap

def add(a,b)
    return a + b

struct Foo
    a : int = 13

[export]
def main
    var i = add(1)
    lpipe(2)
    print("i = {i}\n")

    print("arr = {arr}\n")

    // this fails
    var lmb <- @ <|         // 40104: not shader-like, type requires heap lambda<void>
        print("hello\n")    // 40104: can't ascend in shader-like code (new [[...]])
    invoke(lmb)

    // this fails
    var f = new Foo()       // 40104: can't new in shader-like code (new ...)

