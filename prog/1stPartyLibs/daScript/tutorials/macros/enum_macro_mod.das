options gen2
options no_aot

// Tutorial macro module: enumeration macro (AstEnumerationAnnotation).
//
// Provides [enum_total] — an enumeration annotation that adds a "total"
// entry to any enum, set to the number of original values.
//
// AstEnumerationAnnotation has a single method:
//   apply(var st, var group, args, var errors) : bool
//     Called BEFORE the infer pass — can modify the enum or generate code.
//
// This macro demonstrates modifying the enum itself by adding a new
// entry with add_enumeration_entry.

module enum_macro_mod

require daslib/ast
require daslib/ast_boost

[enumeration_macro(name="enum_total")]
class EnumTotalAnnotation : AstEnumerationAnnotation {
    def override apply(var enu : EnumerationPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        // Check that the enum doesn't already have a "total" entry.
        for (ee in enu.list) {
            if (ee.name == "total") {
                errors := "enumeration already has a 'total' field"
                return false
            }
        }
        // Add a new "total" entry at the end.
        let idx = add_enumeration_entry(enu, "total")
        if (idx < 0) {
            errors := "failed to add 'total' field"
            return false
        }
        // Set total = number of original entries (before we added "total").
        // length(enu.list) is now N+1, so total = N+1-1 = N.
        enu.list[idx].value |> move_new() <| new ExprConstInt(at = enu.at, value = length(enu.list) - 1)
        return true
    }
}
