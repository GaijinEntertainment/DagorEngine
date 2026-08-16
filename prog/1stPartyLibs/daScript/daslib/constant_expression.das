options gen2
options indenting = 4
options strict_smart_pointers = true

module constant_expression shared private

//! Compile-time constant-expression enforcement.
//!
//! ``[constexpr(arg)]`` verifies at compile time that the named argument
//! is a constant expression and also implements ``[constant_expression(arg)]``
//! which folds the argument to a compile-time value.

require daslib/ast
require daslib/ast_boost
require daslib/strings_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/algorithm

[function_macro(name=constexpr)]
class ConstExprAnnotation : AstFunctionAnnotation {
    //! Checks selected function arguments and fails when any receives a non-constant expression.
    def override verifyCall(var call : ExprCallFunc?; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool {
        for (aa in args) {
            if (aa.basicType == Type.tBool) {
                for (ca, ce in call.func.arguments, call.arguments) {
                    if (ca.name == aa.name) {
                        if (!isConstantExpression(ce)) {
                            errors := "{aa.name} is not a constexpr, {describe(ce)}"
                            return false
                        }
                    }
                }
            }
        }
        return true
    }
}

[macro_function]
def public isConstantExpression(expr : Expression?) : bool {
    //! This macro function returns true if the expression is a constant expression
    if (expr is ExprRef2Value) {
        return isConstantExpression((expr as ExprRef2Value).subexpr)
    } else {
        return expr.__rtti |> starts_with("ExprConst")
    }
}

[function_macro(name="constant_expression")]
class ConstantExpressionMacro : AstFunctionAnnotation {
    //! This function annotation implements constant expression folding for the given arguments.
    def override transform(var expr : ExprCallFunc?; var errors : das_string) : Expression? {
        macro_verify(expr.func != null, compiling_program(), expr.at, "expecting inferred function")
        var argi : array<int>
        var anni = -1
        for (ann, idd in expr.func.annotations, count()) {
            if (ann.annotation.name == "constant_expression") {
                anni = idd
                for (arg in ann.arguments) {
                    macro_verify(arg.basicType == Type.tBool, compiling_program(), expr.at, "unsupported argument type {arg.basicType}")
                    for (farg, fidx in expr.func.arguments, count()) {
                        if (farg.name == arg.name) {
                            if (!isConstantExpression(expr.arguments[fidx])) {
                                if (arg.bValue) {
                                    compiling_program() |> macro_error(expr.at, "function {expr.func.name} argument {arg.name} is not a compilation time constant")
                                }
                                return null
                            }
                            argi |> push(fidx)
                        }
                    }
                }
                break
            }
        }
        argi |> sort_unique
        argi |> reverse
        if (empty(argi)) return null
        var func_copy = clone_function(expr.func)
        let func_name = build_string() $(var w) {
            w |> write("{func_copy.name}`constant_expression")
            for (i in argi) {
                let fhash = hash(describe(expr.arguments[i]))
                w |> write("`{fhash}")
            }
        }
        func_copy.name := func_name
        if (expr.func.fromGeneric != null) {
            func_copy.body := null
            func_copy.result := null
            unsafe {
                func_copy.body = clone_expression(expr.func.fromGeneric.body)
                func_copy.result = clone_type(expr.func.fromGeneric.result)
            }
        }

        apply_template(func_copy.body) $(rules) {
            for (i in argi) {
                rules |> replaceVariable(string(expr.func.arguments[i].name)) <| clone_expression(expr.arguments[i])
            }
        }
        for (i in argi) {
            func_copy.arguments |> erase(i)
        }
        // func_copy.flags |= FunctionFlags exports
        func_copy.flags.generated = true
        func_copy.annotations |> erase(anni)
        compiling_module() |> add_function(func_copy)
        var new_call = clone_expression(expr)
        var ncall = new_call as ExprCall
        // Resolve the generated specialization in thisModule (where add_function placed it below), not the call site's defining module. Matters when the annotated function is called from library-internal generic code (e.g. json_boost`from_json_row`'s apply_imm): such a call site lives in the generic's ORIGIN module, so a plain name would search there and miss the specialization in the program root.
        ncall.name := "__::{func_name}"
        for (i in argi) {
            ncall.arguments |> erase(i)
        }
        return new_call
    }
}
