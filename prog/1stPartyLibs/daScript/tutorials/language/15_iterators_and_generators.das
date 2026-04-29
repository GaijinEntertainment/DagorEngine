// Tutorial 15: Iterators and Generators
//
// This tutorial covers:
//   - What iterators are and how for loops use them
//   - Built-in iterators: range, each, keys, values, count
//   - Creating iterators from generators (yield)
//   - Generator control flow (loops, break, continue)
//   - Practical generator examples
//   - Making custom types iterable
//
// Run: daslang.exe tutorials/language/15_iterators_and_generators.das

options gen2

// === Custom struct with an each() function ===
// Any type can be made iterable by defining 'each' for it.

struct NumberRange {
    low : int
    high : int
}

def each(r : NumberRange) : iterator<int> {
    return <- generator<int>() <| $ {
        for (i in r.low .. r.high) {
            yield i
        }
        return false
    }
}

[export]
def main {

    // === Built-in iterators recap ===
    // We've used these in earlier tutorials. Quick recap:

    // range() and .. create integer sequences
    print("range: ")
    for (i in range(5)) {
        print("{i} ")
    }
    print("\n")

    // each() makes arrays iterable (arrays are iterable by default too)
    var arr <- [10, 20, 30]
    print("array: ")
    for (v in arr) {
        print("{v} ")
    }
    print("\n")

    // keys() and values() iterate over tables
    var scores <- { "math" => 95, "art" => 87 }
    for (k, v in keys(scores), values(scores)) {
        print("  {k}: {v}\n")
    }

    // === Generators ===

    // A generator creates an iterator using yield.
    // generator<ElementType>() <| $ { ... yield value ... return false }
    // Note: generators use either $ { } or $() { } (both are valid).

    // --- Simple generator: count up ---
    var counter <- generator<int>() <| $ {
        for (i in range(5)) {
            yield i
        }
        return false
    }
    print("generator count: ")
    for (v in counter) {
        print("{v} ")
    }
    print("\n")

    // --- Generator with filtering ---
    // Yield only even numbers from 0..9
    var evens <- generator<int>() <| $ {
        for (i in range(10)) {
            if (i % 2 == 0) {
                yield i
            }
        }
        return false
    }
    print("even generator: ")
    for (v in evens) {
        print("{v} ")
    }
    print("\n")

    // --- Fibonacci generator ---
    var fibs <- generator<int>() <| $ {
        var a = 0
        var b = 1
        while (a < 100) {
            yield a
            let next = a + b
            a = b
            b = next
        }
        return false
    }
    print("fibonacci: ")
    for (f in fibs) {
        print("{f} ")
    }
    print("\n")

    // --- Generator with state ---
    // Each generator maintains its own state between yields.
    var powers <- generator<int>() <| $ {
        var val = 1
        for (_ in range(8)) {
            yield val
            val *= 2
        }
        return false
    }
    print("powers of 2: ")
    for (p in powers) {
        print("{p} ")
    }
    print("\n")

    // === Making custom types iterable ===
    // The NumberRange struct has an each() function defined above.
    // This means we can iterate over it directly.

    var r = NumberRange(low = 3, high = 8)
    print("NumberRange(3,8): ")
    for (i in r) {
        print("{i} ")
    }
    print("\n")

    // === Generators are one-shot ===
    // Once a generator is exhausted, it produces no more values.
    // You need to create a new generator to iterate again.

    // === Generator vs comprehension ===
    // Comprehensions (tutorial 06) create arrays eagerly:
    //   var sq <- [for (x in 0..5); x*x]   // builds entire array
    //
    // Generators produce values lazily — one at a time.
    // Use generators when:
    //   - The sequence is large or infinite
    //   - You only need to iterate once
    //   - You want to avoid allocating an array
}

// output:
// range: 0 1 2 3 4
// array: 10 20 30
//   art: 87
//   math: 95
// generator count: 0 1 2 3 4
// even generator: 0 2 4 6 8
// fibonacci: 0 1 1 2 3 5 8 13 21 34 55 89
// powers of 2: 1 2 4 8 16 32 64 128
// NumberRange(3,8): 3 4 5 6 7
