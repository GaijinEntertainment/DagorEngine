options gen2
require dastest/testing_boost public

[test]
def test_memset(t : T?) {
    t |> run("memset8") @(t : T?) {
        var a : uint8[10]
        unsafe {
            memset8(addr(a[2]), 0x55u8, 3)
        }
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? 0x55u8 : 0u8, a[i])
        }
    }
    t |> run("memset16") @(t : T?) {
        var a : uint16[10]
        unsafe {
            memset16(addr(a[2]), uint16(0x55), 3)
        }
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint16(0x55) : uint16(0), a[i])
        }
    }
    t |> run("memset32") @(t : T?) {
        var a : uint[10]
        unsafe {
            memset32(addr(a[2]), 0x55, 3)
        }
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? 0x55 : 0x00, a[i])
        }
    }
    t |> run("memset64") @(t : T?) {
        var a : uint64[10]
        unsafe {
            memset64(addr(a[2]), uint64(0x55), 3)
        }
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint64(0x55) : uint64(0), a[i])
        }
    }
    t |> run("memset128") @(t : T?) {
        var a : uint4[10]
        unsafe {
            memset128(addr(a[2]), uint4(0x12, 0x34, 0x56, 0x78), 3)
        }
        for (i in 0..10) {
            t |> equal((i >= 2 && i < 5) ? uint4(0x12, 0x34, 0x56, 0x78) : uint4(0), a[i])
        }
    }
}

[test]
def test_memmove(t : T?) {
    t |> run("memmove overlapping ranges, every size spelling") @(t : T?) {
        // shifting a[0..6] right by 2 overlaps itself: memmove keeps the source bytes, unlike memcpy
        var a : uint8[10]
        for (i in 0..10) {
            a[i] = uint8(i)
        }
        unsafe {
            memmove(addr(a[2]), addr(a[0]), 6)
        }
        for (i in 0..10) {
            t |> equal(uint8(i >= 2 && i < 8 ? i - 2 : i), a[i])
        }
        unsafe {
            memmove(addr(a[0]), addr(a[2]), 6u)
        }
        for (i in 0..6) {
            t |> equal(uint8(i), a[i])
        }
        unsafe {
            memmove(addr(a[2]), addr(a[0]), 6l)
        }
        t |> equal(0u8, a[2])
        t |> equal(5u8, a[7])
        unsafe {
            memmove(addr(a[0]), addr(a[2]), 6ul)
        }
        for (i in 0..6) {
            t |> equal(uint8(i), a[i])
        }
    }
    t |> run("memmove between distinct buffers") @(t : T?) {
        var src : int[4]
        var dst : int[4]
        for (i in 0..4) {
            src[i] = i * 11
        }
        unsafe {
            memmove(addr(dst[0]), addr(src[0]), 4 * 4)
        }
        for (i in 0..4) {
            t |> equal(i * 11, dst[i])
        }
    }
}

