module _enum_total

require daslib/ast_boost
require daslib/strings_boost

[enumeration_macro(name="enum_total")]
class EnumTotalAnnotation : AstEnumerationAnnotation
    def override apply ( var enu:EnumerationPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        for ee in enu.list
            if ee.name=="total"
                errors := "enumeration already has `total` field"
                return false
        let itot = add_enumeration_entry(enu,"total")
        if itot < 0
            errors := "can't add `total` field"
            return false
        enu.list[itot].value |> move_new() <| new [[ExprConstInt() at=enu.at, value=length(enu.list)-1]]
        return true
