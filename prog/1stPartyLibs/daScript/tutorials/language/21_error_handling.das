// Tutorial 21: Error Handling
//
// This tutorial covers:
//   - panic() for triggering runtime errors
//   - try/recover for catching panics
//   - assert() — debug-only checks (may be removed in release)
//   - verify() — always checked, even in release
//   - static_assert() — compile-time assertions
//   - concept_assert() — compile-time, reported at call site
//   - finally blocks for cleanup
//   - defer() for Go-style cleanup (LIFO order)
//
// Run: daslang.exe tutorials/language/21_error_handling.das

options gen2

require daslib/defer

// === assert and verify ===
// assert checks a condition — may be removed in release builds.
// NEVER put side effects in assert!

def check_positive(x : int) {
    assert(x >= 0, "x must be non-negative")
}

// verify is NEVER removed — side effects are safe here.

def safe_divide(a, b : float) : float {
    verify(b != 0.0, "division by zero")
    return a / b
}

// === try/recover catches panics ===
// This is NOT general exception handling. It catches:
//   - null pointer dereference
//   - out-of-bounds access
//   - explicit panic() calls
// State may be inconsistent after recovery.

def risky_operation(do_panic : bool) : string {
    var result = "ok"
    try {
        if (do_panic) {
            panic("something went wrong")
        }
    } recover {
        result = "recovered from panic"
    }
    return result
}

// === concept_assert: compile-time checks at call site ===
// Used in generic functions to enforce type constraints.
// The error is reported where the function is CALLED, not defined.

def sum_numbers(a, b : auto(T)) : T {
    concept_assert(typeinfo is_numeric(type<T>), "sum_numbers requires numeric types")
    return a + b
}

// === finally blocks ===
// Runs when a block exits, regardless of how (return, panic, etc.)

def process_with_cleanup() {
    var counter = 0
    for (i in range(3)) {
        counter += 1
    } finally {
        print("  loop cleanup, counter={counter}\n")
    }
    // finally can attach to any block: functions, loops, if blocks
}

// === defer: Go-style cleanup ===
// Moves code to the finally section of the enclosing scope.
// Multiple defers execute in LIFO (last-in, first-out) order.

def defer_example() {
    print("  step 1: acquire resources\n")

    defer() {
        print("  step 4: cleanup A (deferred first)\n")
    }

    print("  step 2: do work\n")

    defer() {
        print("  step 3: cleanup B (deferred second, runs first)\n")
    }

    // When this function returns, deferred blocks run in reverse order
}

[export]
def main {

    // === assert ===
    print("assert:\n")
    check_positive(42)
    print("  check_positive(42) passed\n")
    // check_positive(-1)  // would trigger assert in debug builds

    // === verify ===
    print("verify:\n")
    print("  10 / 3 = {safe_divide(10.0, 3.0)}\n")

    // === try/recover ===
    print("try/recover:\n")
    print("  no panic: {risky_operation(false)}\n")
    print("  panic:    {risky_operation(true)}\n")

    // Catching null pointer dereference
    try {
        var p : int?
        print("  {*p}\n")
    } recover {
        print("  caught null pointer dereference\n")
    }

    // Catching out-of-bounds access
    try {
        var arr <- [1, 2, 3]
        print("  {arr[100]}\n")
    } recover {
        print("  caught out-of-bounds access\n")
    }

    // === concept_assert ===
    print("concept_assert:\n")
    print("  sum_numbers(3, 4) = {sum_numbers(3, 4)}\n")
    print("  sum_numbers(1.5, 2.5) = {sum_numbers(1.5, 2.5)}\n")
    // sum_numbers("a", "b")  // compile error: "sum_numbers requires numeric types"

    // === finally ===
    print("finally:\n")
    process_with_cleanup()

    // === defer ===
    print("defer:\n")
    defer_example()

    // === Summary ===
    // - panic("msg")    — trigger runtime panic
    // - try { } recover { } — catch panics (NOT general exceptions)
    // - assert(cond)    — debug check (may be stripped in release)
    // - verify(cond)    — always checked, side effects allowed
    // - static_assert() — compile-time assertion
    // - concept_assert() — compile-time, reported at call site
    // - finally { }     — runs on scope exit
    // - defer() { } — LIFO cleanup (from daslib/defer)
}

// output:
// assert:
//   check_positive(42) passed
// verify:
//   10 / 3 = 3.3333333
// try/recover:
//   no panic: ok
//   panic:    recovered from panic
//   caught null pointer dereference
//   caught out-of-bounds access
// concept_assert:
//   sum_numbers(3, 4) = 7
//   sum_numbers(1.5, 2.5) = 4
// finally:
//   loop cleanup, counter=3
// defer:
//   step 1: acquire resources
//   step 2: do work
//   step 3: cleanup B (deferred second, runs first)
//   step 4: cleanup A (deferred first)
