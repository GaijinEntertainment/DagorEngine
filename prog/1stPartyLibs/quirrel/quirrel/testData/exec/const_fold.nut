// Test AST-level constant folding
// These expressions are fully const-evaluable and should be folded
// to a single LOAD instruction during codegen.

// Basic arithmetic
assert(2 + 3 == 5)
assert(10 - 3 == 7)
assert(4 * 5 == 20)
assert(15 / 3 == 5)
assert(17 % 5 == 2)

// Nested arithmetic
assert((2 + 3) * (4 + 1) == 25)
assert(1 + 2 + 3 + 4 + 5 == 15)
assert(100 - 10 * 3 - 20 / 4 == 65)

// String concatenation (NOT handled by peephole optimizer)
assert("hello" + " " + "world" == "hello world")
assert("abc" + "def" == "abcdef")

// Comparison operators
assert(3 > 2 == true)
assert(3 < 2 == false)
assert(3 >= 3 == true)
assert(3 <= 2 == false)
assert(3 == 3)
assert(3 != 4)

// Ternary with const condition
assert((true ? "yes" : "no") == "yes")
assert((false ? "yes" : "no") == "no")
assert((1 > 0 ? 42 : 0) == 42)

// Bitwise
assert((0xFF & 0x0F) == 15)
assert((1 << 4) == 16)
assert((0xFF | 0x100) == 0x1FF)
assert((0xFF ^ 0x0F) == 0xF0)
assert((16 >> 2) == 4)
assert((16 >>> 2) == 4)

// Logical
assert((true || false) == true)
assert((true && false) == false)
assert((false || true) == true)
assert((false && true) == false)

// Null coalescing
assert((null ?? "default") == "default")
assert(("value" ?? "default") == "value")

// typeof
assert(typeof 42 == "integer")
assert(typeof "hello" == "string")
assert(typeof 1.0 == "float")
assert(typeof true == "bool")

// Negation and bitwise not
assert(-(-5) == 5)
assert(!false == true)
assert(!true == false)
assert(~0 == -1)
assert(~0xFF == -256)

// Const identifiers in expressions
const A = 10
const B = 20
assert(A + B == 30)
assert(A * B + 5 == 205)
assert(A > B == false)
assert(A < B == true)

// Enum values in expressions
enum Color { RED = 1, GREEN = 2, BLUE = 4 }
assert((Color.RED | Color.GREEN | Color.BLUE) == 7)
assert(Color.RED + Color.GREEN == 3)

// Mixed const and enum
const MASK = 0xFF
assert((Color.BLUE & MASK) == 4)

// Complex nested const expressions
assert((A + B) * 2 - A == 50)
assert((A > 5 ? A : B) == 10)
assert((A < 5 ? A : B) == 20)

println("ok")
