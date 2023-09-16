options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module type_traits shared private

require daslib/ast_boost

[typeinfo_macro(name="fields_count")]
class TypeInfoGetFieldsNum : AstTypeInfoMacro
    //! this macro implements "fields_count" type trait, which returns total number of fields in the structure
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr
        if expr.typeexpr != null
            if !expr.typeexpr.isStructure
                errors := "expecting structure"
                return <- [[ExpressionPtr]]
            let fieldsNum = length(expr.typeexpr.structType.fields)
            return <- new [[ExprConstInt() at=expr.at, value=fieldsNum]]
        errors := "type is missing or not inferred"
        return <- [[ExpressionPtr]]


[typeinfo_macro(name="safe_has_property")]
class TypeInfoHasProperty : AstTypeInfoMacro
    //! this macro implements "has_property" type trait, which returns true when structure has a property
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr
        if expr.typeexpr != null
            if !expr.typeexpr.isHandle && !expr.typeexpr.isStructure
                errors := "expecting handle type or structure. typeinfo(safe_has_property<propName> v)"
                return <- [[ExpressionPtr]]
            if empty(expr.subtrait)
                errors := "expecting trait string. typeinfo(safe_has_property<propName> v)"
                return <- [[ExpressionPtr]]

            let propName = ".`{expr.subtrait}"
            var res = false
            get_ptr(compiling_program()) |> for_each_module() <| $(mod)
                if res
                    return
                mod |> for_each_function(propName) <| $(fn)
                    if fn.moreFlags.propertyFunction && expr.typeexpr |> is_same_type(fn.arguments[0]._type, RefMatters yes, ConstMatters no, TemporaryMatters no)
                        res = true

            return <- new [[ExprConstBool() at=expr.at, value=res]]
        errors := "type is missing or not inferred"
        return <- [[ExpressionPtr]]
