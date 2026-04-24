// Valid syntax: Functions

// Basic function declaration
function add(a, b) {
    return a + b
}
assert(add(2, 3) == 5)

// local function
local function subtract(a, b) {
    return a - b
}
assert(subtract(10, 3) == 7)

// let function
let multiply = function(a, b) {
    return a * b
}
assert(multiply(3, 4) == 12)

// Named function expression
let factorial = function factorial(n) {
    if (n <= 1) return 1
    return n * factorial(n - 1)
}
assert(factorial(5) == 120)

// Lambda expressions
let square = @(x) x * x
assert(square(5) == 25)

let add_lambda = @(a, b) a + b
assert(add_lambda(2, 3) == 5)

// Lambda with function wrapper for multi-statement
let complex_fn = function(x) {
    local result = x * 2
    return result + 1
}
assert(complex_fn(5) == 11)

// Default parameters
function greet(name = "World") {
    return "Hello " + name
}
assert(greet() == "Hello World")
assert(greet("Quirrel") == "Hello Quirrel")

// Multiple default parameters
function multi_def(a, b = 10, c = 20) {
    return a + b + c
}
assert(multi_def(1) == 31)
assert(multi_def(1, 2) == 23)
assert(multi_def(1, 2, 3) == 6)

// Variable arguments
function sum_all(...) {
    local total = 0
    foreach (v in vargv)
        total += v
    return total
}
assert(sum_all(1, 2, 3, 4, 5) == 15)
assert(sum_all() == 0)

// Varargs with regular params
function first_and_rest(first, ...) {
    local rest_sum = 0
    foreach (v in vargv)
        rest_sum += v
    return [first, rest_sum]
}
let far = first_and_rest(10, 20, 30)
assert(far[0] == 10)
assert(far[1] == 50)

// Function returning function (closure)
function make_adder(n) {
    return @(x) x + n
}
let add5 = make_adder(5)
assert(add5(10) == 15)
assert(add5(20) == 25)

// Closures capture by reference
function make_counter() {
    local count = 0
    return {
        function inc() { count++ }
        function get() { return count }
    }
}
let counter = make_counter()
counter.inc()
counter.inc()
counter.inc()
assert(counter.get() == 3)

// Return without value
function no_return() {
    local x = 42
}
assert(no_return() == null)

// Explicit return null
function return_null() {
    return null
}
assert(return_null() == null)

// Function as value
let fn_array = [
    @(x) x + 1,
    @(x) x * 2,
    @(x) x - 3
]
assert(fn_array[0](5) == 6)
assert(fn_array[1](5) == 10)
assert(fn_array[2](5) == 2)

// Recursive function
function fib(n) {
    if (n <= 1) return n
    return fib(n - 1) + fib(n - 2)
}
assert(fib(10) == 55)

// Tail recursion
function tail_sum(n, acc = 0) {
    if (n <= 0) return acc
    return tail_sum(n - 1, acc + n)
}
assert(tail_sum(100) == 5050)

// Function with type annotations
function typed_add(x: int, y: int): int {
    return x + y
}
assert(typed_add(2, 3) == 5)

// Function with union type annotations
function nullable_fn(x: int|null = null) {
    return x ?? 0
}
assert(nullable_fn() == 0)
assert(nullable_fn(42) == 42)

// Lambda with default
let def_lambda = @(x = 10) x * 2
assert(def_lambda() == 20)
assert(def_lambda(5) == 10)

// callee()
let self_ref = function() {
    return callee()
}
assert(typeof self_ref() == "function")

// Function call/acall
function test_fn(a, b) { return a + b }
assert(test_fn.call(null, 3, 4) == 7)
assert(test_fn.acall([null, 3, 4]) == 7)

// bindenv
let obj = {name = "test"}
let bound = function() { return this.name }.bindenv(obj)
assert(bound() == "test")

print("functions: OK\n")
