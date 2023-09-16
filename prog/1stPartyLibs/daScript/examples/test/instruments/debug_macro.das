options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true

module debug_macro shared private

require daslib/debug
require daslib/ast_debug

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost

// require daslib/debug

[tag_function(debug_function)]
def public debug_print ( message:string="" )
    print(message)

[tag_function_macro(tag="debug_function")]
class TestDebugMacro : AstFunctionAnnotation

    def show_AnnotationList ( list:AnnotationList )
        print("AnnotationList is {list}\n")
        show_AnnotationDeclaration(list[0])
        show_AnnotationDeclaration([[AnnotationDeclarationPtr]])

    def show_AnnotationDeclaration ( decl:smart_ptr<AnnotationDeclaration> )
        print("AnnotationList is {decl}\n")
        if decl != null
            show_AnnotationArgument(decl.arguments)

    def show_AnnotationArgument ( arg:AnnotationArgumentList )
        print("AnnotationArgumentList is {arg}\n")

    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        // TypeDeclPtr
        var t : TypeDeclPtr <- call._type
        print("TypeDeclPtr is {t}\n")
        // FunctionPtr
        var fn = call.func
        print("FunctionPtr is {fn}\n")
        // VariablePtr
        var vptr : Variable? = get_ptr(call.func.arguments[0])
        print("VariablePtr is {vptr}\n")
        // AnnotationList
        show_AnnotationList(call.func.annotations)
        // ExpressionPtr
        var expr <- qmacro_expr() <|
            print($e(call.arguments[0]))
        expr |> force_at(call.at)
        return expr


