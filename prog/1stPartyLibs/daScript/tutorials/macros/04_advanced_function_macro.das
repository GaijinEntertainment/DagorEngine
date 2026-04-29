// Macro Tutorial 4: Advanced Function Macros
//
// This tutorial demonstrates [memoize] — a function macro that uses
// three AstFunctionAnnotation methods together:
//
//   apply()     — validates the function (no generics, no void, has args)
//   patch()     — generates cache table + wrapper function after inference
//   transform() — redirects all calls to the memoized wrapper
//
// The [memoize] macro caches function results in a private global
// table<uint64; ReturnType>.  On subsequent calls with the same
// arguments, the cached result is returned via clone instead of
// recomputing.  This is especially dramatic for recursive functions
// like Fibonacci, where exponential time becomes linear.
//
// Covers:
//   - apply() for pre-inference validation (isGeneric, isVoid, canClone)
//   - patch() with astChanged restart and "already processed" guard
//   - transform() for call-site redirection (clone + rename pattern)
//   - qmacro_function + add_function for generating companion functions
//   - add_global_var for creating private module-level cache variables
//   - hash() with XOR for multi-argument cache keys
//   - insert_clone for safe table insertion of cloneable values
//   - find_unique_function to prevent duplicate generation
//
// The macro is defined in advanced_function_macro_mod.das (a separate
// module, because macros cannot be used in the module that defines them).
//
// Run: daslang.exe tutorials/macros/04_advanced_function_macro.das

options gen2

require advanced_function_macro_mod

// ── Memoized recursive Fibonacci ──
//
// Without memoization, fib(40) would take billions of recursive calls.
// With [memoize], each unique argument is computed only once — the
// transform() method redirects recursive fib() calls to the memoized
// wrapper, so previously computed values are returned from the cache.

[memoize]
def fib(n : int) : int {
    if (n <= 1) {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

// ── Memoized multi-argument function ──
//
// Demonstrates hash XOR for multiple arguments.  The cache key is
// hash(a) ^ hash(b).

[memoize]
def slow_add(a, b : int) : int {
    // Imagine this is an expensive computation
    return a + b
}

// ── Memoized function returning a string ──
//
// Strings are cloneable, so [memoize] works.  The result is stored
// in the cache via insert_clone.

[memoize]
def greet(name : string) : string {
    return "hello, {name}!"
}

// ── Error examples (uncomment to see compile-time errors) ──

// Cannot memoize a void function — there is nothing to cache:
// [memoize]
// def fire(x : int) { print("fire {x}\n"); }

// Cannot memoize a function with no arguments:
// [memoize]
// def get_zero() : int { return 0; }

[export]
def main() {
    // Fibonacci: fib(30) would be ~1 billion calls without memoization
    print("fib(10) = {fib(10)}\n")
    print("fib(20) = {fib(20)}\n")
    print("fib(30) = {fib(30)}\n")

    // Multi-argument memoization
    print("slow_add(3, 4) = {slow_add(3, 4)}\n")
    print("slow_add(3, 4) = {slow_add(3, 4)}\n")  // cached

    // String result memoization
    print("{greet("daslang")}\n")
    print("{greet("daslang")}\n")  // cached
}

// output:
// fib(10) = 55
// fib(20) = 6765
// fib(30) = 832040
// slow_add(3, 4) = 7
// slow_add(3, 4) = 7
// hello, daslang!
// hello, daslang!
