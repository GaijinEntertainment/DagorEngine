// Valid syntax: Miscellaneous language features
#allow-delete-operator

// Comments
// Single line comment
/* Block comment */
/* Multi
   line
   comment */

// __LINE__ and __FILE__
let line = __LINE__
assert(typeof line == "integer")
let file = __FILE__
assert(typeof file == "string")

// Semicolons (optional)
let a = 1; let b = 2; let c = 3;
assert(a + b + c == 6)

// Semicolons can be omitted
let d_val = 4
let e_val = 5
assert(d_val + e_val == 9)

// Empty blocks
if (true) {}
while (false) {}
for (;;) { break }

// Nested expressions
let complex = ((1 + 2) * (3 + 4)) / (5 - 2)
assert(complex == 7)

// Expression precedence
assert(2 + 3 * 4 == 14)
assert((2 + 3) * 4 == 20)

// Type conversion methods (use variables to avoid parse issues)
local iv = 42
assert(iv.tofloat() == 42.0)
local fv = 3.14
assert(fv.tointeger() == 3)
assert(iv.tostring() == "42")
assert(65 .tochar() == "A")

// assert with message
assert(true, "this should pass")

// assert with lazy message
assert(true, @() "lazy message")

// Weakref
let obj = {value = 42}
let weak = obj.weakref()
assert(typeof weak == "weakref")
assert(weak.ref().value == 42)

// Truthiness rules
assert(1)          // non-zero int is truthy
assert("hello")    // non-empty string is truthy
assert([])         // empty array is truthy
assert({})         // empty table is truthy
assert(!null)      // null is falsy
assert(!false)     // false is falsy

// Chained method calls
let result = [3, 1, 4, 1, 5].sort().reverse()
assert(result[0] == 5)

// Multiline expressions
let multi = 1 +
    2 +
    3
assert(multi == 6)

// Table as namespace
let Math = {
    function abs(x) { return x >= 0 ? x : -x }
    function max(a, b) { return a > b ? a : b }
    function min(a, b) { return a < b ? a : b }
}
assert(Math.abs(-5) == 5)
assert(Math.max(3, 7) == 7)
assert(Math.min(3, 7) == 3)

// Newthread
function thread_fn() {
    return 42
}
let t = newthread(thread_fn)
assert(typeof t == "thread")

// compilestring
let compiled = compilestring("return 42")
assert(compiled() == 42)

// Type method access with .$
let override_tbl = {len = @() 0}
assert(override_tbl.len() == 0)           // calls override
assert(override_tbl.$len() == 1)          // calls type method

// delete expression returns deleted value
local del_tbl = {a = 42, b = 99}
let deleted_val = delete del_tbl.a
assert(deleted_val == 42)
assert(!("a" in del_tbl))

print("misc: OK\n")
