// Tutorial 13: Blocks
//
// This tutorial covers:
//   - Block type: nameless functions that capture by reference
//   - Declaring blocks with $() syntax
//   - Pipe syntax with <|
//   - Blocks as callback parameters
//   - Call syntax: action() and transform(value)
//   - Simplified => syntax for single-expression blocks
//   - Local block variables and invoke()
//
// Run: daslang.exe tutorials/language/13_blocks.das

options gen2

// === Functions that accept block parameters ===

// A function that takes a block<():void> — no args, no return.
def do_twice(action : block) {
    // Call blocks directly — just like regular functions!
    action()
    action()
}

// A function that takes a block with parameters and a return value.
def apply_to(value : int; transform : block<(x : int) : int>) : int {
    return transform(value)    // call syntax, not invoke
}

// Filter elements using a predicate block
def filter_array(arr : array<int>; predicate : block<(x : int) : bool>) {
    for (v in arr) {
        if (predicate(v)) {
            print("{v} ")
        }
    }
    print("\n")
}

// Iterate with index
def each_indexed(arr : array<int>; action : block<(idx : int; val : int) : void>) {
    for (i, v in count(), arr) {
        action(i, v)
    }
}

[export]
def main {

    // === Basic block with pipe syntax ===
    // The <| operator passes a block as the last argument.
    // $() declares a block with no parameters.
    print("do_twice:\n")
    do_twice() {
        print("  hello!\n")
    }

    // === Block with parameters ===
    // $(x) declares a block parameter. Types are inferred from the function signature.
    let result = apply_to(10) $(x) {
        return x * x
    }
    print("apply_to(10, x*x) = {result}\n")

    // === Simplified => syntax ===
    // When a block is a single return expression, use => instead of { return ... }
    let doubled = apply_to(7) <| $(x) => x * 2
    print("apply_to(7, x*2) = {doubled}\n")

    // === Inline block syntax ===
    // Blocks can also be passed inline as a regular argument.
    let tripled = apply_to(5, $(x) => x * 3)
    print("apply_to(5, x*3) = {tripled}\n")

    // === Blocks capture local variables by REFERENCE ===
    // No copy is made — the block sees the original variables.
    var total = 0
    var data <- [10, 20, 30, 40, 50]
    each_indexed(data) $(idx, val) {
        total += val
        print("  [{idx}] = {val}\n")
    }
    print("total = {total}\n")

    // === Filtering with blocks ===
    var numbers <- [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    print("even: ")
    filter_array(numbers) <| $(x) => x % 2 == 0
    print("greater than 5: ")
    filter_array(numbers) <| $(x) => x > 5

    // === Blocks cannot be stored or returned ===
    // Blocks live on the stack — they can only be passed DOWN as arguments.
    // They cannot be stored in arrays, tables, or other containers.
    // This makes them very fast (no heap allocation).
    // If you need to store or return a callable, use a lambda (tutorial 14).

    // === Local block variables and invoke() ===
    // You CAN assign a block to a local variable.
    // Local block variables require invoke() to call:
    var blk = $(a, b : int) => a + b
    print("blk: {invoke(blk, 3, 4)}\n")
    // Inside a function that receives a block parameter, prefer the direct
    // call syntax: action(), transform(value), predicate(x).
    // invoke() is the explicit form, useful for local block variables.

    // === Practical example: find_if ===
    var names <- ["Alice", "Bob", "Carol", "Dave"]
    var found = ""
    find_first(names) $(name) {
        if (name == "Carol") {
            found = name
            return true
        }
        return false
    }
    print("found: {found}\n")

    // === Nested blocks ===
    var sum = 0
    do_twice() {
        do_twice() {
            sum++
        }
    }
    print("nested sum = {sum}\n")    // 2 * 2 = 4
}

// A helper that iterates until the block returns true
def find_first(arr : array<string>; predicate : block<(s : string) : bool>) {
    for (s in arr) {
        if (predicate(s)) {
            return
        }
    }
}

// output:
// do_twice:
//   hello!
//   hello!
// apply_to(10, x*x) = 100
// apply_to(7, x*2) = 14
// apply_to(5, x*3) = 15
//   [0] = 10
//   [1] = 20
//   [2] = 30
//   [3] = 40
//   [4] = 50
// total = 150
// even: 2 4 6 8 10
// greater than 5: 6 7 8 9 10
// blk: 7
// found: Carol
// nested sum = 4
