options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/ast_boost

//! Subprocess form of ast_dump (file-based path). Argv (after daslang exe + script path):
//!   <file> <func_name> <mode> <lineinfo> <project>
//! Prints the make_tool_result envelope to stdout.
//! The expression-based path stays in tools/ast_dump.das because it already
//! spawns its own short-lived subprocess via a generated stub.

def format_func_body(func; use_source_mode : bool; use_lineinfo : bool = false) : string {
    if (use_source_mode) {
        return describe_expression(func.body)
    } else {
        var deFlags = bitfield(0)
        if (use_lineinfo) {
            deFlags = DebugExpressionFlags.lineInfo
        }
        return debug_expression(func.body, deFlags)
    }
}

def run_ast_dump_file(file, func_name : string; use_source_mode, use_lineinfo : bool; project : string = "") : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    return compile_program(file, true, false, project) $(program, issues) {
        let thisMod = get_this_module(program)
        var result = ""
        var found = false
        for_each_function(thisMod, "") $(func) {
            if (func.flags.builtIn || func.flags._lambda || func.flags.generated
                || (!empty(func_name) && func.name != func_name)) return
            found = true
            result += "def {func.name}"
            var has_args = false
            for (i in range(length(func.arguments))) {
                if (!arg_needs_documenting(func.arguments[i]._type)) continue
                if (!has_args) {
                    result += "("
                    has_args = true
                } else {
                    result += "; "
                }
                result += "{func.arguments[i].name} : {describe(func.arguments[i]._type)}"
            }
            if (has_args) {
                result += ")"
            }
            if (func.result != null && !func.result.isVoid) {
                result += " : {describe(func.result)}"
            }
            result += "\n"
            result += format_func_body(func, use_source_mode, use_lineinfo)
            result += "\n\n"
        }
        if (!found) {
            if (!empty(func_name)) return "Function '{func_name}' not found in {file}"
            return "No user functions found in {file}"
        }
        if (!empty(issues)) {
            result += "Warnings:\n{issues}"
        }
        return result
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 5) {
        print(make_tool_result("ast_dump subtool: expected 5 args (file, func_name, mode, lineinfo, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let func_name = args[1]
    let mode = args[2]
    let lineinfo = args[3]
    let project = args[4]
    let use_source_mode = mode == "source"
    let use_lineinfo = lineinfo == "true"
    print(run_ast_dump_file(file, func_name, use_source_mode, use_lineinfo, project))
}
