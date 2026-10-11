options gen2
options indenting = 4

module export_c shared private !inscope

require daslib/ast_boost
require strings


def private is_c_identifier(name : string) : bool {
    if (name |> empty()) {
        return false
    }
    var first = true
    for (ch in name) {
        let alpha = is_alpha(ch) || ch == '_'
        if (!(alpha || (is_number(ch) && !first))) {
            return false
        }
        first = false
    }
    return true
}


[function_macro(name = "export_c")]
class private ExportCAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        for (arg in func.arguments) {
            if (arg._type.baseType == Type.autoinfer && arg.init == null) {
                errors := "[export_c] can't export generic function `{func.name}`: it has auto or template arguments, and C needs one concrete signature"
                return false
            }
        }
        for (arg in args) {
            if (arg.name != "name") {
                errors := "[export_c] unknown argument `{arg.name}`; the only argument is name=\"<c identifier>\""
                return false
            }
            if (arg.basicType != Type.tString || !is_c_identifier(string(arg.sValue))) {
                errors := "[export_c] name must be a valid C identifier ([A-Za-z_][A-Za-z0-9_]*), got `{arg.sValue}`"
                return false
            }
        }
        func.flags.exports = true
        return true
    }
}
