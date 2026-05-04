options gen2

// Companion script for the "Callbacks" tutorial.
// The module "tutorial_07_cpp" provides C++ functions that accept
// blocks, function pointers, and lambdas as callbacks.

require tutorial_07_cpp

// A simple function that can be passed as a function pointer
def print_value(x : int) {
    print("  fn({x})\n")
}

[export]
def test() {
    // --- 1. Block callbacks ---
    print("=== Block: with_values ===\n")
    with_values(10, 20) $(a, b) {
        print("  a + b = {a + b}\n")
    }

    // --- 2. Predicate block ---
    print("\n=== Block: count_matching ===\n")
    var arr <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    let evens = count_matching(arr) $(x) {
        return x % 2 == 0
    }
    print("  even numbers: {evens}\n")

    // --- 3. Function pointer callback ---
    print("\n=== Function pointer: call_function_twice ===\n")
    call_function_twice(@@print_value, 5)

    // --- 4. Fibonacci iteration with block ---
    print("\n=== Practical: for_each_fibonacci ===\n")
    for_each_fibonacci(8) $(index, value) {
        print("  fib({index}) = {value}\n")
    }

    // --- 6. Reduce pattern ---
    print("\n=== Practical: reduce_range (sum 1..10) ===\n")
    let total = reduce_range(1, 11, 0) $(acc, i) {
        return acc + i
    }
    print("  sum = {total}\n")

    // --- 7. Reduce pattern: factorial ---
    print("\n=== Practical: reduce_range (factorial 10) ===\n")
    let fact = reduce_range(1, 11, 1) $(acc, i) {
        return acc * i
    }
    print("  10! = {fact}\n")

    delete arr
}
