// Macro Tutorial 7: Block Macros — Module
//
// This module defines the [traced] block macro, which demonstrates
// both AstBlockAnnotation methods:
//
//   apply()  — runs before inference.  Validates the annotation
//              argument, injects an enter-print at the start of the
//              block body, and appends an exit-print to the block's
//              finally list.
//
//   finish() — runs after all inference and optimization.  Reads
//              the block's fully-typed argument list and prints a
//              compile-time diagnostic summary.
//
// Block macros have only two methods (no patch).  They annotate
// block closures — the $ { ... } expressions passed to functions —
// not function or struct definitions.
//
// Block macros cannot be used in the module that defines them —
// see 07_block_macro.das for usage.

options gen2

module block_macro_mod public

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost

// ============================================================================
// The [traced] block macro
//
// Given:
//
//   run_block() $ [traced(tag="setup")] {
//       print("initializing\n")
//   }
//
// The macro:
//   1. (apply)  validates `tag`, prepends enter-print, appends
//               exit-print to finally
//   2. (finish) prints compile-time summary with typed argument info
//
// ============================================================================

[block_macro(name="traced")]
class TracedBlockMacro : AstBlockAnnotation {

    // ------------------------------------------------------------------
    // apply() — before type inference
    //
    // Runs during parsing.  The block's argument types may not yet be
    // resolved.  We:
    //   - validate the `tag` string argument
    //   - prepend a print(">> tag\n") to the block body (position 0)
    //   - append a print("<< tag\n") to the block's finalList, so
    //     the exit-message prints on block exit (like a finally block)
    // ------------------------------------------------------------------
    def override apply(var blk : smart_ptr<ExprBlock>; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        // Validate: require a 'tag' string argument
        let labelArg = find_arg(args, "tag")
        if (!(labelArg is tString)) {
            errors := "[traced] requires a 'tag' string argument, e.g. [traced(tag=\"setup\")]"
            return false
        }
        let lbl = labelArg as tString

        // Prepend enter-print at position 0 (before any user code).
        // emplace(vec, val, index) inserts at a specific position.
        var inscope enterExpr <- qmacro(print($v(">> {lbl}\n")))
        blk.list |> emplace(enterExpr, 0)

        // Append exit-print to finalList.  The finalList acts like a
        // `finally` block — it runs when the block exits, even on
        // early return.  This guarantees the exit message always prints.
        var inscope exitExpr <- qmacro(print($v("<< {lbl}\n")))
        blk.finalList |> emplace(exitExpr)

        return true
    }

    // ------------------------------------------------------------------
    // finish() — after inference is complete
    //
    // All types are resolved.  We read the block's argument list to
    // report their names and inferred types, and count the user's
    // original statements (total minus the injected enter-print).
    //
    // Unlike structure macros, block annotations have no patch() method.
    // Everything that needs AST modification must happen in apply().
    // finish() is read-only — use it for diagnostics and reporting.
    // ------------------------------------------------------------------
    def override finish(var blk : smart_ptr<ExprBlock>; var group : ModuleGroup;
                        args, progArgs : AnnotationArgumentList; var errors : das_string) : bool {
        // Recover the tag
        let labelArg = find_arg(args, "tag")
        var lbl = "?"
        if (labelArg is tString) {
            lbl = labelArg as tString
        }

        // Count original user statements (subtract 1 for the injected
        // enter-print that apply() prepended)
        var numStmts = 0
        for (s in blk.list) {
            numStmts++
        }
        if (numStmts > 0) {
            numStmts -= 1
        }

        // Report
        print("[traced] \"{lbl}\": {numStmts} statement(s)")

        // Show typed arguments — types are fully resolved after inference,
        // so describe() returns proper type names like "int", "string", etc.
        var numArgs = 0
        for (a in blk.arguments) {
            numArgs++
        }
        if (numArgs > 0) {
            print(", args = (")
            var first = true
            for (arg in blk.arguments) {
                if (!first) {
                    print(", ")
                }
                print("{arg.name}:{describe(arg._type)}")
                first = false
            }
            print(")")
        }

        print("\n")
        return true
    }
}
