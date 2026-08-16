options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT017: int64/uint64 cast of a call that has a long_ counterpart
//
// Problem: int64(length(x)) widens a result that is already 32-bit, so the 2^31
// limit is hit inside length() before the cast ever runs — as a wrap for the
// unguarded pairs, or as a panic for array/table/string length, which carry an
// always-on guard. Either way the cast buys nothing; long_length(x) is 64-bit
// the whole way through.
//
// Bad pattern:
//   let n = int64(length(arr))
//
// Good pattern:
//   let n = long_length(arr)

expect 50503:5

require daslib/lint
require daslib/linq

// bad: int64 of array length (1 warning)
def bad_int64_length(arr : array<int>) : int64 {
    return int64(length(arr))
}

// bad: uint64 of array capacity (1 warning)
def bad_uint64_capacity(arr : array<int>) : uint64 {
    return uint64(capacity(arr))
}

// bad: int64 of string length (1 warning)
def bad_int64_string_length(s : string) : int64 {
    return int64(length(s))
}

// bad: int64 of table length (1 warning)
def bad_int64_table_length(t : table<string; int>) : int64 {
    return int64(length(t))
}

// bad: int64 of linq count (1 warning)
def bad_int64_count(arr : array<int>) : int64 {
    return int64(count(arr))
}

// good: 64-bit all the way through
def good_long_length(arr : array<int>) : int64 {
    return long_length(arr)
}

// good: a fixed array has no long_length twin, so the cast is the only spelling
def good_fixed_array(fa : int[4]) : int64 {
    return int64(length(fa))
}

// good: widening an ordinary int is not a truncation
def good_plain_widen(n : int) : int64 {
    return int64(n)
}
