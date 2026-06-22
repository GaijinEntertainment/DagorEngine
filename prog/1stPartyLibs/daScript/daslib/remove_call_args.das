options gen2
options indenting = 4
options strict_smart_pointers = true

module remove_call_args shared private

//! Call argument removal annotation.
//!
//! ``[remove_call_args(index_list)]`` strips the specified arguments from
//! a function's call site after the function has been type-checked, while
//! keeping the arguments available during inference.

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost


[macro_function]
def get_remove_indices(func) {
    for (ann in func.annotations) {
        if (ann.annotation.name == "remove_call_args") return <- get_remove_indices(ann.arguments)
    }
    return <- array<int>()
}


[macro_function]
def get_remove_indices(args : AnnotationArgumentList) {
    var indices <- [for (arg in args); arg.iValue; where arg.name == "arg"]
    if (!empty(indices)) {
        indices |> sort() <| $(a, b) => a > b
    }
    return <- indices
}


[function_macro(name="remove_call_args")]
class RemoveCallArgsMacro : AstFunctionAnnotation {
    //! This macro removes all arguments by given indices [remove_call_args(arg=(1,2,3))]
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        var removeIndices <- get_remove_indices(args)
        if (empty(removeIndices)) return true

        for (arg in func.arguments) {
            arg.flags.marked_used = true // to disable errors: unused function argument
        }

        var fn = clone_function(func)
        fn.flags |= FunctionFlags.generated | FunctionFlags.exports
        for (idx in removeIndices) {
            fn.arguments |> erase(idx)
        }

        for (i in iter_range(fn.annotations)) {
            if (fn.annotations[i].annotation.name == "remove_call_args") {
                fn.annotations |> erase(i)
                break
            }
        }

        compiling_module() |> add_function(fn)
        return true
    }

    def override transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr {
        var removeIndices <- get_remove_indices(call.func)

        if (empty(removeIndices)) return null

        var newCall = clone_expression(call)
        var ncall = newCall as ExprCall
        for (idx in removeIndices) {
            ncall.arguments |> erase(idx)
        }
        return <- newCall
    }
}
