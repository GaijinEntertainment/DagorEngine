// Valid syntax: Enums and constants

// Basic enum
enum Color {
    RED,
    GREEN,
    BLUE
}
assert(Color.RED == 0)
assert(Color.GREEN == 1)
assert(Color.BLUE == 2)

// Enum with explicit values
enum HttpCode {
    OK = 200,
    NOT_FOUND = 404,
    ERROR = 500
}
assert(HttpCode.OK == 200)
assert(HttpCode.NOT_FOUND == 404)

// Enum with mixed auto and explicit values
// Note: auto-increment always starts from 0 for each member without explicit value
enum Mixed {
    A,        // 0
    B = 10,
    C,        // 1 (resets from 0-based counter)
    D = 20
}
assert(Mixed.A == 0)
assert(Mixed.B == 10)
assert(Mixed.C == 1)
assert(Mixed.D == 20)

// Enum with string values
enum Names {
    FIRST = "first",
    SECOND = "second"
}
assert(Names.FIRST == "first")

// Enum with float values
enum Precision {
    LOW = 0.1,
    MEDIUM = 0.01,
    HIGH = 0.001
}

// const declaration
const MAX_SIZE = 100
const PI_APPROX = 3.14159
const GREETING = "hello"

assert(MAX_SIZE == 100)
assert(PI_APPROX == 3.14159)
assert(GREETING == "hello")

// Computed const
const DOUBLE_MAX = MAX_SIZE * 2
assert(DOUBLE_MAX == 200)

// global const
global const GLOBAL_VAL = 42

// global enum
global enum Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
}
assert(Direction.NORTH == 0)
assert(Direction.WEST == 3)

// Using enum values in expressions
let color = Color.RED
if (color == Color.RED) {
    assert(true)
}

// Enum in switch
#allow-switch-statement
local result = ""
let status = HttpCode.NOT_FOUND
switch (status) {
    case HttpCode.OK:
        result = "ok"
        break
    case HttpCode.NOT_FOUND:
        result = "not found"
        break
    case HttpCode.ERROR:
        result = "error"
        break
}
assert(result == "not found")

print("enums_consts: OK\n")
