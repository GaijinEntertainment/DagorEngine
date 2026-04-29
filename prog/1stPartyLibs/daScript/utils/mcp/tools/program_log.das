options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/ast_boost

def do_program_log(file, func_name : string; project : string = "") : string {
    if (!empty(func_name)) {
        // filter to specific function — use ast_dump source mode approach
        return compile_program(file, true, false, project) <| $(program, issues) {
            let thisMod = get_this_module(program)
            var result = ""
            var found = false
            for_each_function(thisMod, "") <| $(func) {
                if (func.flags.builtIn || func.flags._lambda || func.flags.generated) {
                    return
                }
                if (string(func.name) != func_name) {
                    return
                }
                found = true
                result += describe_function(func)
                result += "\n"
            }
            if (!found) {
                return "Function '{func_name}' not found in {file}"
            }
            return result
        }
    }
    // full program log — use C++ Program::operator<<
    return compile_program(file, true, false, project) <| $(program, issues) {
        var result = describe_program(program)
        if (!empty(issues)) {
            result += "\nWarnings:\n{issues}"
        }
        return result
    }
}
