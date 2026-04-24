options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/ast_boost

def do_ast_dump_expression(expression : string) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    let script_path = make_temp_das_file()
    let code = "options gen2\noptions rtti\nrequire daslib/ast\nrequire daslib/ast_boost\n\n[export]\ndef main() \{\n    var inscope ed <- quote({expression})\n    print(\"\{debug_expression(ed)\}\n\")\n\}\n"
    var write_ok = false
    fopen(script_path, "wb") <| $(fw) {
        if (fw == null) {
            return
        }
        write_ok = true
        fw |> fprint(code)
    }
    if (!write_ok) {
        return make_tool_result("Cannot write temp file", true)
    }
    var output : string
    let exit_code = run_and_capture("{exe} {script_path}", output)
    remove(script_path)
    if (exit_code != 0) {
        return make_tool_result("AST dump failed (exit code {exit_code}):\n{output}", true)
    }
    return make_tool_result(output)
}

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

def do_ast_dump_file(file, func_name : string; use_source_mode, use_lineinfo : bool; project : string = "") : string {
    return compile_program(file, true, false, project) <| $(program, issues) {
        let thisMod = get_this_module(program)
        var result = ""
        var found = false
        for_each_function(thisMod, "") <| $(func) {
            if (func.flags.builtIn || func.flags._lambda || func.flags.generated) {
                return
            }
            if (!empty(func_name) && string(func.name) != func_name) {
                return
            }
            found = true
            result += "def {func.name}"
            var has_args = false
            for (i in range(length(func.arguments))) {
                if (!arg_needs_documenting(func.arguments[i]._type)) {
                    continue
                }
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
            if (!empty(func_name)) {
                return "Function '{func_name}' not found in {file}"
            }
            return "No user functions found in {file}"
        }
        if (!empty(issues)) {
            result += "Warnings:\n{issues}"
        }
        return result
    }
}

def do_ast_dump(expression, file, func_name, mode, lineinfo : string; project : string = "") : string {
    if (!empty(expression)) {
        return do_ast_dump_expression(expression)
    } elif (!empty(file)) {
        let use_source_mode = mode == "source"
        let use_lineinfo = lineinfo == "true"
        return do_ast_dump_file(file, func_name, use_source_mode, use_lineinfo, project)
    } else {
        return make_tool_result("Provide either 'expression' or 'file' argument", true)
    }
}
