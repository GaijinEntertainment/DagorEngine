// options log=true
// options log_infer_passes=true // log_nodes=false

options persistent_heap = true

require strings
require UnitTest

struct SomethingSmall
    a, b, c : int

def test_delete_array
    var a : array<int> [5]
    let w0 = heap_bytes_allocated()
    for t in range(5)
        resize(a[t],10)
    let w1 = heap_bytes_allocated()
    delete a
    let w2 = heap_bytes_allocated()
    assert(w1>w0 && w0==w2)
    for t in range(5)
        assert(length(a[t])==0 && capacity(a[t])==0)

def test_delete_table
    var a : table<int;int>
    let w0 = heap_bytes_allocated()
    a |> insert(0,1)
    a |> insert(1,2)
    let w1 = heap_bytes_allocated()
    delete a
    let w2 = heap_bytes_allocated()
    assert(w1>w0 && w0==w2)
    assert(length(a)==0 && capacity(a)==0)

def test_delete_struct
    var a : SomethingSmall?
    let w0 = heap_bytes_allocated()
    a = new SomethingSmall
    let w1 = heap_bytes_allocated()
    unsafe
        delete a
    let w2 = heap_bytes_allocated()
    assert(w1>w0 && w0==w2)
    assert(a==null)

def test_delete_struct_array
    let w0 = heap_bytes_allocated()
    var a = new <SomethingSmall[10]>
    let w1 = heap_bytes_allocated()
    unsafe
        delete a
    let w2 = heap_bytes_allocated()
    assert(w1>w0 && w0==w2)
    for pA in a
        assert(pA==null)

def test_delete_handle
    var a : TestObjectFoo?
    a = new TestObjectFoo
    unsafe
        delete a
    assert(a==null)

def test_delete_handle_array
    var a  = new <TestObjectFoo[10]>
    unsafe
        delete a
    for pA in a
        assert(pA==null)

[sideeffects]
def da_inscope(x)
    var b = new SomethingSmall
    var a = new <SomethingSmall[10]>
    b.a = x
    for pA in a
        pA.a = x
finally
    unsafe
        delete a
        delete b

def test_delete_struct_array_in_scope
    let w0 = heap_bytes_allocated()
    da_inscope(2)
    let w1 = heap_bytes_allocated()
    assert(w0==w1)

[sideeffects]
def test_delete_string()
    let w0 = heap_bytes_allocated()
    var a = "{deref(getPtr())}"
    // assert(a=="0")
    unsafe
        delete_string(a)
    let w1 = heap_bytes_allocated()
    assert(w0==w1)
    assert(a=="")

[sideeffects]
def test_delete_string_array()
    let w0 = heap_bytes_allocated()
    var a = [[string[3] "{deref(getPtr())}_1"; "{deref(getPtr())}_2"; "{deref(getPtr())}_3"]]
    for AA in a
        unsafe
            delete_string(AA)
    let w1 = heap_bytes_allocated()
    assert(w0==w1)

[export]
def test
    test_delete_struct_array_in_scope()
    test_delete_handle()
    test_delete_handle_array()
    test_delete_struct()
    test_delete_struct_array()
    test_delete_array()
    test_delete_table()
    test_delete_string()
    test_delete_string_array()
    return true
