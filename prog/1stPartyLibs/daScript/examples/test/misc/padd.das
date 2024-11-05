module padd

require daslib/ast_boost
require daslib/macro_boost
require daslib/templates_boost

[type_macro(name="padd_dim")]
class PaddDimMacro : AstTypeMacro
    def override visit ( prog:ProgramPtr; mod:Module?; td:TypeDeclPtr ) : TypeDeclPtr
        if length(td.dimExpr) != 3
            macro_error(compiling_program(), td.at, "expecting 2 arguments")
            return <- [[TypeDeclPtr()]]
        if !(td.dimExpr[2] is ExprConstInt)
            macro_error(compiling_program(), td.at, "expecting constant integer (size)")
            return <- [[TypeDeclPtr()]]
        let count = (td.dimExpr[2] as ExprConstInt).value
        if td.dimExpr[1]._type == null
            // type is not inferred, which means we are inferring generic type
            var inscope auto_type : TypeDeclPtr
            if td.dimExpr[1] is ExprTypeDecl
                auto_type |> move_new <| clone_type((td.dimExpr[1] as ExprTypeDecl).typeexpr)
            else
                auto_type |> move_new <| new [[TypeDecl() baseType=Type autoinfer]]
            auto_type.dim |> push(count)
            return <- auto_type
        var inscope final_type <- clone_type(td.dimExpr[1]._type)
        final_type.dim |> push(count)
        return <- final_type

