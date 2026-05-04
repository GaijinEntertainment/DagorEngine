// Valid syntax: All operators

// Arithmetic operators
let a = 10 + 5      // 15
let b = 10 - 5      // 5
let c = 10 * 5      // 50
let d = 10 / 5      // 2
let e = 10 % 3      // 1
assert(a == 15)
assert(b == 5)
assert(c == 50)
assert(d == 2)
assert(e == 1)

// Unary negation
let neg = -42
assert(neg == -42)

// Increment/decrement
local inc_val = 5
inc_val++
assert(inc_val == 6)
++inc_val
assert(inc_val == 7)
inc_val--
assert(inc_val == 6)
--inc_val
assert(inc_val == 5)

// Post-increment returns old value
local pv = 10
let old_val = pv++
assert(old_val == 10)
assert(pv == 11)

// Pre-increment returns new value
let new_val = ++pv
assert(new_val == 12)
assert(pv == 12)

// Comparison operators
assert(5 == 5)
assert(5 != 6)
assert(5 < 6)
assert(6 > 5)
assert(5 <= 5)
assert(5 <= 6)
assert(5 >= 5)
assert(6 >= 5)

// Three-way comparison
assert((5 <=> 6) < 0)
assert((5 <=> 5) == 0)
assert((6 <=> 5) > 0)

// Logical operators
assert(true && true)
assert(!(true && false))
assert(true || false)
assert(!(false || false))
assert(!false)
assert(!!true)

// Bitwise operators
assert((0xFF & 0x0F) == 0x0F)
assert((0xF0 | 0x0F) == 0xFF)
assert((0xFF ^ 0x0F) == 0xF0)
assert(~0 == -1)
assert((1 << 4) == 16)
assert((16 >> 2) == 4)
assert((16 >>> 2) == 4)

// Compound assignment
local ca = 10
ca += 5
assert(ca == 15)
ca -= 3
assert(ca == 12)
ca *= 2
assert(ca == 24)
ca /= 4
assert(ca == 6)
ca %= 4
assert(ca == 2)

// Ternary operator
let ter = true ? "yes" : "no"
assert(ter == "yes")
let ter2 = false ? "yes" : "no"
assert(ter2 == "no")

// typeof operator
assert(typeof 42 == "integer")
assert(typeof 3.14 == "float")
assert(typeof "str" == "string")
assert(typeof true == "bool")
assert(typeof null == "null")
assert(typeof [] == "array")
assert(typeof {} == "table")

// in operator
let tbl = {a = 1, b = 2}
assert("a" in tbl)
assert(!("c" in tbl))

// not in operator
assert("c" not in tbl)
assert(!("a" not in tbl))

// instanceof
class MyClass {}
let inst = MyClass()
assert(inst instanceof MyClass)

// Null-coalescing operator
let nc1 = null ?? "default"
assert(nc1 == "default")
let nc2 = "value" ?? "default"
assert(nc2 == "value")
let nc3 = null ?? null ?? "last"
assert(nc3 == "last")

// String concatenation with +
let str_cat = "hello" + " " + "world"
assert(str_cat == "hello world")

// delete operator (with allow directive)
#allow-delete-operator
local del_tbl = {a = 1, b = 2}
delete del_tbl.a
assert(!("a" in del_tbl))

// clone operator
let orig = {x = 1, y = 2}
let copy = clone orig
assert(copy.x == 1)
copy.x = 99
assert(orig.x == 1)

print("operators: OK\n")
