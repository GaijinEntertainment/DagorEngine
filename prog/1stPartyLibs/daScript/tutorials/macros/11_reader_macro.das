options gen2

// Tutorial 11 — Reader macros
//
// Demonstrates AstReaderMacro (registered via [reader_macro]).
// A reader macro embeds custom syntax inside daScript source via the
// %name~ character_sequence %% syntax.  The macro controls parsing
// through three methods:
//
//   accept()  — called per-character during parsing; collects input
//               until the terminator (%%) is reached.
//   visit()   — called during type inference; replaces the ExprReader
//               node with an AST expression.  Used for EXPRESSION-LEVEL
//               reader macros (the "visit" pattern).
//   suffix()  — called immediately after accept() during parsing;
//               returns daScript source text that the parser re-parses.
//               Used for MODULE-LEVEL reader macros (the "suffix" pattern).
//
// The companion module (reader_macro_mod) defines:
//   CsvReader   — [reader_macro(name=csv)]   visit pattern
//   BasicReader — [reader_macro(name=basic)]  suffix pattern
//
// Section 1 shows the visit pattern: %csv~ embeds a compile-time
//   string array from comma-separated text.
// Section 2 shows the suffix pattern: %basic~ transpiles a toy BASIC
//   program into a daScript function at parse time.

require reader_macro_mod

// ---- Section 1: CSV reader (visit pattern) ----------------------------------

def section1() {
    print("--- Section 1: CSV reader (visit pattern) ---\n")
    // %csv~ ... %% parses CSV at compile time into a string array.
    // The accept() method collects characters; visit() splits by comma,
    // trims whitespace, and builds the array via convert_to_expression().
    var data <- %csv~ Alice, 30, New York %%
    print("  items ({length(data)}):\n")
    for (item in data) {
        print("    '{item}'\n")
    }
    // A second example with different data.
    var colors <- %csv~red,green,blue%%
    print("  colors ({length(colors)}):\n")
    for (c in colors) {
        print("    '{c}'\n")
    }
    delete data
    delete colors
}

// ---- Section 2: BASIC transpiler (suffix pattern) ---------------------------
// The %basic~ ... %% reader macro at MODULE LEVEL generates a daScript
// function definition.  The suffix() method transpiles numbered BASIC
// lines into daScript source code, which the parser re-parses as normal
// module-level content.  The ExprReader node is discarded.
//
// The generated function can then be called from daScript code.

%basic~
DEF basic_hello
10 PRINT "Hello from BASIC"
20 LET x = 42
30 PRINT x
%%

[export]
def main() {
    section1()
    print("\n--- Section 2: BASIC transpiler (suffix pattern) ---\n")
    basic_hello()
}

// output:
// --- Section 1: CSV reader (visit pattern) ---
//   items (3):
//     'Alice'
//     '30'
//     'New York'
//   colors (3):
//     'red'
//     'green'
//     'blue'
//
// --- Section 2: BASIC transpiler (suffix pattern) ---
// Hello from BASIC
// 42
