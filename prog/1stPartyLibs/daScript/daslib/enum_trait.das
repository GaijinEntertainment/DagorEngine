options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module enum_trait shared private

require ast
require daslib/ast_boost

[typeinfo_macro(name="enum_length")]
class TypeInfoGetEnumLength : AstTypeInfoMacro
    //! Implements typeinfo(enum_length EnumOrEnumType) which returns total number of elements in enumeration.
    def override getAstChange ( expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : ExpressionPtr
        if expr.typeexpr != null
            if !expr.typeexpr.isEnum
                errors := "expecting enumeration"
                return <- [[ExpressionPtr]]
            let sz = length(expr.typeexpr.enumType.list)
            return <- new [[ExprConstInt() at=expr.at, value=sz]]
        errors := "type is missing or not inferred"
        return <- [[ExpressionPtr]]

[typeinfo_macro(name="enum_names")]
class TypeInfoGetEnumNames : AstTypeInfoMacro
    //! Implements typeinfo(enum_names EnumOrEnumType) which returns array of strings with enumValue names.
    def override getAstChange ( expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : ExpressionPtr
        if expr.typeexpr == null
            errors := "type is missing or not inferred"
            return <- [[ExpressionPtr]]
        if !expr.typeexpr.isEnum
            errors := "expecting enumeration"
            return <- [[ExpressionPtr]]
        var inscope arr <- new [[ExprMakeArray() at=expr.at, makeType <- typeinfo(ast_typedecl type<string>)]]
        for i in iter_range(expr.typeexpr.enumType.list)
            if true
                assume name = expr.typeexpr.enumType.list[i].name
                var inscope nameExpr <- new [[ExprConstString() at=expr.at, value:=name]]
                arr.values |> emplace <| nameExpr
        return <- arr
