// Macro Tutorial 2: When Expression — Advanced Call Macros
//
// This tutorial demonstrates a "when" call macro — a value-returning
// match expression that transforms block syntax with => tuples into
// a chain of if-return statements.
//
// Covers:
//   - canVisitArgument: controlling when the compiler type-checks arguments
//   - canFoldReturnResult: deferring return type inference
//   - qmacro_block: block-level AST reification
//   - $i(), $t(), $b(): injecting identifiers, types, and statement lists
//   - clone_type and type flag manipulation
//   - can_shadow for generated variable names
//   - Using => tuples as a DSL building block
//
// The macro is defined in when_macro_mod.das (a separate module,
// because macros cannot be used in the module that defines them).
//
// Run: daslang.exe tutorials/macros/02_when_macro.das

options gen2

require when_macro_mod

// ============================================================================
// Section 1: Basic integer matching
//
// The simplest use of when() — match an integer value against constants.
// The first case where arg == pattern returns the corresponding result.
// The _ case is the default (must be last if present).
// ============================================================================

[export]
def main() {

    // Section 1: Integer matching with default
    var x = 2
    let result = when(x) {
        1 => "one"
        2 => "two"
        3 => "three"
        _ => "other"
    }
    print("x={x}: {result}\n")

    // Section 2: String matching
    let lang = "daslang"
    let greeting = when(lang) {
        "python"  => "import this"
        "daslang" => "hello, call macro!"
        _         => "unknown language"
    }
    print("lang={lang}: {greeting}\n")

    // Section 3: Expression results (not just constants)
    let n = 3
    let desc = when(n) {
        1 => "single"
        2 => "pair"
        3 => "triple ({n} items)"
        _ => "many ({n} items)"
    }
    print("n={n}: {desc}\n")

    // Section 4: Without a default case
    // When no _ default is provided, the macro generates one automatically
    // using default<typedecl(first_value)> — the type's default value
    // (empty string for string, 0 for int, etc.)
    let y = 42
    let found = when(y) {
        1 => "found one"
        2 => "found two"
    }
    print("y={y}: '{found}'\n")

    // Section 5: Nested when expressions
    // can_shadow allows nesting — each when() generates its own
    // __when_arg__ that shadows the outer one.
    let a = 1
    let b = 2
    let nested = when(a) {
        1 => when(b) {
            1 => "a=1, b=1"
            2 => "a=1, b=2"
            _ => "a=1, b=other"
        }
        _ => "a=other"
    }
    print("nested: {nested}\n")

    // Section 6: Works with any type that supports ==
    let val = 3.14
    let approx = when(val) {
        3.14 => "pi"
        2.72 => "e"
        _    => "unknown constant"
    }
    print("val={val}: {approx}\n")

}

// output:
// x=2: two
// lang=daslang: hello, call macro!
// n=3: triple (3 items)
// y=42: ''
// nested: a=1, b=2
// val=3.14: pi
