options gen2

// Companion script for the "Custom Modules" tutorial.
// Two C++ modules are loaded:
//   - "math_types" provides Vec2 and constants (PI, TWO_PI, etc.)
//   - "math_utils" provides utility functions (length, dot, normalize, lerp)

require math_types
require math_utils

[export]
def test() {
    // --- Constants from math_types ---
    print("=== Constants ===\n")
    print("PI      = {PI}\n")
    print("TWO_PI  = {TWO_PI}\n")
    print("HALF_PI = {HALF_PI}\n")
    print("ORIGIN  = {ORIGIN}\n")

    // --- Vec2 from math_types, functions from math_utils ---
    print("\n=== Vec2 ===\n")
    let a = make_vec2(3.0, 4.0)
    let b = make_vec2(1.0, 0.0)
    print("a = ({a.x}, {a.y})\n")
    print("b = ({b.x}, {b.y})\n")

    print("\n=== Utility functions ===\n")
    print("length(a)     = {length(a)}\n")
    print("dot(a, b)     = {dot(a, b)}\n")

    let n = normalize(a)
    print("normalize(a)  = ({n.x}, {n.y})\n")

    // --- Operators from math_utils ---
    print("\n=== Operators ===\n")
    let c = a + b
    print("a + b = ({c.x}, {c.y})\n")

    let d = a * 2.0
    print("a * 2 = ({d.x}, {d.y})\n")

    // --- lerp ---
    print("\n=== Lerp ===\n")
    let mid = lerp(a, b, 0.5)
    print("lerp(a, b, 0.5) = ({mid.x}, {mid.y})\n")

    let start = lerp(a, b, 0.0)
    print("lerp(a, b, 0.0) = ({start.x}, {start.y})\n")

    let end_pt = lerp(a, b, 1.0)
    print("lerp(a, b, 1.0) = ({end_pt.x}, {end_pt.y})\n")
}

