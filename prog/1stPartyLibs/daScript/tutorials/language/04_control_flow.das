// Tutorial 4: Control Flow
//
// This tutorial covers:
//   - if / elif / else
//   - while loops
//   - for loops with range and ..
//   - break and continue
//   - Multiple iterators in for
//
// Run: daslang.exe tutorials/language/04_control_flow.das

options gen2

[export]
def main {

    // === if / elif / else ===
    // Parentheses are required around the condition in gen2 syntax.
    // Braces are required around the body.

    let temperature = 25
    if (temperature > 30) {
        print("It's hot!\n")
    } elif (temperature > 20) {
        print("It's pleasant.\n")
    } elif (temperature > 10) {
        print("It's cool.\n")
    } else {
        print("It's cold!\n")
    }

    // === while loop ===

    var count = 5
    while (count > 0) {
        print("countdown: {count}\n")
        count--
    }
    print("liftoff!\n")

    // === for loop with range ===

    // range(start, end) iterates from start to end-1 (exclusive end).
    print("range(0,5): ")
    for (i in range(0, 5)) {
        print("{i} ")
    }
    print("\n")

    // The .. operator creates a range: a..b is the same as range(a, b).
    print("0..5: ")
    for (i in 0..5) {
        print("{i} ")
    }
    print("\n")

    // Just range(n) starts from 0.
    print("range(4): ")
    for (i in range(4)) {
        print("{i} ")
    }
    print("\n")

    // === Nested loops ===

    print("multiplication table:\n")
    for (row in 1..4) {
        for (col in 1..4) {
            let product = row * col
            print("{product}\t")
        }
        print("\n")
    }

    // === break ===
    // Exits the innermost loop immediately.

    print("break at 3: ")
    for (i in 0..10) {
        if (i == 3) {
            break
        }
        print("{i} ")
    }
    print("\n")

    // === continue ===
    // Skips to the next iteration.

    print("skip odds: ")
    for (i in 0..8) {
        if (i % 2 != 0) {
            continue
        }
        print("{i} ")
    }
    print("\n")

    // === Multiple iterators ===
    // for can iterate over multiple sequences in parallel.
    // The loop stops when the shortest sequence ends.

    let names = fixed_array("Alice", "Bob", "Carol")
    let scores = fixed_array(95, 87, 92)
    for (name, score in names, scores) {
        print("{name}: {score}\n")
    }

    // === for with index using count() ===
    // count() is a built-in infinite iterator: count(start, step).
    // Pair it with a finite sequence to get an index.

    let fruits = fixed_array("apple", "banana", "cherry")
    for (idx, fruit in count(), fruits) {
        print("  [{idx}] {fruit}\n")
    }
}

// output:
// It's pleasant.
// countdown: 5
// countdown: 4
// countdown: 3
// countdown: 2
// countdown: 1
// liftoff!
// range(0,5): 0 1 2 3 4
// 0..5: 0 1 2 3 4
// range(4): 0 1 2 3
// multiplication table:
// 1    2    3
// 2    4    6
// 3    6    9
// break at 3: 0 1 2
// skip odds: 0 2 4 6
// Alice: 95
// Bob: 87
// Carol: 92
//   [0] apple
//   [1] banana
//   [2] cherry
