options gen2
// options log_infer_passes = true
// options log_generics = true
options rtti

require fio
require rtti
require ast
require debugapi
require daslib/ast_boost
require daslib/cpp_bind


[export]
def main {
    let root = get_das_root() + "/src/builtin/"
    fopen(root + "ast_gen.inc", "wb") <| $(cpp_file) {
        log_cpp_class_adapter(cpp_file, "AstFunctionAnnotation_Adapter", typeinfo ast_typedecl(type<AstFunctionAnnotation>))
        log_cpp_class_adapter(cpp_file, "AstBlockAnnotation_Adapter", typeinfo ast_typedecl(type<AstBlockAnnotation>))
        log_cpp_class_adapter(cpp_file, "AstStructureAnnotation_Adapter", typeinfo ast_typedecl(type<AstStructureAnnotation>))
        log_cpp_class_adapter(cpp_file, "AstEnumerationAnnotation_Adapter", typeinfo ast_typedecl(type<AstEnumerationAnnotation>))
        log_cpp_class_adapter(cpp_file, "AstPassMacro_Adapter", typeinfo ast_typedecl(type<AstPassMacro>))
        log_cpp_class_adapter(cpp_file, "AstVarianMacro_Adapter", typeinfo ast_typedecl(type<AstVariantMacro>))
        log_cpp_class_adapter(cpp_file, "AstForLoopMacro_Adapter", typeinfo ast_typedecl(type<AstForLoopMacro>))
        log_cpp_class_adapter(cpp_file, "AstCaptureMacro_Adapter", typeinfo ast_typedecl(type<AstCaptureMacro>))
        log_cpp_class_adapter(cpp_file, "AstTypeMacro_Adapter", typeinfo ast_typedecl(type<AstTypeMacro>))
        log_cpp_class_adapter(cpp_file, "AstSimulateMacro_Adapter", typeinfo ast_typedecl(type<AstSimulateMacro>))
        log_cpp_class_adapter(cpp_file, "AstReaderMacro_Adapter", typeinfo ast_typedecl(type<AstReaderMacro>))
        log_cpp_class_adapter(cpp_file, "AstCommentReader_Adapter", typeinfo ast_typedecl(type<AstCommentReader>))
        log_cpp_class_adapter(cpp_file, "AstCallMacro_Adapter", typeinfo ast_typedecl(type<AstCallMacro>))
        log_cpp_class_adapter(cpp_file, "AstTypeInfoMacro_Adapter", typeinfo ast_typedecl(type<AstTypeInfoMacro>))
        log_cpp_class_adapter(cpp_file, "AstVisitor_Adapter", typeinfo ast_typedecl(type<AstVisitor>))
    }
    fopen(root + "debugapi_gen.inc", "wb") <| $(cpp_file) {
        log_cpp_class_adapter(cpp_file, "DapiDataWalker_Adapter", typeinfo ast_typedecl(type<DapiDataWalker>))
        log_cpp_class_adapter(cpp_file, "DapiDebugAgent_Adapter", typeinfo ast_typedecl(type<DapiDebugAgent>))
        log_cpp_class_adapter(cpp_file, "DapiStackWalker_Adapter", typeinfo ast_typedecl(type<DapiStackWalker>))
    }
    return true
}

