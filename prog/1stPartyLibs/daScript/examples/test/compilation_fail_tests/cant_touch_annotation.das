expect 30111

require UnitTest

[checkRange,safe_when_uninitialized]
struct Foo
    [[min=0,max=3]] a:int = 1
    [[min=10,max=20]] b:int = 2
    [[min=2]] c:int
/*
30111: can't touch annotation
b out of annotated range [10..20]
c out of annotated range [2..2147483647]
*/


[export]
def test
    let t : Foo
    return true

