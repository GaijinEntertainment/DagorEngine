// Tutorial 5: Functions
//
// This tutorial covers:
//   - Defining functions with def
//   - Parameter types and return types
//   - Pass-by-value vs pass-by-reference
//   - Default arguments
//   - Function overloading
//   - Returning multiple values with tuples
//
// Run: daslang.exe tutorials/language/05_functions.das

options gen2

// === Basic function ===
// Parameters use comma to separate names that share a type,
// and semicolons to separate groups with different types (in type declarations).
// In practice, you can just use commas or semicolons for readability.

def greet(name : string) {
    print("Hello, {name}!\n")
}

// === Return type specified with : after the parameters ===

def add(a, b : int) : int {
    return a + b
}

def multiply(a, b : float) : float {
    return a * b
}

// === Pass-by-value vs pass-by-reference ===

// By default, parameters are passed by value (copied) and are immutable.
// Use & to pass by reference. Use 'var' to make it mutable.

def tryModify(x : int) {
    // x = 10         // ERROR: x is immutable (passed by value)
    print("  tryModify received: {x}\n")
}

def increment(var x : int&) {
    // var + & = mutable reference — changes are visible to the caller
    x++
    print("  increment changed x to: {x}\n")
}

// === Default arguments ===

def formatNumber(value : int; prefix : string = "#") : string {
    return "{prefix}{value}"
}

// === Function overloading ===
// Multiple functions with the same name but different parameter types.

def describe(x : int) {
    print("  integer: {x}\n")
}

def describe(x : float) {
    print("  float: {x}\n")
}

def describe(x : string) {
    print("  string: \"{x}\"\n")
}

// === Returning multiple values with tuples ===

def divmod(a, b : int) : tuple<quotient : int; remainder : int> {
    return (quotient = a / b, remainder = a % b)
}

// === Early return ===

def classifyAge(age : int) : string {
    if (age < 0) {
        return "invalid"
    }
    if (age < 13) {
        return "child"
    }
    if (age < 20) {
        return "teenager"
    }
    return "adult"
}

[export]
def main {

    // Call a simple function
    greet("World")
    greet("daslang")

    // Call a function with a return value
    print("add(3, 4) = {add(3, 4)}\n")
    print("multiply(2.5, 4.0) = {multiply(2.5, 4.0)}\n")

    // Pass by value — original not modified
    var val = 42
    tryModify(val)
    print("val after tryModify: {val}\n")

    // Pass by mutable reference — original IS modified
    increment(val)
    print("val after increment: {val}\n")

    // Default arguments — omit the ones with defaults
    print("formatNumber(42) = {formatNumber(42)}\n")
    // To specify a specific default, use named arguments: [name = value]
    let formatted = formatNumber(42, [prefix = ">"])
    print("formatNumber(42, prefix=\">\") = {formatted}\n")

    // Overloaded functions — compiler picks the right one by argument type
    print("describe overloads:\n")
    describe(42)
    describe(3.14)
    describe("hello")

    // Multiple return values via tuple
    let result = divmod(17, 5)
    print("17 / 5 = {result.quotient} remainder {result.remainder}\n")

    // You can also destructure the tuple
    let (q, r) = divmod(23, 7)
    print("23 / 7 = {q} remainder {r}\n")

    // Early return
    print("age 5: {classifyAge(5)}\n")
    print("age 15: {classifyAge(15)}\n")
    print("age 30: {classifyAge(30)}\n")
}

// output:
// Hello, World!
// Hello, daslang!
// add(3, 4) = 7
// multiply(2.5, 4.0) = 10
//   tryModify received: 42
// val after tryModify: 42
//   increment changed x to: 43
// val after increment: 43
// formatNumber(42) = #42
// formatNumber(42, prefix=">") = >42
// describe overloads:
//   integer: 42
//   float: 3.14
//   string: "hello"
// 17 / 5 = 3 remainder 2
// 23 / 7 = 3 remainder 2
// age 5: child
// age 15: teenager
// age 30: adult
