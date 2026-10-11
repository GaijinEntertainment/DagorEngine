options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT024: int64/uint64 cast over a 32-bit product
//
// `int64(w * h * 3)` computes the product in 32 bits and widens the WRAPPED result -
// the cast buys nothing, the overflow happened inside the parentheses. LINT017's
// sibling: there the inner CALL clamps at 2^31, here the inner ARITHMETIC wraps.
// Fix by widening a factor first: `int64(w) * int64(h) * 3l`.
//
// Fires when the cast operand is a +,-,* tree containing a product whose result type is
// still 32-bit AND one of the leaves is a call - `length(a)`, a dimension getter - the
// unbounded factor. A product of plain locals (`int64(qd * rv)`, kernel tile geometry)
// stays silent by design: plain-local products are dominated by tile geometry, calls by
// real byte counts.

expect 50503:5

require daslib/lint

// === Positives ===

def bad_length(a : array<int>) : int64 {
    return int64(length(a) * 4)      // LINT024
}

def bad_two_calls(a, b : array<int>) : int64 {
    return int64(length(a) * length(b))   // LINT024
}

def bad_sum_with_product(a : array<int>; pad : int) : int64 {
    return int64(length(a) * 4 + pad)     // LINT024 - the product wraps under the sum
}

def bad_minus(a : array<int>; pad : int) : int64 {
    return int64(length(a) * 4 - pad)     // LINT024 - and under a difference
}

def bad_uint(a : array<int>) : uint64 {
    return uint64(length(a) * 4)          // LINT024 - the uint64 arm
}

// === Negatives ===

def good_widened(a : array<int>) : int64 {
    return long_length(a) * 4l
}

def good_plain_locals(w, h : int) : int64 {
    return int64(w * h * 3)          // no call among the factors: silent by design
}

def good_sum(a, b : int) : int64 {
    return int64(a + b)              // no product: not this rule's shape
}

def good_call_no_product(a : array<int>; pad : int) : int64 {
    return int64(length(a) + pad)    // a call, but no product: silent
}

def good_const() : int64 {
    return int64(4 * 1024 * 1024)    // folded constant
}

def good_already64(w : int64; h : int) : int64 {
    return int64(w * int64(h))       // the operand is already 64-bit
}

def good_plain(w : int) : int64 {
    return int64(w)
}

[export]
def main() {
    let a <- [1, 2]
    print("{bad_length(a)} {bad_two_calls(a, a)} {bad_sum_with_product(a, 3)} {bad_minus(a, 1)} {bad_uint(a)}\n")
    print("{good_widened(a)} {good_plain_locals(2, 3)} {good_sum(1, 2)} {good_call_no_product(a, 1)} {good_const()} {good_already64(1l, 2)} {good_plain(1)}\n")
}
