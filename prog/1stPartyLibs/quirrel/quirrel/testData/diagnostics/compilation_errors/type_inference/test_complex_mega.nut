// EXPECTED: compile-time error on line with v_fail
// This test exercises many expression types in combination
//
// Type inference rules exercised:
//   - Literals: int, float, string, bool, null
//   - Arithmetic: int+int=int, float*float=float, int+float=int|float
//   - Comparison: always bool
//   - Typeof: always string
//   - Bitwise: always int
//   - Negation: ~int = int
//   - Logical not: !x = bool
//   - Ternary: union of branches
//   - Null-coalesce: (lhs minus null) | rhs
//   - Logical or/and: union of operands
//   - String concat: string + any = string
//   - Function call: return type of callee
//   - Table/array/class literals: table/array/class
//   - Lambda: function

const function [pure] sqri(x: int): int { return x * x }
const function [pure] sqrf(x: float): float { return x * x }
function fn(val): string|int { return val ? sqri(val) : "err" }
function get_flag(): bool { return true }
function maybe_int(): int|null { return 42 }
function make_table(): table { return { a = 1 } }
function make_array(): array { return [1, 2] }

// --- All of these should be OK ---
local ok_int: int = sqri(5)                                           // int
local ok_float: float = sqrf(3.14)                                    // float
local ok_union: string|int = fn(42)                                   // string|int
local ok_bool: bool = get_flag()                                      // bool
local ok_nullc: int = maybe_int() ?? 0                                // int
local ok_ternary: int|float = get_flag() ? 42 : 3.14                  // int|float
local ok_arith: int = (1 + 2) * 3 - 4 % 5                             // int
local ok_bitwise: int = (0xFF & 0x0F) | (0xAA ^ 0x55) << 2            // int
local ok_comp: bool = 42 > 10                                         // bool
local ok_typeof: string = typeof []                                   // string
local ok_not: bool = !get_flag()                                      // bool
local ok_table: table = { x = 1, y = "hello" }                        // table
local ok_array: array = [1, "two", 3.0]                               // array
local ok_class: class = class { val = 0 }                             // class
local ok_lambda: function = @(x) x * 2                                // function
local ok_concat: string = "value: " + 42                              // string (concat)
local ok_deep: string = get_flag() ? typeof 42 : "fallback"           // string|string = string
local ok_chain: int|string = get_flag() ? sqri(10) : fn(5)            // int | string|int = int|string
local ok_wide: int|float|string|null = get_flag() ? (maybe_int() ?? sqri(1)) : fn(2)

// --- This should FAIL ---
// sqrf(4.3) returns float, fn(3) returns string|int
// ternary: float | string|int = float|string|int
// declared type: function|null => mismatch!
local v_fail: function|null = get_flag() ? sqrf(4.3) : fn(3)

return [ok_int, ok_float, ok_union, ok_bool]
