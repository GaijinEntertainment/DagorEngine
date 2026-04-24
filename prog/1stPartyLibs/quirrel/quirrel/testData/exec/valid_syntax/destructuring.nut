// Valid syntax: Destructuring

// Array destructuring
let [a, b, c] = [10, 20, 30]
assert(a == 10)
assert(b == 20)
assert(c == 30)

// Table/object destructuring
let {x, y} = {x = 100, y = 200}
assert(x == 100)
assert(y == 200)

// Destructuring with default values
let {p = 1, q = 2} = {p = 10}
assert(p == 10)
assert(q == 2)

// Destructuring in function parameters - table
function process_point({x, y}) {
    return x + y
}
assert(process_point({x = 3, y = 4}) == 7)

// Destructuring in function parameters - array
function process_pair([a, b]) {
    return a * b
}
assert(process_pair([3, 4]) == 12)

// Destructuring with type annotations
let {m: int, n: int} = {m = 5, n = 10}
assert(m == 5)
assert(n == 10)

// Function parameter destructuring with defaults
function with_defaults({name = "default", value = 0}) {
    return $"{name}={value}"
}
assert(with_defaults({name = "test", value = 42}) == "test=42")
assert(with_defaults({}) == "default=0")

// Multiple destructurings
let {r} = {r = 99}
assert(r == 99)

// Partial array destructuring
let [first, second] = [1, 2, 3, 4, 5]
assert(first == 1)
assert(second == 2)

// Table destructuring with multiple fields
let {name, age, active} = {name = "John", age = 30, active = true}
assert(name == "John")
assert(age == 30)
assert(active == true)

print("destructuring: OK\n")
