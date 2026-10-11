options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT017: int64/uint64 cast of a call that has a 64-bit counterpart
//
// Problem: int64(length(x)) widens a result that is already 32-bit, so the 2^31
// limit is hit inside length() before the cast ever runs — as a wrap for the
// unguarded pairs, or as a panic for array/table/string length, which carry an
// always-on guard. Either way the cast buys nothing; long_length(x) is 64-bit
// the whole way through. The same holds for a 32-bit string parse: to_int() and
// to_uint() answer 0 for any value that overflows 32 bits, so int64(to_int(s))
// asks for a range the parse never produced — to_int64(s, false) parses at 64
// bits.
//
// Bad pattern:
//   let n = int64(length(arr))
//   let m = int64(to_int(s))
//
// Good pattern:
//   let n = long_length(arr)
//   let m = to_int64(s, false)
//
// The parse remedy repeats the site's own hex argument and keeps a mixed-sign
// site's outer cast, so only the parse width changes:
//   int64(to_int(s, true))  -> to_int64(s, true)
//   int64(to_uint(s))       -> int64(to_uint64(s, false))

expect 50503:12

require strings
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

// bad: int64 of a 32-bit string parse (1 warning)
def bad_int64_to_int(s : string) : int64 {
    return int64(to_int(s))
}

// bad: uint64 of a 32-bit unsigned string parse (1 warning)
def bad_uint64_to_uint(s : string) : uint64 {
    return uint64(to_uint(s))
}

// bad: mixed signedness still widens a 32-bit parse (1 warning)
def bad_int64_to_uint(s : string) : int64 {
    return int64(to_uint(s))
}

// bad: mixed signedness the other way (1 warning)
def bad_uint64_to_int(s : string) : uint64 {
    return uint64(to_int(s))
}

// bad: a hex parse widens the same way, and the remedy stays hex (1 warning)
def bad_int64_to_int_hex(s : string) : int64 {
    return int64(to_int(s, true))
}

// bad: hex and mixed signedness at once (1 warning)
def bad_uint64_to_int_hex(s : string) : uint64 {
    return uint64(to_int(s, true))
}

// bad: the base is a runtime flag, so the remedy carries the flag (1 warning)
def bad_int64_to_int_var_hex(s : string; isHex : bool) : int64 {
    return int64(to_int(s, isHex))
}

struct Parsed {
    n : int
}

def to_int(p : Parsed) : int {
    return p.n
}

// good: 64-bit all the way through
def good_long_length(arr : array<int>) : int64 {
    return long_length(arr)
}

// good: the parse itself is 64-bit
def good_to_int64(s : string) : int64 {
    return to_int64(s, false)
}

// good: a float parse is not a 32-bit integer parse
def good_float_parse(s : string) : int64 {
    return int64(to_float(s))
}

// good: to_int over a non-string argument is not a string parse at all
def good_non_string_to_int(p : Parsed) : int64 {
    return int64(to_int(p))
}

// good: a fixed array has no long_length twin, so the cast is the only spelling
def good_fixed_array(fa : int[4]) : int64 {
    return int64(length(fa))
}

// good: widening an ordinary int is not a truncation
def good_plain_widen(n : int) : int64 {
    return int64(n)
}
