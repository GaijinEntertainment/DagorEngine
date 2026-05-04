options gen2

module ecs_macro shared

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/rtti public
require tutorial_c_12

// [es] function macro — transforms a no-argument function into an
// ECS system that receives all struct fields as var ref arguments.
//
// User writes:
//   [es]
//   def update() {
//       position += velocity * dt
//   }
//
// Macro adds struct fields as arguments, marks [export], generates [init] registration.

[function_macro(name="es")]
class EsMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        // Find the user-defined struct in the compiling module.
        var st : Structure?
        for_each_structure(compiling_module()) $(s) {
            if (!s.flags.generated && !s.flags.isLambda && !s.flags._generator) {
                st = get_ptr(s)
            }
        }
        if (st == null) {
            errors := "no component struct found in module; define a struct with your component fields"
            return false
        }
        // Add struct fields as function arguments: var field : type&
        for (fld in st.fields) {
            func.arguments |> emplace_new <| qmacro_variable(string(fld.name), type<$t(fld._type)&>)
        }
        // Mark exported so C side can find the function by name
        func.flags |= FunctionFlags.exports
        // Generate [init] function that calls ecs_register(fn_ptr, name, typeinfo)
        let funcName = string(func.name)
        var inscope stType <- new TypeDecl(at = func.at, baseType = Type.tStructure)
        stType.structType = st
        var blk <- setup_call_list("register`es`{funcName}", func.at, true, true)
        blk.list |> emplace_new <| qmacro(ecs_register(unsafe(reinterpret<void?> @@$c(funcName)), $v(funcName), typeinfo rtti_typeinfo(type<$t(stType)>)))
        // Register @required globals — only if not already done
        var gblk <- setup_call_list("register`globals", func.at, true, true)
        if (length(gblk.list) == 0) {
            for_each_global(compiling_module()) $(gvar) {
                let isRequired = !(gvar.annotation |> find_arg("required") is nothing)
                if (isRequired) {
                    let gname = string(gvar.name)
                    var inscope gType <- clone_type(gvar._type)
                    gblk.list |> emplace_new <| qmacro(ecs_register_global($v(gname), unsafe(addr($i(gname))), typeinfo rtti_typeinfo(type<$t(gType)>)))
                }
            }
        }
        return true
    }
}
