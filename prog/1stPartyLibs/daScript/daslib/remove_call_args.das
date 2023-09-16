options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module remove_call_args shared private

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost


[macro_function]
def get_remove_indices(func)
    for ann in func.annotations
        if ann.annotation.name == "remove_call_args"
            return <- get_remove_indices(ann.arguments)
    return <- [[array<int>]]


[macro_function]
def get_remove_indices(args : AnnotationArgumentList)
    var indices : array<int>
    for arg in args
        if arg.name == "arg"
            indices |> push(arg.iValue)
    if length(indices) > 0
        indices |> sort() <| $(a, b) => a >= b;
    return <- indices


[function_macro(name="remove_call_args")]
class RemoveCallArgsMacro : AstFunctionAnnotation
    //! This macro remove all arguments by given indeces [remove_call_args(arg=(1,2,3))]
    def override apply(var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string) : bool
        var removeIndices <- get_remove_indices(args)
        if length(removeIndices) == 0
            return true

        for arg in func.arguments
            arg.flags |= VariableFlags marked_used // to disable errors: unused function argument

        var inscope fn <- clone_function(func)
        fn.flags |= FunctionFlags generated | FunctionFlags exports
        for idx in removeIndices
            fn.arguments |> erase(idx)

        for i in iter_range(fn.annotations)
            if fn.annotations[i].annotation.name == "remove_call_args"
                fn.annotations |> erase(i)
                break

        compiling_module() |> add_function(fn)
        return true

    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        var removeIndices <- get_remove_indices(call.func)

        if length(removeIndices) == 0
            return <- [[ExpressionPtr]]

        var inscope newCall <- clone_expression(call)
        var ncall = newCall as ExprCall
        for idx in removeIndices
            ncall.arguments |> erase(idx)
        return <- newCall
