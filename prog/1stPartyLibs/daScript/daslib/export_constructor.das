options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module export_constructor shared private

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost

[structure_macro(name="export_constructor")]
class ExportConstructor : AstStructureAnnotation
    //! implements 'export_constructor' macro, adds function make`{StructureName} which makes a new instance of a class or structure
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if args |> length!=0
            errors := "expecting [export_constructor] to have no arguments"
            return false
        var inscope qfn <- qmacro_function("make`{st.name}") <|
            return new <$t(st)>()
        qfn.flags |= FunctionFlags exports
        add_function(st._module, qfn)
        return true
