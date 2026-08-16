options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// #3391 — indexing a raw pointer with int64/uint64 was AOT-ambiguous for the
// non-var pointer form (das_index<T * const> lacked 64-bit at/safe_at overloads).
// Non-var params cover the gap; var / const-pointee params are the already-working controls.

def read_i64(p : float?; i : int64) : float {
    return unsafe(p[i])
}

def read_u64(p : float?; i : uint64) : float {
    return unsafe(p[i])
}

def safe_read_i64(p : float?; i : int64) : float {
    return unsafe(p?[i]) ?? -1.0
}

def safe_read_u64(p : float?; i : uint64) : float {
    return unsafe(p?[i]) ?? -1.0
}

def write_i64(var p : float?; i : int64; v : float) {
    unsafe {
        p[i] = v
    }
}

def read_const_pointee_i64(p : float const?; i : int64) : float {
    return unsafe(p[i])
}

[test]
def test_int64_ptr_index(t : T?) {
    t |> run("int64/uint64 raw-pointer indexing") @(t : T?) {
        var buf : float[8]
        for (k in range(8)) {
            buf[k] = float(k)
        }
        var p = unsafe(addr(buf[0]))
        write_i64(p, int64(3), 30.0)
        t |> equal(read_i64(p, int64(3)), 30.0)
        t |> equal(read_u64(p, uint64(2)), 2.0)
        t |> equal(safe_read_i64(p, int64(1)), 1.0)
        t |> equal(safe_read_u64(p, uint64(4)), 4.0)
        t |> equal(read_const_pointee_i64(p, int64(5)), 5.0)
        let nullp : float? = null
        t |> equal(safe_read_i64(nullp, int64(0)), -1.0)
        t |> equal(safe_read_u64(nullp, uint64(0)), -1.0)
    }
}
