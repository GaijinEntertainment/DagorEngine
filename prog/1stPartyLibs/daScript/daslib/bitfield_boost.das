options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module bitfield_boost shared public

//! Bitfield operator overloads.
//!
//! Adds index-style access ``bitfieldValue[fieldName]`` for reading and
//! writing individual bits of a bitfield value.

require daslib/contracts

[expect_any_bitfield(b)]
def operator [] (b : auto, i : int) : bool {
    //! get bitfield bit at index i
    return (uint64(b) & (1ul << uint64(i))) != 0ul
}

[expect_any_bitfield(b)]
def operator [] =(var b : auto(TT)&, i : int, v : bool) {
    //! set bitfield bit at index i to v
    let index = uint64(i)
    if (index >= uint64(typeinfo sizeof(b) << 3)) {
        panic("bitfield index {i} out of range")
    }
    if (v) {
        b = TT(uint64(b) | (1ul << index))
    } else {
        b = TT(uint64(b) & ~(1ul << index))
    }
}

[expect_any_bitfield(b)]
def operator [] &&=(var b : auto(TT)&, i : int, v : bool) {
    //! && assignment for bitfield bit at index i
    let index = uint64(i)
    if (index >= uint64(typeinfo sizeof(b) << 3)) {
        panic("bitfield index {i} out of range")
    }
    if (!v) {
        b = TT(uint64(b) & ~(1ul << index))
    }
}

[expect_any_bitfield(b)]
def operator [] ||=(var b : auto(TT)&, i : int, v : bool) {
    //! || assignment for bitfield bit at index i
    let index = uint64(i)
    if (index >= uint64(typeinfo sizeof(b) << 3)) {
        panic("bitfield index {i} out of range")
    }
    if (v) {
        b = TT(uint64(b) | (1ul << index))
    }
}

[expect_any_bitfield(b)]
def operator [] ^^=(var b : auto(TT)&, i : int, v : bool) {
    //! toggle bitfield bit at index i if v is true
    let index = uint64(i)
    if (index >= uint64(typeinfo sizeof(b) << 3)) {
        panic("bitfield index {i} out of range")
    }
    if (v) {
        b = TT(uint64(b) ^ (1ul << index))
    }
}

[expect_any_bitfield(b)]
def each_bit(b) : iterator<bool> {
    //! Iterates over each bit of a bitfield value, yielding true or false for each bit.
    let bits = uint64(b)
    return <- generator<bool>() <| $() {
        var mask = 1ul
        for (i in range(typeinfo sizeof(b) << 3)) {
            yield ((bits & mask) != 0ul)
            mask <<= 1ul
        }
        return false
    }
}
