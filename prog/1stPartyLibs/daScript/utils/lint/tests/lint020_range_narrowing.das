options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT020: 64-bit bound narrowed into a 32-bit range constructor
//
// Problem: range(int(n)) truncates the bound above 2^31 before the loop even
// starts. range64 / urange64 take the 64-bit value directly, and arrays,
// fixed arrays, and (unsafe) pointers all index with int64/uint64 as-is.
//
// Bad pattern:
//   for (i in range(int(n64))) { ... }
//
// Good pattern:
//   for (i in range64(n64)) { ... }

expect 50503:4

require daslib/lint

// bad: 1-arg range with a narrowed int64 bound (1 warning)
def bad_range_1arg(arr : array<float>; n : int64) : float {
    var s = 0.0
    for (i in range(int(n))) {
        s += arr[i]
    }
    return s
}

// bad: 2-arg range, both bounds narrowed - one warning per constructor (1 warning)
def bad_range_2arg(lo, hi : int64) : int {
    var s = 0
    for (i in range(int(lo), int(hi))) {
        s += i
    }
    return s
}

// bad: urange with a narrowed uint64 bound (1 warning)
def bad_urange(n : uint64) : uint {
    var s = 0u
    for (i in urange(uint(n))) {
        s += i
    }
    return s
}

// bad: interval carries an int64 form, so the casts just truncate (1 warning)
def bad_interval(lo, hi : int64) : int {
    var s = 0
    for (i in interval(int(lo), int(hi))) {
        s += i
    }
    return s
}

// good: range64 takes the 64-bit bound directly
def good_range64(arr : array<float>; n : int64) : float {
    var s = 0.0
    for (i in range64(n)) {
        s += arr[i]
    }
    return s
}

// good: float operand - a genuine conversion, not a 64-bit truncation
def good_range_float(f : float) : int {
    var s = 0
    for (i in range(int(f))) {
        s += i
    }
    return s
}

// good: 32-bit bound needs no cast and gets none
def good_range_int(n : int) : int {
    var s = 0
    for (i in range(n)) {
        s += i
    }
    return s
}

// good: int(length(...)) is a 32-bit operand - not this rule's business
def good_range_length(arr : array<float>) : float {
    var s = 0.0
    for (i in range(int(length(arr)))) {
        s += arr[i] * float(i)
    }
    return s
}
