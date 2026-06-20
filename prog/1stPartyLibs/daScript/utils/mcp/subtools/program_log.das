options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/ast_boost

//! Subprocess form of program_log. Argv (after daslang exe + script path):
//!   <file> <func_name> <project>
//! Prints the make_tool_result envelope to stdout.

def run_program_log(file, func_name : string; project : string = "") : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    // filter to specific function — use ast_dump source mode approach
    if (!empty(func_name)) return compile_and_simulate(file, project) $(program, _issues) {
        let thisMod = get_this_module(program)
        var result = ""
        var found = false
        for_each_function(thisMod, "") $(func) {
            if (func.flags.builtIn || func.flags._lambda || func.flags.generated
                || func.name != func_name) return
            found = true
            result += describe_function(func)
            result += "\n"
        }
        if (!found) return "Function '{func_name}' not found in {file}"
        return result
    }
    // full program log — use C++ Program::operator<<
    return compile_and_simulate(file, project) $(program, issues) {
        var result = describe_program(program)
        if (!empty(issues)) {
            result += "\nWarnings:\n{issues}"
        }
        return result
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 3) {
        print(make_tool_result("program_log subtool: expected 3 args (file, func_name, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let func_name = args[1]
    let project = args[2]
    print(run_program_log(file, func_name, project))
}
