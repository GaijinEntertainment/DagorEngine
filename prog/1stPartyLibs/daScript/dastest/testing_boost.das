options indenting = 4
options no_aot

module testing_boost shared

require daslib/ast_boost
require testing public

[macro_function]
def private validate_arguments(args): bool
    let len = length(args)
    if len == 0
        return true
    if len == 1
        return (args[0]._type.isPointer && args[0]._type.firstType.isStructure && args[0]._type.firstType.structType.name == "T"
                && args[0]._type.firstType.structType._module != null && args[0]._type.firstType.structType._module.name == "testing")
    return false

[function_macro(name="test")]
class TestFunctionAnnotation : AstFunctionAnnotation
    [unused_argument(group, args, errors)] def override apply(var func: FunctionPtr; var group: ModuleGroup; args: AnnotationArgumentList; var errors: das_string): bool
        if !validate_arguments(func.arguments)
            errors := "Invalid arguments for test macro\nexpected function header: def {func.name}() or def {func.name}(t: testing::T?)"
            return false
        func.flags |= FunctionFlags exports
        return true
