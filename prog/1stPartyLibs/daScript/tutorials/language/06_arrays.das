// Tutorial 6: Arrays
//
// This tutorial covers:
//   - Fixed-size arrays (value types, stack-allocated)
//   - Dynamic arrays (heap-allocated, must be moved)
//   - push, erase, length, resize
//   - Array iteration and indexed iteration
//   - Array comprehensions
//   - Move semantics with <-
//
// Run: daslang.exe tutorials/language/06_arrays.das

options gen2

[export]
def main {

    // === Fixed-size arrays ===
    // Fixed arrays are value types — they live on the stack and can be copied.

    var scores = fixed_array(10, 20, 30, 40, 50)
    print("scores: ")
    for (s in scores) {
        print("{s} ")
    }
    print("\n")

    // Access by index (0-based, bounds-checked)
    print("scores[0] = {scores[0]}\n")
    print("scores[4] = {scores[4]}\n")

    // Fixed arrays can be modified in place
    scores[2] = 99
    print("after scores[2]=99: {scores[2]}\n")

    // Declare with explicit type and size
    var grid : int[3]
    grid[0] = 1
    grid[1] = 2
    grid[2] = 3
    print("grid: {grid[0]} {grid[1]} {grid[2]}\n")

    // === Dynamic arrays ===
    // Dynamic arrays are heap-allocated and can grow.
    // They use move semantics (<-) — they cannot be copied with =.

    var numbers : array<int>
    push(numbers, 10)
    push(numbers, 20)
    push(numbers, 30)
    print("numbers: ")
    for (n in numbers) {
        print("{n} ")
    }
    print("\n")
    print("length = {length(numbers)}\n")

    // Pipe syntax: arr |> push(value) is equivalent to push(arr, value)
    numbers |> push(40)
    numbers |> push(50)
    print("after push 40, 50: length = {length(numbers)}\n")

    // Array literal — creates a dynamic array
    var fruits <- ["apple", "banana", "cherry"]
    print("fruits: ")
    for (f in fruits) {
        print("{f} ")
    }
    print("\n")

    // === erase — removes element at index ===
    erase(fruits, 1)    // remove "banana"
    print("after erase(1): ")
    for (f in fruits) {
        print("{f} ")
    }
    print("\n")

    // === Iteration with index using count() ===
    var colors <- ["red", "green", "blue"]
    for (i, color in count(), colors) {
        print("  [{i}] {color}\n")
    }

    // === Move semantics ===
    // Dynamic arrays cannot be copied — they must be moved with <-.
    // After a move, the source becomes empty.

    var source <- [1, 2, 3]
    var dest <- source                  // dest gets the data, source is now empty
    print("dest: ")
    for (d in dest) {
        print("{d} ")
    }
    print("\n")
    print("source length after move: {length(source)}\n")

    // === resize and clear ===
    var buf : array<int>
    resize(buf, 5)                      // 5 elements, all zero-initialized
    print("resized buf: ")
    for (v in buf) {
        print("{v} ")
    }
    print("\n")

    clear(buf)                          // removes all elements
    print("after clear: length = {length(buf)}\n")

    // === Array comprehension ===
    // Concise syntax to build arrays from expressions.
    // [for (var in range); expression]

    var squares <- [for (x in 0..6); x * x]
    print("squares: ")
    for (s in squares) {
        print("{s} ")
    }
    print("\n")

    // With a filter using 'where'
    var evens <- [for (x in 0..10); x; where x % 2 == 0]
    print("evens: ")
    for (e in evens) {
        print("{e} ")
    }
    print("\n")
}

// output:
// scores: 10 20 30 40 50
// scores[0] = 10
// scores[4] = 50
// after scores[2]=99: 99
// grid: 1 2 3
// numbers: 10 20 30
// length = 3
// after push 40, 50: length = 5
// fruits: apple banana cherry
// after erase(1): apple cherry
//   [0] red
//   [1] green
//   [2] blue
// dest: 1 2 3
// source length after move: 0
// resized buf: 0 0 0 0 0
// after clear: length = 0
// squares: 0 1 4 9 16 25
// evens: 0 2 4 6 8
