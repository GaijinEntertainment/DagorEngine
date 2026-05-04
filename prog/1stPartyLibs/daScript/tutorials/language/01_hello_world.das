// Tutorial 1: Hello World
//
// Your first daslang program. This tutorial covers:
//   - options gen2     (enabling modern syntax)
//   - [export] def main (the entry point)
//   - print            (writing to the console)
//   - string interpolation with {expression}
//
// Run: daslang.exe tutorials/language/01_hello_world.das

options gen2

// Every daslang program needs an entry point.
// [export] marks the function as callable by the host.
// def main is the conventional entry point function.

[export]
def main {

    // print is a built-in function that writes text to the console.
    // It does NOT add a newline automatically — use \n explicitly.
    print("Hello, World!\n")

    // String interpolation: any expression inside {braces} within
    // a string literal is evaluated and converted to text.
    let name = "daslang"
    print("Welcome to {name}!\n")

    // You can put arbitrary expressions inside {braces}.
    print("2 + 3 = {2 + 3}\n")

    // Multiple expressions in one string.
    let a = 10
    let b = 20
    print("{a} + {b} = {a + b}\n")

    // To print a literal brace character, escape it with backslash.
    print("Use \{braces\} for interpolation\n")
}

// output:
// Hello, World!
// Welcome to daslang!
// 2 + 3 = 5
// 10 + 20 = 30
// Use {braces} for interpolation
