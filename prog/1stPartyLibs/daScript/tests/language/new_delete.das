options gen2
// options log=true
// options log_infer_passes=true // log_nodes=false

options no_coverage
options persistent_heap = true

require strings
require UnitTest
require dastest/testing_boost public

struct SomethingSmall {
    a, b, c : int
}

def test_delete_array(t : T?) {
    var a : array<int>[5]
    let w0 = heap_bytes_allocated()
    for (ii in range(5)) {
        resize(a[ii], 10)
    }
    let w1 = heap_bytes_allocated()
    delete a
    let w2 = heap_bytes_allocated()
    t |> success(w1 > w0 && w0 == w2)
    for (ii in range(5)) {
        t |> success(length(a[ii]) == 0 && capacity(a[ii]) == 0)
    }
}

def test_delete_table(t : T?) {
    var a : table<int; int>
    let w0 = heap_bytes_allocated()
    a |> insert(0, 1)
    a |> insert(1, 2)
    let w1 = heap_bytes_allocated()
    delete a
    let w2 = heap_bytes_allocated()
    t |> success(w1 > w0 && w0 == w2)
    t |> success(length(a) == 0 && capacity(a) == 0)
}

def test_delete_struct(t : T?) {
    var a : SomethingSmall?
    let w0 = heap_bytes_allocated()
    a = new SomethingSmall
    let w1 = heap_bytes_allocated()
    unsafe {
        delete a
    }
    let w2 = heap_bytes_allocated()
    t |> success(w1 > w0 && w0 == w2)
    t |> equal(a, null)
}

def test_delete_struct_array(t : T?) {
    let w0 = heap_bytes_allocated()
    var a = new <SomethingSmall[10]>
    let w1 = heap_bytes_allocated()
    unsafe {
        delete a
    }
    let w2 = heap_bytes_allocated()
    t |> success(w1 > w0 && w0 == w2)
    for (pA in a) {
        t |> equal(pA, null)
    }
}

def test_delete_handle(t : T?) {
    var a : TestObjectFoo?
    a = new TestObjectFoo
    unsafe {
        delete a
    }
    t |> equal(a, null)
}

def test_delete_handle_array(t : T?) {
    var a  = new <TestObjectFoo[10]>
    unsafe {
        delete a
    }
    for (pA in a) {
        t |> equal(pA, null)
    }
}

[sideeffects]
def da_inscope(x) {
    var b = new SomethingSmall
    var a = new <SomethingSmall[10]>
    b.a = x
    for (pA in a) {
        pA.a = x
    }
} finally {
    unsafe {
        delete a
        delete b
    }
}

def test_delete_struct_array_in_scope(t : T?) {
    let w0 = heap_bytes_allocated()
    da_inscope(2)
    let w1 = heap_bytes_allocated()
    t |> equal(w0, w1)
}

[sideeffects]
def test_delete_string(t : T?) {
    let w0 = heap_bytes_allocated()
    var a = "{deref(getPtr())}"
    // assert(a=="0")
    unsafe {
        delete_string(a)
    }
    let w1 = heap_bytes_allocated()
    t |> equal(w0, w1)
    t |> equal(a, "")
}

[sideeffects]
def test_delete_string_array(t : T?) {
    let w0 = heap_bytes_allocated()
    var a = fixed_array<string>("{deref(getPtr())}_1", "{deref(getPtr())}_2", "{deref(getPtr())}_3")
    for (AA in a) {
        unsafe {
            delete_string(AA)
        }
    }
    let w1 = heap_bytes_allocated()
    t |> equal(w0, w1)
}

[test]
def test_new_delete(t : T?) {
    test_delete_struct_array_in_scope(t)
    test_delete_handle(t)
    test_delete_handle_array(t)
    test_delete_struct(t)
    test_delete_struct_array(t)
    test_delete_array(t)
    test_delete_table(t)
    test_delete_string(t)
    test_delete_string_array(t)
}
