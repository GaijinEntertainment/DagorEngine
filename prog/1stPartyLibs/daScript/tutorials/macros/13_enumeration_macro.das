options gen2

// Tutorial 13 — Enumeration macros
//
// Demonstrates AstEnumerationAnnotation (registered via [enumeration_macro]).
// An enumeration annotation has a single method:
//   apply(var enu, var group, args, var errors) : bool
// Called BEFORE the infer pass — the macro can modify the enum or
// generate new code (functions, global variables) in the compiling module.
//
// Section 1 uses [enum_total] from enum_macro_mod — modifies the enum
//   by adding a "total" entry equal to the number of original values.
// Section 2 uses [string_to_enum] from daslib/enum_trait — generates
//   a lookup table and two constructor functions for string-to-enum
//   conversion.

require enum_macro_mod
require daslib/enum_trait

// ---- Section 1: enum_total — modifying the enum ----------------------------

[enum_total]
enum Direction {
    North
    South
    East
    West
}

def section1() {
    // Direction now has an extra entry: Direction.total == 4.
    print("--- Section 1: enum_total ---\n")
    print("Direction.total = {int(Direction.total)}\n")
    // Iterate all original values using each() from enum_trait.
    // each() yields every value including total, so we stop at total.
    for (d in each(Direction.North)) {
        if (d == Direction.total) {
            break
        }
        print("  {d}\n")
    }
}

// ---- Section 2: string_to_enum — generating code ---------------------------

[string_to_enum]
enum Color {
    Red
    Green
    Blue
}

def section2() {
    // [string_to_enum] generated two functions:
    //   Color(src : string) : Color            — panics if not found
    //   Color(src : string; def : Color) : Color — returns default if not found
    print("--- Section 2: string_to_enum ---\n")
    let c1 = Color("Red")
    print("Color(\"Red\")   = {c1}\n")
    let c2 = Color("invalid", Color.Blue)
    print("Color(\"invalid\", Color.Blue) = {c2}\n")
}

[export]
def main() {
    section1()
    section2()
}

// Expected output:
// --- Section 1: enum_total ---
// Direction.total = 4
//   North
//   South
//   East
//   West
// --- Section 2: string_to_enum ---
// Color("Red")   = Red
// Color("invalid", Color.Blue) = Blue
