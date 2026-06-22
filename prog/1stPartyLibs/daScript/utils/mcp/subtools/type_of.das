options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/ast_cursor
require daslib/ast_boost

//! Subprocess form of type_of. Argv (after daslang exe + script path):
//!   <file> <line> <col> <no_opt> <project>
//! Prints the make_tool_result envelope to stdout.

def lineinfo_file(at : LineInfo) : string {
    if (at.fileInfo != null) return string(at.fileInfo.name)
    return ""
}

def run_type_of(file : string; line_str, col_str, no_opt_str, project : string) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    let line = to_int(line_str)
    let col = to_int(col_str)
    if (line <= 0 || col <= 0) return make_tool_result("Invalid line or column (must be >= 1)", true)
    let no_opt = no_opt_str == "true"
    return compile_program(file, true, no_opt, project) $(program; _issues) {
        var hits <- find_at_cursor(program, file, line, col)
        if (empty(hits)) return "No expression found at {file}:{line}:{col}"
        return build_string() $(var w) {
            for (hit in hits) {
                write(w, "{hit.rtti}")
                if (!empty(hit.name)) {
                    write(w, "({hit.name})")
                }
                if (hit.expr != null && hit.expr._type != null) {
                    write(w, " : {describe(hit.expr._type)}")
                    // add extra detail for struct/enum types
                    let td = hit.expr._type
                    if (td.structType != null) {
                        let st = td.structType
                        let st_file = lineinfo_file(st.at)
                        if (!empty(st_file)) {
                            write(w, "  // {st_file}:{int(st.at.line)}")
                        }
                    }
                    if (td.enumType != null) {
                        let en = td.enumType
                        let en_file = lineinfo_file(en.at)
                        if (!empty(en_file)) {
                            write(w, "  // {en_file}:{int(en.at.line)}")
                        }
                    }
                } else {
                    write(w, " : (no type)")
                }
                write(w, "\n")
            }
        }
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 5) {
        print(make_tool_result("type_of subtool: expected 5 args (file, line, col, no_opt, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let line_str = args[1]
    let col_str = args[2]
    let no_opt_str = args[3]
    let project = args[4]
    print(run_type_of(file, line_str, col_str, no_opt_str, project))
}
