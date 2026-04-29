// Test subtraction of -2147483648 (INT_MIN for 32-bit).
//
// The parser folds `-<integer>` into a single negative LiteralExpr.
// The codegen optimizes `x - literal` to `_OP_ADDI x, -literal` when the
// literal fits in 32 bits. But -(-2147483648) = +2147483648 overflows SQInt32,
// wrapping to -2147483648. This causes `x - (-2147483648)` to be miscompiled
// as `x + (-2147483648)` instead of `x + 2147483648`.
//
// Note: the unparenthesized form `x - -2147483648` is required to trigger the
// bug, because the parser folds the unary minus into the literal directly.
// With parens `x - (-2147483648)`, the RHS is a TO_PAREN node and the
// optimization path is not taken.

let x = 5

// Without parens: parser creates LiteralExpr(-2147483648) directly.
// Codegen sees TO_SUB with a TO_LITERAL RHS in 32-bit range, uses _OP_ADDI.
let result = x - -2147483648

// Expected: 5 - (-2147483648) = 5 + 2147483648 = 2147483653
assert(result == 2147483653)

// With a variable (non-optimized path for comparison):
local y = -2147483648
let result2 = x - y
assert(result2 == 2147483653)

// Both paths must agree.
assert(result == result2)
