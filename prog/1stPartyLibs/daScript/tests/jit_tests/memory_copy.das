options gen2
require dastest/testing_boost

// Exported wrappers keep every length spelling observable in generated IR.
[export] def copy_i32(var dst : void ?; src : void ?; size : int){unsafe(memcpy(dst, src, size))}
[export] def copy_u32(var dst : void ?; src : void ?; size : uint){unsafe(memcpy(dst, src, size))}
[export] def copy_i64(var dst : void ?; src : void ?; size : int64){unsafe(memcpy(dst, src, size))}
[export] def copy_u64(var dst : void ?; src : void ?; size : uint64){unsafe(memcpy(dst, src, size))}
[export] def move_i32(var dst : void ?; src : void ?; size : int){unsafe(memmove(dst, src, size))}
[export] def move_u32(var dst : void ?; src : void ?; size : uint){unsafe(memmove(dst, src, size))}
[export] def move_i64(var dst : void ?; src : void ?; size : int64){unsafe(memmove(dst, src, size))}
[export] def move_u64(var dst : void ?; src : void ?; size : uint64){unsafe(memmove(dst, src, size))}

def private check_transfers(t : T?; copy, move : auto(FT); zero : auto(N)){
    t |> success(!jit_enabled() || is_jit_function(copy))
    t |> success(!jit_enabled() || is_jit_function(move))
    invoke(copy, null, null, zero); invoke(move, null, null, zero)
    for (size in fixed_array(0, 1, 3, 7, 31, 63, 129)){
        var source : uint8[160]; var destination : uint8[160]
        for (i in range(160)){source[i] = uint8(i); destination[i] = 0xa5u8}
        invoke(copy, unsafe(addr(destination[3])), unsafe(addr(source[1])), N(size))
        for (i in range(160)){t |> equal(destination[i], i >= 3 && i < size + 3 ? uint8(i - 2) : 0xa5u8); t |> equal(source[i], uint8(i))}
        invoke(move, unsafe(addr(source[3])), unsafe(addr(source[0])), N(size))
        for (i in range(160)){t |> equal(source[i], uint8(i >= 3 && i < size + 3 ? i - 3 : i))}
        for (i in range(160)){source[i] = uint8(i)}
        invoke(move, unsafe(addr(source[0])), unsafe(addr(source[3])), N(size))
        for (i in range(160)){t |> equal(source[i], uint8(i < size ? i + 3 : i))}
        invoke(move, unsafe(addr(source[1])), unsafe(addr(source[1])), N(size))
        for (i in range(160)){t |> equal(source[i], uint8(i < size ? i + 3 : i))}
    }
}
[test]
def memory_copy_all_size_types(t : T?){
    check_transfers(t, @@copy_i32, @@move_i32, 0)
    check_transfers(t, @@copy_u32, @@move_u32, 0u)
    check_transfers(t, @@copy_i64, @@move_i64, 0l)
    check_transfers(t, @@copy_u64, @@move_u64, 0ul)
}
