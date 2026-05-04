options gen2

// Companion script for the "AOT" tutorial.
// This script is compiled, then AOT-generated C++ is linked in
// so that functions execute as native code instead of interpreted.

require math

[export]
def fibonacci(n : int) : int {
    if (n <= 1) {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

[export]
def test() {
    print("=== AOT Tutorial ===\n")
    // Compute Fibonacci numbers — with AOT, this runs as native C++ code
    for (i in range(10)) {
        print("fib({i}) = {fibonacci(i)}\n")
    }
    // Math functions are also AOT-accelerated
    let angle = 3.14159265 / 4.0
    print("\nsin(pi/4) = {sin(angle)}\n")
    print("cos(pi/4) = {cos(angle)}\n")
    print("sqrt(2)   = {sqrt(2.0)}\n")
}
