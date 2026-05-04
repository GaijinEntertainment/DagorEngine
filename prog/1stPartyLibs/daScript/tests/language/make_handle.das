options gen2
require UnitTest
require dastest/testing_boost public

def cmresTest(a : int) {
    return TestObjectFoo(fooData = a)
}

struct OverFoo {
    a : int
    foo : TestObjectFoo
}

var {
    gstackref = TestObjectFoo(fooData = 6)
    gstackrefa : TestObjectFoo[2]
}

[test]
def test_make_handle(t : T?) {
    // stack-ref initialization
    gstackrefa[0] = TestObjectFoo(fooData = 1)
    gstackrefa[1] = TestObjectFoo(fooData = 2)
    t |> equal(gstackref.fooData, 6)
    t |> success(gstackrefa[0].fooData == 1 && gstackrefa[1].fooData == 2)
    // regular local variable
    let local_t = TestObjectFoo(fooData = 5)
    t |> equal(local_t.fooData, 5)
    // cmres variable
    var cmres_t = cmresTest(13)
    t |> equal(cmres_t.fooData, 13)
    // mixed in the other type
    var mixed = fixed_array(OverFoo(a = 1, foo = TestObjectFoo(fooData = 1)), OverFoo(a = 2, foo = TestObjectFoo(fooData = 2)))
    t |> success(mixed[0].foo.fooData == 1 && mixed[1].foo.fooData == 2)
    // ascend
    var ascend_t <- new TestObjectFoo(fooData = 1)
    t |> equal(ascend_t.fooData, 1)
    unsafe {
        delete ascend_t
    }
    // ascend with smart-ptr
    // TODO: fix this (aot is broken)
    // var inscope ascend_smart_t <- new TestObjectSmart(fooData=1)
    // assert(ascend_smart_t.fooData==1)
    // unsafe {
    //     delete ascend_smart_t
    // }
    t |> success(true)
}

