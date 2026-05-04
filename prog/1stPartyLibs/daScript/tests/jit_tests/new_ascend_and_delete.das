options gen2
options strict_smart_pointers = false

require dastest/testing_boost

require UnitTest

def test_new(t : T?) {
    var f = new FancyClass()
    t |> equal(13, f.value)
    unsafe {
        delete f
    }
}

def test_ascend(t : T?) {
    var f = new FancyClass(value = 123)
    t |> equal(123, f.value)
    unsafe {
        delete f
    }
}

def test_ascend_cmres(t : T?) {
    var f = new array<int>(1, 2)
    t |> equal(*f |> length(), 2)
    for (a, b in *f, [1, 2]) {
        t |> equal(a, b)
    }
}

def test_delete(t : T?; var f : FancyClass?) {
    unsafe {
        delete f
    }
    assert(f == null)
}

[test]
def test_new_and_ascend_delete(t : T?) {
    t |> run("new") @(t : T?) {
        test_new(t)
    }
    t |> run("ascend") @(t : T?) {
        test_ascend(t)
    }
    t |> run("ascend_cmres") @(t : T?) {
        test_ascend_cmres(t)
    }
    t |> run("delete") @(t : T?) {
        test_delete(t, new FancyClass())
    }
}

def return_smart_ptr(t : T?) {
    verify(getTotalTestObjectSmart() == 0)
    var ptr <- makeTestObjectSmart()
    t |> equal(1u, countTestObjectSmart(ptr))
    t |> equal(1, getTotalTestObjectSmart())
    unsafe {
        delete ptr
    }
    t |> equal(null, ptr)
    t |> equal(0, getTotalTestObjectSmart())
}

def inside_scope_test {
    var sptr <- new TestObjectSmart
    verify(getTotalTestObjectSmart() == 1)
    return get_ptr(sptr)
}

def scope_test(t : T?) {
    t |> equal(0, getTotalTestObjectSmart())
    let lostPtr = inside_scope_test()       // we loose one in there, return regular pointer, and cast it to smart_ptr again
    t |> equal(1, getTotalTestObjectSmart())
    unsafe {
        var sptr : smart_ptr<TestObjectSmart> <- reinterpret< smart_ptr<TestObjectSmart> >(lostPtr)
        delete sptr
    }
    t |> equal(0, getTotalTestObjectSmart())
}

def inside_move_test(t : T?) {
    t |> equal(0, getTotalTestObjectSmart())
    var t1 <- new TestObjectSmart
    var t2 <- t1                            // unsafe operation
    t |> equal(0u, smart_ptr_use_count(t1))
    t |> equal(1u, smart_ptr_use_count(t2))
    t |> equal(1, getTotalTestObjectSmart())
    return <- t2
}

def move_test(t : T?) {
    t |> equal(0, getTotalTestObjectSmart())
    var D <- inside_move_test(t)
    t |> equal(1, getTotalTestObjectSmart())
    unsafe {
        delete D
    }
    t |> equal(0, getTotalTestObjectSmart())
}

def ref_count_test(t : T?) {
    t |> equal(0, getTotalTestObjectSmart())
    var sptr <- new TestObjectSmart         // ref_count = 1
    t |> equal(1, getTotalTestObjectSmart())
    t |> equal(1u, smart_ptr_use_count(sptr))
    var ptr = get_ptr(sptr)
    var pptr : TestObjectSmart?
    pptr = get_ptr(sptr)
    t |> success(sptr == ptr)
    t |> success(get_ptr(sptr) == ptr)
    var qptr : smart_ptr<TestObjectSmart>
    qptr := get_ptr(sptr)                   // ref_count = 2
    t |> equal(2u, smart_ptr_use_count(sptr))
    t |> success(qptr == ptr)
    t |> success(get_ptr(qptr) == ptr)
    t |> success(qptr == sptr)
    var fptr := qptr                        // ref_count = 3
    t |> equal(3u, smart_ptr_use_count(sptr))
    t |> success(fptr != null)
    t |> success(get_ptr(fptr) == sptr)
    t |> success(fptr == qptr)
    t |> success(fptr == sptr)
    t |> equal(3u, smart_ptr_use_count(sptr))
    t |> equal(1, getTotalTestObjectSmart())
    unsafe {
        delete sptr                             // ref_count = 2
    }
    t |> equal(1, getTotalTestObjectSmart())
    t |> equal(0u, smart_ptr_use_count(sptr))
    t |> equal(2u, smart_ptr_use_count(qptr))
    unsafe {
        delete qptr                             // ref_count = 1
    }
    t |> equal(1, getTotalTestObjectSmart())
    t |> equal(0u, smart_ptr_use_count(qptr))
    t |> equal(1u, smart_ptr_use_count(fptr))
    unsafe {
        delete fptr                             // physical delete
    }
    t |> equal(0, getTotalTestObjectSmart())
    t |> equal(0u, smart_ptr_use_count(fptr))
}

def access_test(t : T?) {
    t |> equal(0, getTotalTestObjectSmart())
    var ptr <- new TestObjectSmart
    t |> equal(1234, ptr.fooData)    //  .
    var D = ptr ?. fooData ?? 1     // ?? rvalue
    t |> equal(1234, D)
    ptr ?. fooData ?? D = 13        // ?? lvalue
    t |> equal(13, ptr.fooData)
    unsafe {
        delete ptr
    }
    D = ptr ?. fooData ?? 2
    t |> equal(2, D)
    unsafe {
        ptr <- new TestObjectSmart
        ptr.first <- new TestObjectSmart
    }
    t |> success(ptr.first.fooData == 1234)
    t |> success(ptr ?. first ?. fooData ?? 13 == 1234)
    unsafe {
        delete ptr.first
        delete ptr
    }
    t |> equal(0, getTotalTestObjectSmart())
}

[test]
def test_smart_ptr(t : T?) {
    t |> run("scope test") @(t : T?) {
        scope_test(t)
    }
    t |> run("move test") @(t : T?) {
        scope_test(t)
    }
    t |> run("ref count test") @(t : T?) {
        ref_count_test(t)
    }
    t |> run("access test") @(t : T?) {
        access_test(t)
    }
    t |> run("return smart ptr from C++") @(t : T?) {
        return_smart_ptr(t)
    }
}

