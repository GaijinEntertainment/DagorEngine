// Macro Tutorial 6: Structure Macros
//
// This tutorial demonstrates [structure_macro] — the structure-level
// equivalent of [function_macro].  Structure macros annotate structs
// and classes, and have three methods that fire at different stages
// of the compilation pipeline:
//
//   apply()  — during parsing, before inference.  Can add fields,
//              validate annotation arguments, and generate stub
//              functions.  Cannot inspect inferred types.
//
//   patch()  — after inference.  Types are resolved, so the macro
//              can filter fields by type.  We fill in the describe
//              function body here, skipping non-serializable types
//              (lambdas, blocks, function pointers).  Sets astChanged
//              to trigger re-inference of the modified body.
//
//   finish() — after all inference and optimization.  Read-only:
//              useful for diagnostics and compile-time reporting.
//
// We build a [serializable] annotation that:
//   1. adds a _version field and generates a stub describe_X()
//      function (apply)
//   2. fills in field printing, skipping non-serializable types
//      like function pointers (patch)
//   3. prints a compile-time summary of each struct (finish)
//
// Covers:
//   - [structure_macro(name=...)] to register a structure annotation
//   - AstStructureAnnotation with apply / patch / finish
//   - add_structure_field to add fields during apply
//   - find_unique_function + reinterpret<ExprBlock?> to modify a
//     generated function body in patch
//   - Type-based field filtering after inference
//   - astChanged to trigger re-inference after body modification
//   - qmacro_function + $b() + $v() + $f() for code generation
//   - Compile-time diagnostics in finish
//
// The macro is defined in structure_macro_mod.das.
//
// Run: daslang.exe tutorials/macros/06_structure_macro.das

options gen2

require structure_macro_mod

// ── A simple struct ──
//
// Default version (1).  All fields are serializable (float).

[serializable]
struct Color {
    r : float
    g : float
    b : float
}

// ── A struct with a non-serializable field ──
//
// version=2.  The `on_hit` function pointer cannot be serialized,
// so patch() skips it when building field prints.

[serializable(version=2)]
struct Player {
    name : string
    health : int
    score : float
    on_hit : function<(damage : int) : void>
}

// ── Using the generated describe functions ──

[export]
def main() {
    var c = Color(r = 0.2, g = 0.7, b = 1.0)
    print("--- describe_Color ---\n")
    describe_Color(c)

    print("\n--- describe_Player ---\n")
    var p = Player(name = "Alice", health = 100, score = 42.5)
    describe_Player(p)

    // The _version field is a regular field — accessible at runtime
    print("\n--- version info ---\n")
    print("Color version: {c._version}\n")
    print("Player version: {p._version}\n")
}

// compile-time output (from finish):
//   [serializable] Color: 3 serializable field(s), version 1
//   [serializable] Player: 3 serializable field(s), 1 skipped, version 2
//
// runtime output:
// --- describe_Color ---
// Color (version 1):
//   r = 0.2
//   g = 0.7
//   b = 1
//
// --- describe_Player ---
// Player (version 2):
//   name = Alice
//   health = 100
//   score = 42.5
//
// --- version info ---
// Color version: 1
// Player version: 2
