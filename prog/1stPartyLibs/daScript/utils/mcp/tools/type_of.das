options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/ast_cursor
require daslib/ast_boost

def public do_type_of(file : string; line_str, col_str : string; no_opt_str : string = ""; project : string = "") : string {
    let line = to_int(line_str)
    let col = to_int(col_str)
    if (line <= 0 || col <= 0) {
        return make_tool_result("Invalid line or column (must be >= 1)", true)
    }
    let no_opt = no_opt_str == "true"
    return compile_program(file, true, no_opt, project) <| $(program; issues) {
        var hits <- find_at_cursor(program, file, line, col)
        if (empty(hits)) {
            return "No expression found at {file}:{line}:{col}"
        }
        return build_string() <| $(var w) {
            for (hit in hits) {
                write(w, "{hit.rtti}")
                if (!empty(hit.name)) {
                    write(w, "({hit.name})")
                }
                if (hit.expr != null && hit.expr._type != null) {
                    write(w, " : {describe(hit.expr._type)}")
                    // add extra detail for struct/enum types
                    let td = get_ptr(hit.expr._type)
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

// reuse lineinfo_file from goto_definition — but since modules are separate, define locally
def lineinfo_file(at : LineInfo) : string {
    if (at.fileInfo != null) {
        return string(at.fileInfo.name)
    }
    return ""
}
