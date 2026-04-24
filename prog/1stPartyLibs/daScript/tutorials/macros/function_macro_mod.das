// Macro Tutorial 3: Function Macros — Module
//
// This module defines three function macros that demonstrate different
// AstFunctionAnnotation methods:
//
//   [log_calls]      — uses apply() to transform a function's body,
//                       adding entry/exit logging with nested indentation.
//
//   [expect_range]   — uses verifyCall() to validate call sites,
//                       rejecting constant arguments outside a given range.
//
//   [no_print]       — uses lint() to walk the fully-compiled body
//                       and reject calls to the builtin print function.
//
// Function macros cannot be used in the module that defines them —
// see 03_function_macro.das for usage.

options gen2

module function_macro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require daslib/macro_boost

require strings public

// Public mutable variable shared between the macro module and user code.
// Tracks the current call nesting depth for log indentation.
var public LOG_DEPTH = 0

// ============================================================================
// The [log_calls] function macro
//
// Transforms a function's body to add entry/exit logging.  Given:
//
//   [log_calls]
//   def add(a, b : int) : int {
//       return a + b
//   }
//
// The macro rewrites the body to:
//
//   def add(a, b : int) : int {
//       {
//           print("{repeat("  ", LOG_DEPTH++)}>> ")
//           print("add({a}, {b})\n")
//           {
//               return a + b       // ← original body
//           }
//       } finally {
//           print("{repeat("  ", --LOG_DEPTH)}<< add\n")
//       }
//   }
//
// ============================================================================

[function_macro(name="log_calls")]
class LogCallsMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {

        // Step 1: Build an ExprStringBuilder for "funcname(arg1, arg2, ...)\n"
        var inscope call_sb <- new ExprStringBuilder(at = func.at)
        call_sb.elements |> emplace_new <| qmacro($v("{string(func.name)}("))
        for (i, arg in count(), func.arguments) {
            if (i > 0) {
                call_sb.elements |> emplace_new <| quote(", ")
            }
            call_sb.elements |> emplace_new <| qmacro($i(arg.name))
        }
        call_sb.elements |> emplace_new <| quote(")\n")

        // Step 2: Build the replacement body with entry logging,
        //         original body, and exit logging in a finally block
        var inscope new_body <- qmacro_block() {
            {
                print("{repeat("  ",LOG_DEPTH++)}>> ")
                print($e(call_sb))
                {
                    $e(func.body)
                }
            } finally {
                print("{repeat("  ",--LOG_DEPTH)}<< {$v(string(func.name))}\n")
            }
        }

        // Step 3: Replace the function body
        func.body |> move <| new_body
        return true
    }
}

// ============================================================================
// Helper: extract a constant integer from an expression
//
// The compiler may wrap constant values in ExprRef2Value (a reference-to-
// value conversion).  This helper unwraps that layer, then checks whether
// the expression is an ExprConstInt and extracts its .value field.
//
// Returns true if the expression is a compile-time integer constant.
// ============================================================================

[macro_function]
def public getConstantInt(expr : ExpressionPtr; var result : int&) : bool {
    if (expr is ExprRef2Value) {
        return getConstantInt((expr as ExprRef2Value).subexpr, result)
    } elif (expr is ExprConstInt) {
        result = (expr as ExprConstInt).value
        return true
    }
    return false
}

// ============================================================================
// The [expect_range] function macro
//
// Validates call sites at compile time.  Given:
//
//   [expect_range(value, min=0, max=255)]
//   def set_channel(name : string; value : int)
//       ...
//
// Every call like set_channel("red", 300) fails at compile time:
//
//   error: call annotated by expect_range failed
//     value = 300 is out of range [0..255]
//
// Runtime (non-constant) arguments pass through — only literals are checked.
//
// Annotation argument layout:
//   - Bare name (basicType == Type.tBool): the function argument to check
//   - "min" with iValue: lower bound (inclusive)
//   - "max" with iValue: upper bound (inclusive)
//
// ============================================================================

[function_macro(name="expect_range")]
class ExpectRangeMacro : AstFunctionAnnotation {
    def override verifyCall(var call : smart_ptr<ExprCallFunc>;
                            args, progArgs : AnnotationArgumentList;
                            var errors : das_string) : bool {

        // Step 1: Parse annotation arguments — find the argument name,
        //         and the min/max bounds
        var arg_name = ""
        var range_min = int(0x80000000)  // default: no lower bound
        var range_max = int(0x7FFFFFFF)  // default: no upper bound
        for (aa in args) {
            if (aa.basicType == Type.tBool) {
                arg_name = string(aa.name)
            } elif (aa.name == "min") {
                range_min = aa.iValue
            } elif (aa.name == "max") {
                range_max = aa.iValue
            }
        }

        // Step 2: Find the matching call-site expression
        for (ca, ce in call.func.arguments, call.arguments) {
            if (string(ca.name) == arg_name) {
                // Step 3: If it is a constant integer, check the range
                var val = 0
                if (getConstantInt(ce, val)) {
                    if (val < range_min || val > range_max) {
                        errors := "{arg_name} = {val} is out of range [{range_min}..{range_max}]"
                        return false
                    }
                }
                // Non-constant arguments pass through — can't check at compile time
                break
            }
        }
        return true
    }
}

// ============================================================================
// The [no_print] function macro
//
// Validates a function's body at lint time (after full compilation).
// Rejects any call to the builtin print function (module "$").
//
// Ironic contrast: [log_calls] ADDS print calls, [no_print] FORBIDS them.
//
// At lint time the AST is fully compiled — types resolved, functions
// linked, overloads selected.  This lets us inspect expr.func._module
// to distinguish the builtin print from user-defined functions named print.
//
// ============================================================================

// AST visitor that detects calls to the builtin print function.
// Used by NoPrintMacro below — walks the function body and sets
// found_print to true if a disallowed call is found.
[macro]
class NoPrintVisitor : AstVisitor {
    found_print : bool = false
    @safe_when_uninitialized print_at : LineInfo
    def override preVisitExprCall(expr : smart_ptr<ExprCall>) : void {
        if (expr.func != null && expr.name == "print" && expr.func._module.name == "$") {
            found_print = true
            print_at = expr.at
        }
    }
}

[function_macro(name="no_print")]
class NoPrintMacro : AstFunctionAnnotation {
    def override lint(var func : FunctionPtr; var group : ModuleGroup;
                      args, progArgs : AnnotationArgumentList;
                      var errors : das_string) : bool {

        // Step 1: Create a visitor and adapt it for the visit() call
        var astVisitor = new NoPrintVisitor()
        var inscope adapter <- make_visitor(*astVisitor)

        // Step 2: Walk the function body
        visit(func, adapter)

        // Step 3: Check the result
        if (astVisitor.found_print) {
            errors := "function {string(func.name)} must not call builtin print (at line {int(astVisitor.print_at.line)})"
            unsafe { delete astVisitor; }
            return false
        }
        unsafe { delete astVisitor; }
        return true
    }
}
