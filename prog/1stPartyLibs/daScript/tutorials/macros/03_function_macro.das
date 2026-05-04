// Macro Tutorial 3: Function Macros
//
// This tutorial demonstrates function macros — compile-time
// transformations applied to annotated functions.  Three macros
// show the three main AstFunctionAnnotation methods:
//
//   [log_calls]      — apply(): rewrites the function body to add
//                      entry/exit logging with nested indentation
//
//   [expect_range]   — verifyCall(): validates every call site,
//                      rejecting constant arguments outside a range
//
//   [no_print]       — lint(): walks the fully-compiled body and
//                      rejects calls to the builtin print function
//
// Covers:
//   - [function_macro] annotation and AstFunctionAnnotation base class
//   - apply():  ExprStringBuilder, qmacro_block, $e(func.body), func.body |> move
//   - verifyCall():  AnnotationArgumentList, iValue, ExprConstInt, errors
//   - lint():  AstVisitor, make_visitor, visit(), expr.func._module.name
//   - Public variables for cross-module mutable state (LOG_DEPTH)
//
// The macros are defined in function_macro_mod.das (a separate module,
// because macros cannot be used in the module that defines them).
//
// Run: daslang.exe tutorials/macros/03_function_macro.das

options gen2

require function_macro_mod

// A simple function — demonstrates basic entry/exit logging
[log_calls]
def add(a, b : int) : int {
    return a + b
}

// A function with its own print — shows how the macro wraps around it
[log_calls]
def greet(name : string) {
    print("hello, {name}!\n")
}

// A recursive function — demonstrates nested call logging with indentation
[log_calls]
def fib(n : int) : int {
    if (n <= 1) {
        return n
    } else {
        return fib(n - 1) + fib(n - 2)
    }
}

// ── Part 2: [expect_range] — verifyCall validates call sites ──

// set_channel only accepts constant values in [0..255].
// Runtime (non-constant) values pass through — can't check at compile time.
[expect_range(value, min=0, max=255)]
def set_channel(name : string; value : int) {
    print("  {name} = {value}\n")
}

// ── Part 3: [no_print] — lint() walks the compiled body ──

// A pure computation — no print calls, so [no_print] is satisfied.
[no_print]
def compute(a, b : int) : int {
    return a * b + 1
}

// This function DOES call print — [no_print] would reject it.
// Try adding [no_print] to see the compile-time lint error.
// [no_print]
def bad_compute(a, b : int) : int {
    print("computing {a} * {b}\n")
    return a * b
}

[export]
def main() {
    // Part 1: [log_calls]
    let sum = add(2, 3)
    print("sum = {sum}\n")
    print("\n")

    greet("daslang")
    print("\n")

    let f = fib(3)
    print("fib(3) = {f}\n")
    print("\n")

    // Part 2: [expect_range]
    print("color channels:\n")
    set_channel("red", 128)
    set_channel("green", 0)
    set_channel("blue", 255)
    var alpha = 200           // runtime value — passes through unchecked
    set_channel("alpha", alpha)

    // Uncomment the line below to see a compile-time error:
    //   error: call annotated by expect_range failed
    //     value = 300 is out of range [0..255]
    // set_channel("red", 300)

    // Part 3: [no_print]
    let c = compute(3, 4)
    print("compute = {c}\n")

    // Uncomment [no_print] on bad_compute below to see a lint error:
    //   error: function bad_compute must not call builtin print
    // (The annotation is commented out so this file compiles.)
}

// output:
// >> add(2, 3)
// << add
// sum = 5
//
// >> greet(daslang)
// hello, daslang!
// << greet
//
// >> fib(3)
//   >> fib(2)
//     >> fib(1)
//     << fib
//     >> fib(0)
//     << fib
//   << fib
//   >> fib(1)
//   << fib
// << fib
// fib(3) = 2
//
// color channels:
//   red = 128
//   green = 0
//   blue = 255
//   alpha = 200
//
// compute = 13
