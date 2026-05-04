// Tests safe index (?[]) on pointer types (new feature)
options gen2
require dastest/testing_boost public

struct Foo {
    a, b : int
}

[test]
def test_ptr_safe_at_int(t : T?) {
    t |> run("int pointer safe at - non null") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var dummy = 0
        unsafe {
            p?[0] ?? dummy = 1
        }
        t |> equal(ar[0], 1)
        t |> equal(dummy, 0)
    }
    t |> run("int pointer safe at - offset") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var dummy = 0
        unsafe {
            p?[2] ?? dummy = 42
        }
        t |> equal(ar[2], 42)
        t |> equal(dummy, 0)
    }
    t |> run("int pointer safe at - null") @(t : T?) {
        var p : int?
        var dummy = 0
        unsafe {
            p?[0] ?? dummy = 99
        }
        t |> equal(dummy, 99)
    }
    t |> run("int pointer safe at - null with offset") @(t : T?) {
        var p : int?
        var dummy = 0
        unsafe {
            p?[5] ?? dummy = 77
        }
        t |> equal(dummy, 77)
    }
    t |> run("int pointer safe at - read value") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val : int
        unsafe {
            val = p?[1] ?? 0
        }
        t |> equal(val, 20)
    }
    t |> run("int pointer safe at - read null") @(t : T?) {
        var p : int?
        var val : int
        unsafe {
            val = p?[0] ?? 42
        }
        t |> equal(val, 42)
    }
}

[test]
def test_ptr_safe_at_uint_index(t : T?) {
    t |> run("pointer safe at uint index - non null") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val : int
        unsafe {
            val = p?[1u] ?? 0
        }
        t |> equal(val, 20)
    }
    t |> run("pointer safe at uint index - null") @(t : T?) {
        var p : int?
        var val : int
        unsafe {
            val = p?[0u] ?? 42
        }
        t |> equal(val, 42)
    }
}

[test]
def test_ptr_safe_at_int64_index(t : T?) {
    t |> run("pointer safe at int64 index - non null") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val : int
        unsafe {
            val = p?[2l] ?? 0
        }
        t |> equal(val, 30)
    }
    t |> run("pointer safe at int64 index - null") @(t : T?) {
        var p : int?
        var val : int
        unsafe {
            val = p?[0l] ?? 42
        }
        t |> equal(val, 42)
    }
}

[test]
def test_ptr_safe_at_uint64_index(t : T?) {
    t |> run("pointer safe at uint64 index - non null") @(t : T?) {
        var ar = fixed_array(10, 20, 30)
        var p : int? = unsafe(addr(ar[0]))
        var val : int
        unsafe {
            val = p?[1ul] ?? 0
        }
        t |> equal(val, 20)
    }
    t |> run("pointer safe at uint64 index - null") @(t : T?) {
        var p : int?
        var val : int
        unsafe {
            val = p?[0ul] ?? 42
        }
        t |> equal(val, 42)
    }
}

[test]
def test_ptr_safe_at_struct(t : T?) {
    t |> run("struct pointer safe at - non null") @(t : T?) {
        var ar = fixed_array(Foo(a = 1, b = 2), Foo(a = 3, b = 4))
        var p : Foo? = unsafe(addr(ar[0]))
        var dummy = Foo()
        unsafe {
            let val = p?[1] ?? dummy
            t |> equal(val.a, 3)
            t |> equal(val.b, 4)
        }
    }
    t |> run("struct pointer safe at - null") @(t : T?) {
        var p : Foo?
        var dummy = Foo(a = 99, b = 88)
        unsafe {
            let val = p?[0] ?? dummy
            t |> equal(val.a, 99)
            t |> equal(val.b, 88)
        }
    }
}

[test]
def test_ptr_safe_at_ptr_to_ptr(t : T?) {
    t |> run("pointer to pointer safe at - non null") @(t : T?) {
        var a = 10
        var b = 20
        var ptrs = fixed_array(unsafe(addr(a)), unsafe(addr(b)))
        var pp : int ?? = unsafe(addr(ptrs[0]))
        var dummy : int?
        unsafe {
            let val = pp?[1] ?? dummy
            t |> success(val != null)
            t |> equal(deref(val), 20)
        }
    }
    t |> run("pointer to pointer safe at - null") @(t : T?) {
        var pp : int ??
        var dummy : int?
        unsafe {
            let val = pp?[0] ?? dummy
            t |> success(val == null)
        }
    }
}
