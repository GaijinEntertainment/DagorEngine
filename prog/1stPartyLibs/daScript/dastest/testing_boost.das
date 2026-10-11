options gen2
options indenting = 4
options no_aot

module testing_boost shared

require daslib/ast_boost
require testing public
require strings
require math

//! The panic a block raises, as text ("" when it returns): a refusal cell reads the message it holds.
def public panic_text(blk : block<() : void>) : string {
    var why = ""
    try {
        invoke(blk)
    } recover {
        why = this_context().last_exception |> rtrim
    }
    return why
}

//! The relative-tolerance float compare of a fastmath (GPU, flattened) result against its oracle.
def public fapprox(x, y : float) : bool {
    return abs(x - y) <= 1e-4 * max(1.0, max(abs(x), abs(y)))
}

[macro_function]
def private validate_arguments(type_name : string, args) : bool {
    let len = length(args)
    if (len == 0) {
        return true
    }
    if (len == 1) {
        return (args[0]._type.isPointer && args[0]._type.firstType.isStructure && args[0]._type.firstType.structType.name == type_name
                && args[0]._type.firstType.structType._module != null && args[0]._type.firstType.structType._module.name == "testing")
    }
    return false
}

[function_macro(name="test")]
class TestFunctionAnnotation : AstFunctionAnnotation {
    [unused_argument(group, args, errors)] def override apply(var func : FunctionPtr; var group : ModuleGroup; var args : AnnotationArgumentList; var errors : das_string) : bool {
        if (!validate_arguments("T", func.arguments)) {
            errors := "Invalid arguments for test macro\nexpected function header: def {func.name}() or def {func.name}(t: testing::T?)"
            return false
        }
        func.flags.exports = true
        return true
    }
}

[function_macro(name="benchmark")]
class BenchmarkFunctionAnnotation : AstFunctionAnnotation {
    [unused_argument(group, args, errors)] def override apply(var func : FunctionPtr; var group : ModuleGroup; var args : AnnotationArgumentList; var errors : das_string) : bool {
        if (!validate_arguments("B", func.arguments)) {
            errors := "Invalid arguments for benchmark macro\nexpected function header: def {func.name}() or def {func.name}(b: testing::B?)"
            return false
        }
        func.flags.exports = true
        return true
    }
}
