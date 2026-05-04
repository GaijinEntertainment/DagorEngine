// Macro Tutorial 7: Block Macros
//
// This tutorial demonstrates [block_macro] — annotations that operate
// on block closures (the $ { ... } expressions passed to functions).
//
// Block macros extend AstBlockAnnotation and have two methods:
//
//   apply()  — during parsing, before inference.  Can modify the
//              block's statement list and finally list, validate
//              annotation arguments, and inject code.
//
//   finish() — after all inference and optimization.  Read-only:
//              argument types are fully resolved, useful for
//              diagnostics and compile-time reporting.
//
// There is no patch() method — unlike structure macros, block macros
// cannot modify the block after inference.  All AST changes must
// happen in apply().
//
// We build a [traced(tag="X")] annotation that:
//   1. prepends an enter-print and appends an exit-print to the
//      block's finally list (apply)
//   2. prints a compile-time summary of each block's arguments
//      and statement count (finish)
//
// Block annotation syntax:
//   $ [annotation(args)] (block_params) { body }
//
// The annotation sits between $ and the parameter list.
//
// Covers:
//   - [block_macro(name=...)] to register a block annotation
//   - AstBlockAnnotation with apply / finish
//   - find_arg for reading annotation arguments
//   - blk.list — the block's statement list
//   - blk.finalList — the block's finally list (runs on exit)
//   - emplace(vec, val, index) to insert at a specific position
//   - describe(arg._type) for type names after inference
//
// The macro is defined in block_macro_mod.das.
//
// Run: daslang.exe tutorials/macros/07_block_macro.das

options gen2

require block_macro_mod

// ── Helper: invoke a parameterless block ──

def run_block(blk : block) {
    invoke(blk)
}

// ── Helper: invoke a block with one int argument ──

def apply_to(x : int; blk : block<(v : int) : void>) {
    invoke(blk, x)
}

// ── Using the [traced] block annotation ──

[export]
def main() {
    // Simple block — no parameters, one statement
    print("--- simple block ---\n")
    run_block() $ [traced(tag="setup")] {
        print("  initializing\n")
    }

    // Block with a parameter — the exit-print goes through finalList,
    // so it runs after the block body completes
    print("\n--- block with argument ---\n")
    apply_to(42) $ [traced(tag="process")] (v : int) {
        print("  received {v}\n")
    }
}

// compile-time output (from finish):
//   [traced] "setup": 1 statement(s)
//   [traced] "process": 1 statement(s), args = (v:int const)
//
// runtime output:
// --- simple block ---
// >> setup
//   initializing
// << setup
//
// --- block with argument ---
// >> process
//   received 42
// << process
