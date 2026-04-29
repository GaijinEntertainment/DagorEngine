// Macro Tutorial 2: When Expression — Module
//
// This module defines a "when" call macro that acts as a value-returning
// match expression.  It demonstrates advanced call macro techniques:
//   - canVisitArgument: controlling argument visitation order
//   - canFoldReturnResult: deferring return type inference
//   - qmacro_block: block-level AST reification
//   - $i(), $t(), $b(): identifier, type, and block-list injection
//   - clone_type and type flag manipulation
//   - Working with ExprMakeBlock, ExprBlock, ExprMakeTuple
//   - can_shadow flag for generated variables
//   - typedecl($e(...)) for inferring types from expressions
//   - default<T> for generating type-safe fallback values
//
// The "when" macro cannot be used in this module — see 02_when_macro.das.

options gen2

module when_macro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require daslib/macro_boost

// ============================================================================
// The "when" macro — a value-returning match expression
//
// Syntax:
//   when(value) {
//       pattern1 => result1
//       pattern2 => result2
//       _        => default_result   // optional, must be last
//   }
//
// The macro transforms this into:
//   invoke($(arg : value_type) {
//       if (arg == pattern1) { return result1; }
//       if (arg == pattern2) { return result2; }
//       return default_result;
//   }, value)
//
// The value is evaluated once (captured as the block argument), and
// each pattern is tested with == in order.  The first match wins.
//
// Key techniques demonstrated:
//
// 1. canVisitArgument(expr, argIndex)
//    Controls whether the compiler type-checks each argument before
//    the macro's visit() runs.  We return true for argument 0 (the
//    condition value) so its type is inferred, but for argument 1
//    (the block), we return !is_reporting_compilation_errors() so
//    the block is visited during normal compilation but not during
//    the error-reporting pass (after the macro has transformed it).
//
// 2. canFoldReturnResult(expr)
//    Returning false tells the compiler not to finalize the return
//    type of the enclosing function until after this macro expands.
//    Without this, the compiler might infer `void` too early.
//
// 3. qmacro_block() { ... }
//    Like qmacro() but produces a list of statements (ExpressionPtr)
//    rather than a single expression.  Used to build if-return pairs.
//
// 4. $i(name) — injects a string as an identifier (ExprVar)
//    $t(type) — injects a TypeDeclPtr into the reified AST
//    $b(list) — injects an array<ExpressionPtr> as a block body
//
// 5. clone_type + flag manipulation
//    We clone the condition's type and clear .ref (we compare values,
//    not references) and set .constant (the argument is read-only).
//
// 6. can_shadow flag
//    The generated block argument uses a fixed name (__when_arg__).
//    Setting can_shadow = true allows nested when() expressions to
//    each introduce their own __when_arg__ without name conflicts.
// ============================================================================

[call_macro(name="when")]
class WhenMacro : AstCallMacro {
    def override canVisitArgument(expr : smart_ptr<ExprCallMacro>; argIndex : int) : bool {
        // Visit the condition (arg 0) so its type is inferred.
        // For the block (arg 1), visit during normal compilation but
        // not during error reporting (the block is already transformed).
        return true if (argIndex == 0)
        return !is_reporting_compilation_errors()
    }
    def override canFoldReturnResult(expr : smart_ptr<ExprCallMacro>) : bool {
        // Don't let the compiler finalize return type before we expand.
        return false
    }
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        // --- Validate arguments ---
        macro_verify(length(expr.arguments) == 2, prog, expr.at,
            "when takes a value and a block")
        assume cond = expr.arguments[0]
        macro_verify(expr.arguments[1] is ExprMakeBlock, prog, expr.at,
            "second argument to when must be a block, not {expr.arguments[1].__rtti}")
        macro_verify(cond._type != null, prog, expr.at,
            "first argument to when must have a type")
        macro_verify(!cond._type.isAutoOrAlias, prog, expr.at,
            "first argument to when must be fully inferred")
        let blk = ((expr.arguments[1] as ExprMakeBlock)._block as ExprBlock)
        macro_verify(length(blk.finalList) == 0, prog, expr.at,
            "'when' block cannot have a finally section")
        macro_verify(length(blk.list) > 0, prog, expr.at,
            "'when' block must have at least one case")
        // --- Build the statement list ---
        var inscope list : array<ExpressionPtr>
        let arg_name = "__when_arg__"
        var any_default = false
        for (stmt, idx in blk.list, count()) {
            macro_verify(stmt is ExprMakeTuple, prog, stmt.at,
                "block statements must be of form 'key => value', not {stmt.__rtti}")
            let tupl = stmt as ExprMakeTuple
            macro_verify(length(tupl.values) == 2, prog, stmt.at,
                "block statements must be of form 'key => value'")
            assume cond_value = tupl.values[0]
            let is_default = (cond_value is ExprVar) && (cond_value as ExprVar).name == "_"
            let is_last = idx == length(blk.list) - 1
            macro_verify(!(is_default && !is_last), prog, stmt.at,
                "default case (_) must be last in the block")
            if (is_default) {
                // _ => value  →  return value
                list |> emplace_new <| qmacro_block() {
                    return $e(tupl.values[1])
                }
                any_default = true
            } else {
                // key => value  →  if (arg == key) { return value; }
                list |> emplace_new <| qmacro_block() {
                    if ($i(arg_name) == $e(tupl.values[0])) {
                        return $e(tupl.values[1])
                    }
                }
            }
        }
        // --- Generate default case if none was provided ---
        // Without a default, the block would have no return on some paths.
        // We infer the return type from the first case's value expression
        // using typedecl($e(...)), and return the type's default value.
        if (!any_default) {
            assume first_value = (blk.list[0] as ExprMakeTuple).values[1]
            list |> emplace_new <| qmacro_block() {
                return default<typedecl($e(first_value))>
            }
        }
        // --- Build the block's argument type ---
        var inscope cond_type <- clone_type(cond._type)
        cond_type.flags.ref = false      // compare values, not references
        cond_type.flags.constant = true  // block argument is read-only
        // --- Assemble: invoke($(arg : type){ body }, condition) ---
        var inscope call_block <- qmacro($($i(arg_name) : $t(cond_type)){ $b(list); })
        // Allow nested when() by letting the generated name shadow outer ones
        ((call_block as ExprMakeBlock)._block as ExprBlock).arguments[0].flags.can_shadow = true
        return <- qmacro(invoke($e(call_block), $e(cond)))
    }
}
