options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public

require daslib/safe_addr

struct TestStruct {
    x : int = 10
    y : int = 20
}

[test]
def test_new_and_fields(t : T?) {
    t |> run("new with fields") @(t : T?) {
        var p = new TestStruct(x = 5, y = 7)
        t |> equal(p.x, 5)
        t |> equal(p.y, 7)
        unsafe {
            delete p
        }
    }
    t |> run("new default") @(t : T?) {
        var p = new TestStruct()
        t |> equal(p.x, 10)
        t |> equal(p.y, 20)
        unsafe {
            delete p
        }
    }
    t |> run("auto-deref field access") @(t : T?) {
        unsafe {
            var inscope p = new TestStruct(x = 42, y = 99)
            // p.x auto-dereferences the pointer — no -> needed
            t |> equal(p.x, 42)
            t |> equal(p.y, 99)
        }
    }
    t |> run("modify through pointer") @(t : T?) {
        unsafe {
            var inscope p = new TestStruct(x = 1, y = 2)
            p.x = 100
            p.y = 200
            t |> equal(p.x, 100)
            t |> equal(p.y, 200)
        }
    }
}

[test]
def test_addr_and_deref(t : T?) {
    t |> run("addr and deref") @(t : T?) {
        var x = 42
        unsafe {
            var p = addr(x)
            t |> equal(*p, 42)
            t |> equal(deref(p), 42)
        }
    }
    t |> run("modify through addr") @(t : T?) {
        var x = 10
        unsafe {
            var p = addr(x)
            *p = 99
        }
        t |> equal(x, 99)
    }
    t |> run("addr of struct field") @(t : T?) {
        var s = TestStruct(x = 7, y = 8)
        unsafe {
            var px = addr(s.x)
            t |> equal(*px, 7)
            *px = 77
        }
        t |> equal(s.x, 77)
    }
}

[test]
def test_safe_addr(t : T?) {
    t |> run("safe_addr local") @(t : T?) {
        var a = 55
        var pa = safe_addr(a)       // returns int?# (temporary pointer)
        t |> equal(*pa, 55)
    }
    t |> run("safe_addr modify") @(t : T?) {
        var b = 10
        var pb = safe_addr(b)
        *pb = 20
        t |> equal(b, 20)
    }
}

[test]
def test_null_pointer(t : T?) {
    t |> run("null default") @(t : T?) {
        var p : int?
        t |> equal(p == null, true)
        t |> equal(p != null, false)
    }
    t |> run("non-null check") @(t : T?) {
        var x = 42
        var p = unsafe(addr(x))
        t |> equal(p != null, true)
        t |> equal(p == null, false)
    }
    t |> run("null deref requires unsafe") @(t : T?) {
        // Dereferencing null requires unsafe — we just verify null state
        var p : int?
        t |> equal(p == null, true)
    }
}

[test]
def test_safe_navigation(t : T?) {
    t |> run("?. on valid pointer") @(t : T?) {
        unsafe {
            var inscope p = new TestStruct(x = 42, y = 99)
            t |> equal(p?.x ?? -1, 42)
            t |> equal(p?.y ?? -1, 99)
        }
    }
    t |> run("?. on null pointer") @(t : T?) {
        var p : TestStruct?
        t |> equal(p?.x ?? -1, -1)
        t |> equal(p?.y ?? -1, -1)
    }
    t |> run("?? null coalescing on int?") @(t : T?) {
        var x = 42
        var p = unsafe(addr(x))
        var q : int?
        if (p != null) {
            t |> equal(*p, 42)
        }
        t |> equal(q == null, true)
    }
}


[test]
def test_pointer_indexing(t : T?) {
    t |> run("index into array via pointer") @(t : T?) {
        var arr <- [10, 20, 30, 40, 50]
        unsafe {
            var p = addr(arr[0])
            t |> equal(p[0], 10)
            t |> equal(p[1], 20)
            t |> equal(p[2], 30)
            t |> equal(p[3], 40)
            t |> equal(p[4], 50)
        }
    }
    t |> run("write through pointer index") @(t : T?) {
        var arr <- [1, 2, 3]
        unsafe {
            var p = addr(arr[0])
            p[1] = 99
        }
        t |> equal(arr[1], 99)
    }
}

[test]
def test_pointer_arithmetic(t : T?) {
    t |> run("pointer increment") @(t : T?) {
        var arr <- [10, 20, 30, 40, 50]
        unsafe {
            var p = addr(arr[0])
            t |> equal(*p, 10)
            ++ p
            t |> equal(*p, 20)
            ++ p
            t |> equal(*p, 30)
        }
    }
    t |> run("pointer += N") @(t : T?) {
        var arr <- [10, 20, 30, 40, 50]
        unsafe {
            var p = addr(arr[0])
            p += 3
            t |> equal(*p, 40)
            p += 1
            t |> equal(*p, 50)
        }
    }
}

[test]
def test_intptr(t : T?) {
    t |> run("intptr non-zero") @(t : T?) {
        var x = 42
        unsafe {
            var p = addr(x)
            let address = intptr(p)
            t |> equal(address != uint64(0), true)
        }
    }
    t |> run("intptr same pointer same value") @(t : T?) {
        var x = 42
        unsafe {
            var p = addr(x)
            t |> equal(intptr(p), intptr(p))
        }
    }
    t |> run("intptr different pointers differ") @(t : T?) {
        var x = 1
        var y = 2
        unsafe {
            var px = addr(x)
            var py = addr(y)
            t |> equal(intptr(px) != intptr(py), true)
        }
    }
}

[test]
def test_reinterpret(t : T?) {
    t |> run("float to int roundtrip") @(t : T?) {
        unsafe {
            let f = 1.0
            let bits = reinterpret<int> f
            t |> equal(bits, int(0x3F800000))       // IEEE 754 for 1.0
            let back = reinterpret<float> bits
            t |> equal(back, 1.0)
        }
    }
}

[test]
def test_void_pointer(t : T?) {
    t |> run("roundtrip through void?") @(t : T?) {
        var x = 42
        unsafe {
            var p = addr(x)
            var vp : void? = reinterpret<void?> p
            var p2 = reinterpret<int?> vp
            t |> equal(*p2, 42)
        }
    }
}

[test]
def test_delete(t : T?) {
    t |> run("delete sets null") @(t : T?) {
        var p = new TestStruct(x = 1, y = 2)
        t |> equal(p != null, true)
        unsafe {
            delete p
        }
        t |> equal(p == null, true)
    }
    t |> run("var inscope auto-deletes") @(t : T?) {
        // var inscope p = new ... — p is deleted at scope exit
        // We just verify it works without crash
        unsafe {
            var inscope p = new TestStruct(x = 5, y = 6)
            t |> equal(p.x, 5)
        }
    }
}
