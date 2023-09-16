module qfn public

require ast
require daslib/ast_boost
require daslib/templates_boost

struct CommandBuffer
    cmd : int

[structure_macro(name=qfn)]
class QfnFields : AstStructureAnnotation
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        var sendFn <- qmacro_function("send") <| $(var cb : CommandBuffer&; cmd : $t(st)) : bool
            print("send({cb},{cmd})\n")
            return true
        sendFn.flags |= FunctionFlags exports
        compiling_module() |> add_function(sendFn)
        return true
