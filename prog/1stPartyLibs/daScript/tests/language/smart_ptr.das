options gen2
require UnitTest
require dastest/testing_boost public

def inside_scope_test(t : T?) {
    unsafe {
        var sptr <- new TestObjectSmart
        t |> equal(getTotalTestObjectSmart(), 1)
        return get_ptr(sptr)
    }
}

def scope_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    let lostPtr = inside_scope_test(t)       // we loose one in there, return regular pointer, and cast it to smart_ptr again
    t |> equal(getTotalTestObjectSmart(), 1)
    unsafe {
        var sptr : smart_ptr<TestObjectSmart> <- reinterpret< smart_ptr<TestObjectSmart> >(lostPtr)
        delete sptr
    }
    t |> equal(getTotalTestObjectSmart(), 0)
}

def inside_move_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    var inscope t1 <- new TestObjectSmart
    var inscope t2 <- t1                            // unsafe operation
    t |> equal(smart_ptr_use_count(t1), 0u)
    t |> equal(smart_ptr_use_count(t2), 1u)
    t |> equal(getTotalTestObjectSmart(), 1)
    return <- t2
}

def move_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    {
        var inscope sptr <- inside_move_test(t)
        t |> equal(getTotalTestObjectSmart(), 1)
    }
    t |> equal(getTotalTestObjectSmart(), 0)
}

def ref_count_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    var inscope sptr <- new TestObjectSmart         // ref_count = 1
    t |> equal(getTotalTestObjectSmart(), 1)
    t |> equal(smart_ptr_use_count(sptr), 1u)
    var ptr = get_ptr(sptr)
    var pptr : TestObjectSmart?
    pptr = get_ptr(sptr)
    t |> equal(sptr, ptr)
    t |> equal(get_const_ptr(sptr), ptr)
    var inscope qptr : smart_ptr<TestObjectSmart>
    qptr := get_ptr(sptr)                   // ref_count = 2
    t |> equal(smart_ptr_use_count(sptr), 2u)
    t |> equal(qptr, ptr)
    t |> equal(get_const_ptr(qptr), ptr)
    t |> equal(qptr, sptr)
    var inscope fptr := qptr                        // ref_count = 3
    t |> equal(smart_ptr_use_count(sptr), 3u)
    t |> success(fptr != null)
    t |> equal(get_const_ptr(fptr), sptr)
    t |> equal(fptr, qptr)
    t |> equal(fptr, sptr)
    t |> equal(smart_ptr_use_count(sptr), 3u)
    t |> equal(getTotalTestObjectSmart(), 1)
    unsafe {
        delete sptr                             // ref_count = 2
    }
    t |> equal(getTotalTestObjectSmart(), 1)
    t |> equal(smart_ptr_use_count(sptr), 0u)
    t |> equal(smart_ptr_use_count(qptr), 2u)
    unsafe {
        delete qptr                             // ref_count = 1
    }
    t |> equal(getTotalTestObjectSmart(), 1)
    t |> equal(smart_ptr_use_count(qptr), 0u)
    t |> equal(smart_ptr_use_count(fptr), 1u)
    unsafe {
        delete fptr                             // physical delete
    }
    t |> equal(getTotalTestObjectSmart(), 0)
    t |> equal(smart_ptr_use_count(fptr), 0u)
}

def access_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    var inscope ptr <- new TestObjectSmart
    t |> equal(ptr.fooData, 1234)     //  .
    var val = ptr ?. fooData ?? 1     // ?? rvalue
    t |> equal(val, 1234)
    ptr ?. fooData ?? val = 13        // ?? lvalue
    t |> equal(ptr.fooData, 13)
    unsafe {
        delete ptr
    }
    val = ptr ?. fooData ?? 2
    t |> equal(val, 2)
    unsafe {
        ptr <- new TestObjectSmart
        ptr.first <- new TestObjectSmart
    }
    t |> equal(ptr.first.fooData, 1234)
    t |> equal(ptr ?. first ?. fooData ?? 13, 1234)
    unsafe {
        delete ptr.first
        delete ptr
    }
    t |> equal(getTotalTestObjectSmart(), 0)
}

def fn_test(t : T?) {
    t |> equal(getTotalTestObjectSmart(), 0)
    var inscope ptr <- makeTestObjectSmart()
    t |> equal(countTestObjectSmart(ptr), 1u)
    t |> equal(getTotalTestObjectSmart(), 1)
    unsafe {
        delete ptr
    }
    t |> equal(getTotalTestObjectSmart(), 0)
}

[test]
def test_smart_ptr(t : T?) {
    ref_count_test(t)
    move_test(t)
    scope_test(t)
    access_test(t)
    fn_test(t)
}
