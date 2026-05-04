// Macro Tutorial 1: Call Macros
//
// This tutorial teaches how to write call macros — compile-time
// transformations that intercept function-call syntax and replace
// it with arbitrary AST.
//
// Covers:
//   - [call_macro] annotation and AstCallMacro base class
//   - The visit() method: inspecting arguments, returning AST
//   - macro_verify for compile-time error reporting
//   - ExprStringBuilder for building string interpolation AST
//   - qmacro and $e() for AST reification
//   - clone_expression for argument reuse
//
// The macros are defined in call_macro_mod.das (a separate module,
// because macros cannot be used in the module that defines them).
//
// Run: daslang.exe tutorials/macros/01_call_macro.das

options gen2

require call_macro_mod

[export]
def main() {

    // Section 1: hello() — simplest call macro
    // The macro replaces hello() with print("hello, call macro!\n")
    hello()

    // Section 2: greet("name") — macro with argument validation
    // The macro reads the string argument and builds:
    //   print("hello, {name}!\n")
    greet("world")
    greet("daslang")

    // Section 3: printf(fmt, args...) — format string macro
    // (N) in the format string refers to the N-th argument (1-based).
    // The macro builds a string interpolation expression at compile time.
    var score = 42
    printf("player (1) scored (2) points\n", "Alice", score)

    // Arguments can be reordered — (2) before (1)
    printf("result: (2) from (1)\n", "source", 100)

    // Arguments can be repeated
    printf("(1) and (1) and (1)\n", "echo")

    // Mixed types work because string interpolation handles conversion
    let pi = 3.14
    printf("pi is approximately (1), or roughly (2)\n", pi, int(pi))
}

// output:
// hello, call macro!
// hello, world!
// hello, daslang!
// player Alice scored 42 points
// result: 100 from source
// echo and echo and echo
// pi is approximately 3.14, or roughly 3
