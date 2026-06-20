// Helper module for tests/language/test_field_decl_flags.das.
//
// Declares [check_add_field_flags(abstract=..., inherited=...)] — a structure_macro
// that walks st.fields looking for "add", and macro_errors if its _abstract /
// inherited bits don't match the annotation arguments. The compile succeeding
// is the test: any mismatch turns into a compile-time error.
options gen2
options indenting = 4

module _field_decl_flags_helper public

require daslib/ast public
require daslib/ast_boost

[structure_macro(name = "check_add_field_flags")]
class private CheckAddFieldFlagsMacro : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        let expected_abstract = find_arg(args, "is_abstract") ?as tBool ?? false
        let expected_inherited = find_arg(args, "is_inherited") ?as tBool ?? false
        var found = false
        for (fld in st.fields) {
            if (fld.name != "add") continue
            found = true
            let got_abstract = fld.flags._abstract
            let got_inherited = fld.flags.inherited
            if (got_abstract != expected_abstract) {
                errors := "[check_add_field_flags] {st.name}.add._abstract = {got_abstract}, expected {expected_abstract}"
                return false
            }
            if (got_inherited != expected_inherited) {
                errors := "[check_add_field_flags] {st.name}.add.inherited = {got_inherited}, expected {expected_inherited}"
                return false
            }
        }
        if (!found) {
            errors := "[check_add_field_flags] {st.name} has no 'add' field"
            return false
        }
        return true
    }
}
