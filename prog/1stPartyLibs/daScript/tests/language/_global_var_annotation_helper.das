options gen2

module _global_var_annotation_helper shared private

require daslib/ast_boost
require strings

def has_arg(list : AnnotationArgumentList; name : string) : bool {
    for (a in list) {
        if (a.name == name) {
            return true
        }
    }
    return false
}

[lint_macro]
class CheckColorAnnotation : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        for_each_global(mod) $(gvar) {
            var matches = false
            peek(gvar.name) $(n : string#) {
                matches = n |> starts_with("assert_color_")
            }
            if (matches && !has_arg(gvar.annotation, "color")) {
                macro_error(prog, gvar.at,
                    "global '{gvar.name}': expected @color annotation, " +
                    "got [{describe(gvar.annotation)}]")
            }
        }
        return true
    }
}
