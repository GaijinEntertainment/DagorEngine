// Tests safe index (?[]) on dynamic arrays, tables, fixed arrays, and vectors
options gen2
require dastest/testing_boost public

def safe_addr(var a) {
    unsafe {
        return addr(a)
    }
}

def safe_addr_val(var a : auto&) {
    unsafe {
        return addr(a)
    }
}

[test]
def test_safe_index(t : T?) {
    t |> run("dynamic array safe index") @(t : T?) {
        var ar : array<int>
        var dummy : int
        ar?[0] ?? dummy = 1
        t |> equal(dummy, 1)
        resize(ar, 1)
        ar?[0] ?? dummy = 2
        t |> equal(length(ar), 1)
        t |> equal(ar[0], 2)
        t |> equal(dummy, 1)
    }
    t |> run("array pointer safe index") @(t : T?) {
        var ar : array<int>
        var par = safe_addr(ar)
        var dummy = 0
        par?[0] ?? dummy = 1
        t |> equal(dummy, 1)
        resize(ar, 1)
        par?[0] ?? dummy = 2
        t |> equal(ar[0], 2)
        t |> equal(dummy, 1)
        par = null
        par?[0] ?? dummy = 3
        t |> equal(dummy, 3)
    }
    t |> run("table safe index") @(t : T?) {
        var ta : table<string; int>
        var dummy = 0
        ta?["one"] ?? dummy = 1
        t |> equal(dummy, 1)
        ta |> insert("one", 0)
        ta?["one"] ?? dummy = 2
        t |> equal(ta |> get_value("one"), 2)
        t |> equal(dummy, 1)
    }
    t |> run("table pointer safe index") @(t : T?) {
        var ta : table<string; int>
        var pta = safe_addr(ta)
        var dummy = 0
        pta?["one"] ?? dummy = 1
        t |> equal(dummy, 1)
        ta |> insert("one", 1)
        pta?["one"] ?? dummy = 2
        t |> equal(ta |> get_value("one"), 2)
        t |> equal(dummy, 1)
        pta = null
        pta?["one"] ?? dummy = 3
        t |> equal(dummy, 3)
    }
    t |> run("fixed array safe index") @(t : T?) {
        var di : int[2]
        var dummy = 0
        di?[3] ?? dummy = 1
        t |> equal(dummy, 1)
        di?[0] ?? dummy = 2
        t |> equal(di[0], 2)
        t |> equal(dummy, 1)
    }
    t |> run("fixed array pointer safe index") @(t : T?) {
        var di : int[2]
        var pdi = safe_addr(di)
        var dummy = 0
        pdi?[3] ?? dummy = 1
        t |> equal(dummy, 1)
        di[0] = 0
        pdi?[0] ?? dummy = 2
        t |> equal(di[0], 2)
        t |> equal(dummy, 1)
        pdi = null
        pdi?[0] ?? dummy = 3
        t |> equal(dummy, 3)
    }
    t |> run("vector safe index") @(t : T?) {
        var vec = int3(1, 2, 3)
        var dummy = 0
        vec?[3] ?? dummy = 1
        t |> equal(dummy, 1)
        vec?[0] ?? dummy = 2
        t |> equal(vec[0], 2)
        t |> equal(dummy, 1)
    }
    t |> run("vector pointer safe index") @(t : T?) {
        var vec = int3(1, 2, 3)
        var pvec = safe_addr_val(vec)
        var dummy = 0
        pvec?[3] ?? dummy = 1
        t |> equal(dummy, 1)
        vec[0] = 0
        pvec?[0] ?? dummy = 2
        t |> equal(vec[0], 2)
        t |> equal(dummy, 1)
        pvec = null
        pvec?[0] ?? dummy = 3
        t |> equal(dummy, 3)
    }
}

// Tests with uint index type
[test]
def test_safe_index_uint(t : T?) {
    t |> run("dynamic array safe index uint") @(t : T?) {
        var ar : array<int>
        var dummy = 0
        ar?[0u] ?? dummy = 1
        t |> equal(dummy, 1)
        resize(ar, 2)
        ar?[0u] ?? dummy = 10
        t |> equal(ar[0], 10)
        ar?[1u] ?? dummy = 20
        t |> equal(ar[1], 20)
        ar?[2u] ?? dummy = 99
        t |> equal(dummy, 99)
    }
    t |> run("fixed array safe index uint") @(t : T?) {
        var di : int[3]
        var dummy = 0
        di?[0u] ?? dummy = 10
        t |> equal(di[0], 10)
        di?[2u] ?? dummy = 30
        t |> equal(di[2], 30)
        di?[3u] ?? dummy = 99
        t |> equal(dummy, 99)
    }
    t |> run("vector safe index uint") @(t : T?) {
        var vec = int3(1, 2, 3)
        var dummy = 0
        vec?[0u] ?? dummy = 10
        t |> equal(vec[0], 10)
        vec?[2u] ?? dummy = 30
        t |> equal(vec[2], 30)
        vec?[3u] ?? dummy = 99
        t |> equal(dummy, 99)
    }
    t |> run("array pointer safe index uint") @(t : T?) {
        var ar : array<int>
        var par = safe_addr(ar)
        var dummy = 0
        par?[0u] ?? dummy = 1
        t |> equal(dummy, 1)
        resize(ar, 1)
        par?[0u] ?? dummy = 2
        t |> equal(ar[0], 2)
        par = null
        par?[0u] ?? dummy = 3
        t |> equal(dummy, 3)
    }
    t |> run("fixed array pointer safe index uint") @(t : T?) {
        var di : int[2]
        var pdi = safe_addr(di)
        var dummy = 0
        pdi?[3u] ?? dummy = 1
        t |> equal(dummy, 1)
        di[0] = 0
        pdi?[0u] ?? dummy = 2
        t |> equal(di[0], 2)
        pdi = null
        pdi?[0u] ?? dummy = 3
        t |> equal(dummy, 3)
    }
    t |> run("vector pointer safe index uint") @(t : T?) {
        var vec = int3(1, 2, 3)
        var pvec = safe_addr_val(vec)
        var dummy = 0
        pvec?[3u] ?? dummy = 1
        t |> equal(dummy, 1)
        vec[0] = 0
        pvec?[0u] ?? dummy = 2
        t |> equal(vec[0], 2)
        pvec = null
        pvec?[0u] ?? dummy = 3
        t |> equal(dummy, 3)
    }
}

// Tests for read-only safe index (no write through ?[])
[test]
def test_safe_index_read(t : T?) {
    t |> run("array safe index read") @(t : T?) {
        var ar <- [1, 2, 3]
        var val = ar?[1] ?? 0
        t |> equal(val, 2)
        val = ar?[5] ?? 42
        t |> equal(val, 42)
    }
    t |> run("fixed array safe index read") @(t : T?) {
        var di = fixed_array(10, 20, 30)
        var val = di?[1] ?? 0
        t |> equal(val, 20)
        val = di?[5] ?? 42
        t |> equal(val, 42)
    }
    t |> run("vector safe index read") @(t : T?) {
        var vec = int3(10, 20, 30)
        var val = vec?[1] ?? 0
        t |> equal(val, 20)
        val = vec?[3] ?? 42
        t |> equal(val, 42)
    }
    t |> run("table safe index read") @(t : T?) {
        var ta <- {"a" => 1, "b" => 2}
        var val = ta?["a"] ?? 0
        t |> equal(val, 1)
        val = ta?["c"] ?? 42
        t |> equal(val, 42)
    }
}
