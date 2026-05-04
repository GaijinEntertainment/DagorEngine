// Valid syntax: All literal types

// Integer literals
let int_dec = 42
let int_neg = -123
let int_zero = 0
let int_hex = 0xFF00A120
let int_hex_lower = 0xff
let int_char = 'w'
let int_char2 = 'A'
let int_sep = 123_456
let int_hex_sep = 0xAB_CD_01_23

// Float literals
let f1 = 1.52
let f2 = 1.0
let f3 = 0.234
let f4 = 6.5e-11
let f5 = 1.e2
let f6 = 12.345_67e+2
let f7 = 3.14159
let f8 = 1e10
let f9 = 2.5E3
let f10 = 0.0

// String literals
let s1 = "regular string"
let s2 = "escape sequences: \t \n \r \\ \" \' \0"
let s3 = "hex escape: \x41\x42"
let s4 = "unicode: \u0041\u0042"
let s5 = @"verbatim string with no escapes \n \t"
let s6 = @"verbatim
multiline
string"
let s7 = @"verbatim with ""doubled"" quotes"
let s8 = ""
let s9 = @""

// String interpolation
let x = 42
let interp1 = $"value is {x}"
let interp2 = $"calc: {x + 1}"
let interp3 = $"nested: {$"inner {x}"}"

// Boolean literals
let b_true = true
let b_false = false

// Null literal
let n = null

// Verify types
assert(typeof int_dec == "integer")
assert(typeof f1 == "float")
assert(typeof s1 == "string")
assert(typeof b_true == "bool")
assert(typeof n == "null")

print("literals: OK\n")
