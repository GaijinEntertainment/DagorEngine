require UnitTest

def cmresTest(a:int)
    return [[TestObjectFoo fooData=a]]

struct OverFoo
    a : int
    foo : TestObjectFoo

var
    gstackref = [[TestObjectFoo fooData=6]]
    gstackrefa = [[TestObjectFoo fooData=1; fooData=2]]

[export]
def test
    // stack-ref initialization
    assert(gstackref.fooData==6)
    assert(gstackrefa[0].fooData==1 && gstackrefa[1].fooData==2)
    // regular local variable
    let local_t = [[TestObjectFoo fooData=5]]
    assert(local_t.fooData==5)
    // cmres variable
    var cmres_t = cmresTest(13)
    assert(cmres_t.fooData==13)
    // mixed in the other type
    var mixed = [[OverFoo[2] a=1, foo = [[TestObjectFoo fooData=1]]; a=2, foo = [[TestObjectFoo fooData=2]] ]]
    assert(mixed[0].foo.fooData==1 && mixed[1].foo.fooData==2)
    // ascend
    var ascend_t <- new [[TestObjectFoo() fooData=1]]
    assert(ascend_t.fooData==1)
    unsafe
        delete ascend_t
    // ascend with smart-ptr
    var ascend_smart_t <- new [[TestObjectSmart() fooData=1]]
    assert(ascend_smart_t.fooData==1)
    unsafe
        delete ascend_smart_t
    return true

