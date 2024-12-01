options no_aot = true

module _typeinfo_macro_example

require ast
require daslib/ast_boost

[typeinfo_macro(name="alias_name")]
class TypeInfoGetAliasName : AstTypeInfoMacro
    def override getAstChange ( expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : ExpressionPtr
        if expr.typeexpr != null
            return <- new [[ExprConstString() at=expr.at, value:=expr.typeexpr.alias]]
        errors := "type is missing or not inferred"
        return <- [[ExpressionPtr]]

