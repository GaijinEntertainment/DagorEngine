// Tutorial 25: Annotations and Options
//
// This tutorial covers:
//   - Function annotations: [export], [sideeffects], private keyword
//   - Lifecycle annotations: [init], [finalize]
//   - Lint annotations: [unused_argument], [deprecated], [no_lint]
//   - Safety annotations: [unsafe_operation]
//   - Struct annotations: [safe_when_uninitialized]
//   - Combining multiple annotations
//   - Compiler options (lint, safety, optimization, memory)
//
// Run: daslang.exe tutorials/language/25_annotations_and_options.das

options gen2

// === Options overview ===
// Options are set at the top of a file:
//   options name = value
//   options gen2             // shorthand for = true
//   options a = true, b = false   // multiple options
//
// Common options:
//   gen2                         — enable gen2 syntax (braces, parens)
//   no_unused_function_arguments — unused func args become errors
//   no_unused_block_arguments    — unused block args become errors
//   strict_smart_pointers        — strict smart pointer checks (ON by default)
//   no_global_variables          — disallow module-level globals
//   stack = 16384                — stack size in bytes
//   no_aot                       — disable AOT for this module
//   lint                         — enable/disable all lint checks (default: true)
//   optimize                     — enable/disable optimizations (default: true)
//   no_deprecated                — deprecated usage becomes an error
//   unsafe_table_lookup          — tab[key] requires unsafe (default: true)

// === Private functions ===
// Use the `private` keyword — not callable from other modules.

def private helper() : string {
    return "I'm private"
}

// === [sideeffects] ===
// Tells the compiler this function has side effects.
// Without it, calls whose return values are unused may be eliminated.
// Required for functions that only do I/O (e.g. print).

[sideeffects]
def log_message(msg : string) {
    print("  LOG: {msg}\n")
}

// === [unused_argument] ===
// Suppresses the "unused argument" warning for specific args.
// Useful for callback signatures where some args are required but not used.

[unused_argument(y)]
def use_only_x(x : int; y : int) : int {
    return x * 2
}

// === [deprecated] ===
// Using a deprecated function produces a compile-time warning.
// (We won't call it here to avoid the warning in this tutorial.)

[deprecated(message="use new_greet instead")]
def old_greet(name : string) : string {
    return "Hi, {name}"
}

def new_greet(name : string) : string {
    return "Hello, {name}!"
}

// === [unsafe_operation] ===
// Calling this function requires an unsafe block.

[unsafe_operation]
def dangerous_compute(x : int) : int {
    return x * x  // pretend this does something unsafe
}

// === Struct annotations ===

// [safe_when_uninitialized] — the struct is safe even if zero-filled.
// Without it, structs with fields lacking initializers may cause errors.
[safe_when_uninitialized]
struct RawData {
    a : int
    b : float
    c : bool
}

// === Combining annotations ===
// Multiple annotations separated by commas.

[export, sideeffects]
def annotated_func() {
    print("  exported with sideeffects\n")
}

// === [init] — runs when the context initializes ===
// The function must take no arguments and return void.
// Multiple [init] functions run in unspecified order
// unless ordered with [init(after="tag")] or [init(before="tag")].

var initialized = false

[init]
def on_startup {
    initialized = true
}

// === Main ===

[export]
def main {

    // private — only callable within this module
    print("private function:\n")
    print("  helper() = {helper()}\n")

    // [sideeffects] — I/O function won't be eliminated
    print("sideeffects:\n")
    log_message("this call is preserved")

    // [unused_argument]
    print("unused_argument:\n")
    print("  use_only_x(5, 99) = {use_only_x(5, 99)}\n")

    // [unsafe_operation] — must be called inside unsafe
    print("unsafe_operation:\n")
    let result = unsafe(dangerous_compute(7))
    print("  dangerous_compute(7) = {result}\n")

    // [safe_when_uninitialized]
    print("safe_when_uninitialized:\n")
    var raw : RawData
    print("  RawData: a={raw.a}, b={raw.b}, c={raw.c}\n")

    // Combined annotations
    print("combined annotations:\n")
    annotated_func()

    // [init] — ran automatically before main
    print("init:\n")
    print("  initialized = {initialized}\n")

    // === Summary tables ===
    //
    // Function annotations:
    //   [export]          — callable from host application
    //   private keyword   — private to current module
    //   [sideeffects]     — has side effects, won't be eliminated
    //   [init]            — runs at context initialization
    //   [finalize]        — runs at context shutdown
    //   [run]             — runs at compile time
    //   [deprecated(message="...")]  — produces warning on use
    //   [unused_argument(x)]         — suppress unused arg lint
    //   [no_lint]         — disable all lint checks
    //   [nodiscard]       — warn if return value is ignored
    //   [unsafe_operation]  — calling requires unsafe block
    //   [no_aot]          — disable AOT for this function
    //   [jit]             — request JIT compilation
    //
    // Struct annotations:
    //   [safe_when_uninitialized]  — zero-filled is valid
    //   [cpp_layout]              — C++ struct alignment
    //   [persistent]              — survives context reset
    //
    // Key options:
    //   options gen2                          — gen2 syntax
    //   options no_unused_function_arguments  — unused args are errors
    //   options strict_smart_pointers         — strict smart_ptr (default: on)
    //   options no_global_variables           — disallow module globals
    //   options unsafe_table_lookup           — tab[key] needs unsafe
    //   options stack = 16384                 — stack size
    //   options no_aot                        — disable AOT for module
    //   options lint                          — enable/disable all lint
    //   options no_deprecated                 — deprecated usage is error

    print("done\n")
}

// output:
// private function:
//   helper() = I'm private
// sideeffects:
//   LOG: this call is preserved
// unused_argument:
//   use_only_x(5, 99) = 10
// unsafe_operation:
//   dangerous_compute(7) = 49
// safe_when_uninitialized:
//   RawData: a=0, b=0, c=false
// combined annotations:
//   exported with sideeffects
// init:
//   initialized = true
// done
