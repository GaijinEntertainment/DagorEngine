// Macro Tutorial 5: Tag Function Macros — Module
//
// This module demonstrates [tag_function] + [tag_function_macro] — the
// standard way to build single-file macro modules in daslang.
//
// The problem:
//   [function_macro(name="...")] requires the macro class to exist
//   before the annotated function is compiled.  That forces two files:
//   a module with the macro, and a usage file that requires it.
//
// The solution:
//   [tag_function(tag_name)] marks a function with a tag — it doesn't
//   need the macro class to exist yet.  [tag_function_macro(tag="tag_name")]
//   later attaches the macro to the tagged function during module setup.
//   Both can live in the same module.
//
// This pattern is used by many daslib modules:
//   - daslib/unroll      — compile-time loop unrolling
//   - daslib/defer        — Go-style defer (move code to finally)
//   - daslib/assert_once  — fire assertion only on first failure
//   - daslib/static_let   — promote locals to globals
//   - daslib/safe_addr    — safe address-of operations
//   - daslib/jobque_boost — parallel_for, new_job
//   - ...and more (14+ modules, 20+ tags)
//
// We build a [once] macro: execute a block only on the first call.
// Each call site gets its own auto-generated global bool flag.

options gen2

module tag_function_macro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

// ============================================================================
// The tagged function stub
//
// [tag_function(once_tag)] records a tag name — it does NOT apply any
// macro yet.  The function body is a fallback that runs only if the
// macro somehow fails to transform the call (should not happen in
// normal use).
// ============================================================================

[tag_function(once_tag)]
def public once(blk : block) {
    invoke(blk)
}

// ============================================================================
// The tag function macro
//
// [tag_function_macro(tag="once_tag")] tells the compiler: "during
// module setup, find every function tagged with 'once_tag' and attach
// this class as its function annotation."
//
// The class overrides transform(), which intercepts every call to once()
// and replaces it with guarded code:
//
//   once() { print("hello\n") }
//
// becomes:
//
//   if (!`__once_flag_42_5`) {
//       `__once_flag_42_5` = true
//       print("hello\n")
//   }
//
// where `__once_flag_42_5` is a private global variable unique to the
// call site (named from line and column numbers).
// ============================================================================

[tag_function_macro(tag="once_tag")]
class OnceMacro : AstFunctionAnnotation {

    // transform() is called for every call to the tagged function.
    // It receives the call expression and returns a replacement.
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {

        // Step 1: Generate a unique name for the global flag variable.
        //         make_unique_private_name uses the call-site LineInfo
        //         (line + column) to produce a name like "__once_flag_12_5".
        let flag_name = make_unique_private_name("__once_flag", call.at)

        // Step 2: Create the global flag, initialized to false.
        //         add_global_private_var adds a private mutable variable
        //         to the compiling module.  quote(false) creates the
        //         initializer expression (ExprConstBool with value false).
        if (!compiling_module() |> add_global_private_var(flag_name, call.at) <| quote(false)) {
            errors := "can't add global variable {flag_name}"
            return <- default<ExpressionPtr>
        }

        // Step 3: Extract the block body from the call argument.
        //         When the user writes once() { ... }, the first
        //         argument is an ExprMakeBlock wrapping an ExprBlock.
        //         We clone the block, unwrap it, and extract its statements.
        var inscope block_clone <- clone_expression(call.arguments[0])
        var inscope blk <- move_unquote_block(block_clone)
        var inscope stmts : array<ExpressionPtr>
        for (s in blk.list) {
            stmts |> emplace_new <| clone_expression(s)
        }

        // Step 4: Build the replacement AST using qmacro_block.
        //         $i(flag_name) splices the flag name as an identifier.
        //         $b(stmts) splices the statement array into the block.
        var inscope replacement <- qmacro_block() {
            if (!$i(flag_name)) {
                $i(flag_name) = true
                $b(stmts)
            }
        }
        replacement |> force_at(call.at)
        return <- replacement
    }
}
