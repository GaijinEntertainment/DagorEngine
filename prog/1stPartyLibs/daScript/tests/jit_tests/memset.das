options gen2
require dastest/testing_boost

require daslib/constant_expression

def jit_memset8(data : void?; value : uint8; size : int) {
    unsafe {
        memset8(data, value, size)
    }
}

def jit_memset16(data : void?; value : uint16; size : int) {
    unsafe {
        memset16(data, value, size)
    }
}

def jit_memset32(data : void?; value : uint; size : int) {
    unsafe {
        memset32(data, value, size)
    }
}

def jit_memset64(data : void?; value : uint64; size : int) {
    unsafe {
        memset64(data, value, size)
    }
}

def jit_memset128(data : void?; value : uint4; size : int) {
    unsafe {
        memset128(data, value, size)
    }
}

[constant_expression(funcname), sideeffects]
def test_any_memset(t : T?; value : auto(TT); funcname : string) {
    for (i in 0..128) {
        for (j in i..128) {
            let count = j - i
            var arr : TT[128]
            unsafe {
                invoke(funcname, addr(arr[i]), value, count)
            }
            for (V in 0..128) {
                if (V >= i && V < j) {
                    t |> equal(value, arr[V])
                } else {
                    t |> equal(TT(), arr[V])
                }
            }
        }
    }
}

[test]
def test_memset(t : T?) {
    t |> run("memset8") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_memset8))
        var a : uint8[10]
        jit_memset8(unsafe(addr(a[2])), 0x55u8, 3)
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? 0x55u8 : 0u8, a[i])
        }
        test_any_memset(t, 12u8, "jit_memset8")
    }
    t |> run("memset16") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_memset16))
        var a : uint16[10]
        jit_memset16(unsafe(addr(a[2])), uint16(0x55), 3)
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint16(0x55) : uint16(0), a[i])
        }
        test_any_memset(t, uint16(1234), "jit_memset16")
    }
    t |> run("memset32") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_memset32))
        var a : uint[10]
        jit_memset32(unsafe(addr(a[2])), 0x55, 3)
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? 0x55 : 0x00, a[i])
        }
        test_any_memset(t, 0x12345678, "jit_memset32")
    }
    t |> run("memset64") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_memset64))
        var a : uint64[10]
        jit_memset64(unsafe(addr(a[2])), uint64(0x55), 3)
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint64(0x55) : uint64(0), a[i])
        }
        test_any_memset(t, 0x12345678abcdef01ul, "jit_memset64")
    }
    t |> run("memset128") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_memset128))
        var a : uint4[10]
        jit_memset128(unsafe(addr(a[2])), uint4(0x12, 0x34, 0x56, 0x78), 3)
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint4(0x12, 0x34, 0x56, 0x78) : uint4(0), a[i])
        }
        test_any_memset(t, uint4(0x12345678, 0x9abcdef0, 0x1ee7c00d, 0xdeadbeef), "jit_memset128")
    }
}