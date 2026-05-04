// Tutorial 37: Utility Patterns (defer + static_let)
//
// This tutorial covers:
//   - defer: Go-style scope-exit cleanup
//   - defer: ordering (LIFO — last deferred runs first)
//   - defer: inside conditionals and nested scopes
//   - static_let: persistent local variables (C-like static)
//   - static_let: with explicit names
//   - static_let_finalize: static that cleans up on shutdown
//   - Combining defer and static_let
//
// requires: daslib/defer, daslib/static_let
// Run: daslang.exe tutorials/language/37_utility_patterns.das

options gen2

require daslib/defer
require daslib/static_let

// === defer: scope-exit cleanup ===

// defer() moves a block of code to the finally section of the enclosing scope.
// It runs when the scope exits — whether normally or via an early return.
// This is Go's `defer` pattern: set up a resource, then immediately defer
// its cleanup, keeping the two related operations adjacent.

def defer_basics() {
    print("\n=== defer basics ===\n")

    // The deferred block runs AFTER the rest of the scope
    print("step 1\n")
    defer() {
        print("cleanup (deferred)\n")
    }
    print("step 2\n")
    print("step 3\n")
    // output: step 1, step 2, step 3, cleanup (deferred)
}

// === defer ordering (LIFO) ===

// When multiple defers exist in the same scope, they run in LIFO order
// (last deferred runs first), just like Go.

def defer_ordering() {
    print("\n=== defer ordering ===\n")

    defer() {
        print("first defer (runs last)\n")
    }
    defer() {
        print("second defer (runs first)\n")
    }
    print("main body\n")
}

// === defer with early return ===

// Deferred blocks run even when the function returns early.

def defer_early_return(do_early : bool) {
    print("\n=== defer early return (early={do_early}) ===\n")

    defer() {
        print("cleanup always runs\n")
    }

    if (do_early) {
        print("returning early\n")
        return
    }

    print("reached the end\n")
}

// === defer in conditional scopes ===

// defer attaches to the NEAREST enclosing scope — so inside an if block,
// it only runs when that if block exits (not the function scope).

def defer_in_condition() {
    print("\n=== defer in condition ===\n")

    print("before if\n")
    {
        defer() {
            print("deferred inside if\n")
        }
        print("inside if\n")
    }
    print("after if\n")
}

// === Practical defer: paired acquire/release ===

// The classic use case: acquire a resource, immediately defer its release.

var acquired_count : int = 0

def acquire_resource(name : string) {
    print("  acquired {name}\n")
    acquired_count ++
}

def release_resource(name : string) {
    print("  released {name}\n")
    acquired_count --
}

def defer_practical() {
    print("\n=== defer practical ===\n")

    acquire_resource("database")
    defer() {
        release_resource("database")
    }

    acquire_resource("file")
    defer() {
        release_resource("file")
    }

    print("  working with both resources...\n")
    // On scope exit: file released first (LIFO), then database
    print("  resources held: {acquired_count}\n")
}


// === static_let: persistent local variables ===

// static_let makes local variable declarations persistent across calls.
// The variable is initialized once and retains its value between calls,
// just like C's `static` local variables.

def call_counter() : int {
    static_let() {
        var count = 0
    }
    count ++
    return count
}

def static_let_basics() {
    print("\n=== static_let basics ===\n")

    // Each call increments the persistent counter
    print("call 1: {call_counter()}\n")
    print("call 2: {call_counter()}\n")
    print("call 3: {call_counter()}\n")
}

// === static_let with initialization ===

// Use static_let for expensive one-time setup: the initializer runs once.

def get_lookup_table() : int {
    static_let() {
        var lookup <- [10, 20, 30, 40, 50]
    }
    var total = 0
    for (v in lookup) {
        total += v
    }
    return total
}

def static_let_init() {
    print("\n=== static_let initialization ===\n")
    // The array is created once, reused on every call
    print("sum: {get_lookup_table()}\n")
    print("sum: {get_lookup_table()}\n")
}

// === static_let with named prefix ===

// The name argument helps disambiguate when you need multiple static_let
// blocks in different functions that might otherwise collide.

def named_counter_a() : int {
    static_let("counter_a") {
        var n = 0
    }
    n ++
    return n
}

def named_counter_b() : int {
    static_let("counter_b") {
        var n = 0
    }
    n ++
    return n
}

def static_let_named() {
    print("\n=== static_let named ===\n")

    named_counter_a()
    named_counter_a()
    print("counter_a: {named_counter_a()}\n")   // 3
    print("counter_b: {named_counter_b()}\n")   // 1
}

// === Combining defer and static_let ===

// A practical pattern: cache a result with static_let, and use defer
// to log when the function exits.
//
// Note: defer is moved to the scope's "finally" at compile time, so it
// ALWAYS runs on scope exit — regardless of whether execution "reached"
// the defer() call.  This differs from Go, where defer is runtime-conditional.

def cached_computation(input : int) : int {
    static_let() {
        var last_input = -1
        var last_result = 0
    }

    defer() {
        print("  exiting cached_computation({input})\n")
    }

    if (input == last_input) {
        print("  cache hit for {input}\n")
        return last_result
    }

    // "Expensive" computation
    last_result = input * input + input
    last_input = input
    print("  computed {input} -> {last_result}\n")
    return last_result
}

def combined_example() {
    print("\n=== combined defer + static_let ===\n")
    print("result: {cached_computation(5)}\n")
    print("result: {cached_computation(5)}\n")    // cache hit
    print("result: {cached_computation(10)}\n")
}

// === Summary ===
//
// +----------------+--------------------------------------------------------+
// | Feature        | Description                                            |
// +----------------+--------------------------------------------------------+
// | defer          | Moves block to scope's finally section (LIFO order)    |
// | static_let     | Promotes local declarations to persistent globals      |
// | static_let(n)  | Same, with a name prefix to avoid collisions           |
// | static_let_fin | Same as static_let, but deletes on context shutdown    |
// +----------------+--------------------------------------------------------+

[export]
def main() {
    defer_basics()
    defer_ordering()
    defer_early_return([do_early = true])
    defer_early_return([do_early = false])
    defer_in_condition()
    defer_practical()
    static_let_basics()
    static_let_init()
    static_let_named()
    combined_example()
    print("done\n")
}

// output:
//
// === defer basics ===
// step 1
// step 2
// step 3
// cleanup (deferred)
//
// === defer ordering ===
// main body
// second defer (runs first)
// first defer (runs last)
//
// === defer early return (early=true) ===
// returning early
// cleanup always runs
//
// === defer early return (early=false) ===
// reached the end
// cleanup always runs
//
// === defer in condition ===
// before if
// inside if
// deferred inside if
// after if
//
// === defer practical ===
//   acquired database
//   acquired file
//   working with both resources...
//   resources held: 2
//   released file
//   released database
//
// === static_let basics ===
// call 1: 1
// call 2: 2
// call 3: 3
//
// === static_let initialization ===
// sum: 150
// sum: 150
//
// === static_let named ===
// counter_a: 3
// counter_b: 1
//
// === combined defer + static_let ===
//   computed 5 -> 30
//   exiting cached_computation(5)
// result: 30
//   cache hit for 5
//   exiting cached_computation(5)
// result: 30
//   computed 10 -> 110
//   exiting cached_computation(10)
// result: 110
// done
