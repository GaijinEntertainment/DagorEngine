require UnitTest

def inside_scope_test
    var sptr <- new TestObjectSmart
    verify(getTotalTestObjectSmart()==1)
    return get_ptr(sptr)

def scope_test
    verify(getTotalTestObjectSmart()==0)
    let lostPtr = inside_scope_test()       // we loose one in there, return regular pointer, and cast it to smart_ptr again
    verify(getTotalTestObjectSmart()==1)
    unsafe
        var sptr : smart_ptr<TestObjectSmart> <- reinterpret< smart_ptr<TestObjectSmart> >(lostPtr)
        delete sptr
    verify(getTotalTestObjectSmart()==0)

def inside_move_test
    verify(getTotalTestObjectSmart()==0)
    var t1 <- new TestObjectSmart
    var t2 <- t1                            // unsafe operation
    assert(smart_ptr_use_count(t1)==0u)
    assert(smart_ptr_use_count(t2)==1u)
    verify(getTotalTestObjectSmart()==1)
    return <- t2

def move_test
    verify(getTotalTestObjectSmart()==0)
    var t <- inside_move_test()
    verify(getTotalTestObjectSmart()==1)
    unsafe
        delete t
    verify(getTotalTestObjectSmart()==0)

def ref_count_test
    verify(getTotalTestObjectSmart()==0)
    var sptr <- new TestObjectSmart         // ref_count = 1
    verify(getTotalTestObjectSmart()==1)
    assert(smart_ptr_use_count(sptr)==1u)
    var ptr = get_ptr(sptr)
    var pptr : TestObjectSmart?
    pptr = get_ptr(sptr)
    assert(sptr==ptr)
    assert(get_const_ptr(sptr)==ptr)
    var qptr : smart_ptr<TestObjectSmart>
    qptr := get_ptr(sptr)                   // ref_count = 2
    assert(smart_ptr_use_count(sptr)==2u)
    assert(qptr==ptr)
    assert(get_const_ptr(qptr)==ptr)
    assert(qptr==sptr)
    var fptr := qptr                        // ref_count = 3
    assert(smart_ptr_use_count(sptr)==3u)
    assert(fptr!=null)
    assert(get_const_ptr(fptr)==sptr)
    assert(fptr==qptr)
    assert(fptr==sptr)
    assert(smart_ptr_use_count(sptr)==3u)
    verify(getTotalTestObjectSmart()==1)
    unsafe
        delete sptr                             // ref_count = 2
    verify(getTotalTestObjectSmart()==1)
    assert(smart_ptr_use_count(sptr)==0u)
    assert(smart_ptr_use_count(qptr)==2u)
    unsafe
        delete qptr                             // ref_count = 1
    verify(getTotalTestObjectSmart()==1)
    assert(smart_ptr_use_count(qptr)==0u)
    assert(smart_ptr_use_count(fptr)==1u)
    unsafe
        delete fptr                             // physical delete
    verify(getTotalTestObjectSmart()==0)
    assert(smart_ptr_use_count(fptr)==0u)

def access_test
    verify(getTotalTestObjectSmart()==0)
    var ptr <- new TestObjectSmart
    assert(ptr.fooData == 1234)     //  .
    var t = ptr ?. fooData ?? 1     // ?? rvalue
    assert(t==1234)
    ptr ?. fooData ?? t = 13        // ?? lvalue
    assert(ptr.fooData == 13)
    unsafe
        delete ptr
    t = ptr ?. fooData ?? 2
    assert(t==2)
    unsafe
        ptr <- new TestObjectSmart
        ptr.first <- new TestObjectSmart
    assert(ptr.first.fooData==1234)
    assert(ptr ?. first ?. fooData ?? 13==1234)
    unsafe
        delete ptr.first
        delete ptr
    verify(getTotalTestObjectSmart()==0)

def fn_test
    verify(getTotalTestObjectSmart()==0)
    var ptr <- makeTestObjectSmart()
    assert(countTestObjectSmart(ptr)==1u)
    verify(getTotalTestObjectSmart()==1)
    unsafe
        delete ptr
    verify(getTotalTestObjectSmart()==0)

[export]
def test
    ref_count_test()
    move_test()
    scope_test()
    access_test()
    fn_test()
    return true
