options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require ../tools/cpp_common.das public
require daslib/ast_cursor
require daslib/ast_boost
require math

//! Subprocess form of goto_definition. Argv (after daslang exe + script path):
//!   <file> <line> <col> <no_opt> <with_cpp_source> <cpp_dirs> <project>
//! Prints the make_tool_result envelope to stdout.

// Result of resolving a definition location
struct DefinitionResult {
    found : bool
    kind : string       // "variable", "function", "field", "struct", "enum", "builtin"
    name : string
    file : string
    line : int
    column : int
    snippet : string    // source lines around the definition
    cppName : string    // C++ symbol name for builtin functions and handled types.
}

// Read a few lines from a file around a given line number
def read_source_snippet(file_path : string; line : int; context_lines : int = 3) : string {
    if (empty(file_path) || line <= 0) return ""
    var f = fopen(file_path, "r")
    if (f == null) return ""
    var lines : array<string>
    while (!feof(f)) {
        var ln = fgets(f)
        // strip trailing newline/cr
        while (!empty(ln)) {
            let last_ch = character_at(ln, length(ln) - 1) // nolint:PERF003
            if (last_ch == '\n' || last_ch == '\r') {
                ln = slice(ln, 0, length(ln) - 1)
            } else {
                break
            }
        }
        lines |> push(ln)
    }
    fclose(f)
    if (empty(lines)) return ""
    let first = max(0, line - 1 - context_lines)
    let last = min(length(lines) - 1, line - 1 + context_lines)
    return build_string() $(var w) {
        for (idx in range(first, last + 1)) {
            write(w, "{idx + 1}: {lines[idx]}\n")
        }
    }
}

// Get file path from LineInfo
def lineinfo_file(at : LineInfo) : string {
    if (at.fileInfo != null) return string(at.fileInfo.name)
    return ""
}

// Resolve definition from a CursorHit
def resolve_definition(hit : CursorHit) : DefinitionResult {
    let expr = hit.expr
    // ExprVar → variable declaration
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.variable != null) {
            let variable = v.variable
            let file = lineinfo_file(variable.at)
            let line = int(variable.at.line)
            return DefinitionResult(
                found = true,
                kind = "variable",
                name = string(variable.name),
                file = file,
                line = line,
                column = int(variable.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
    }
    // ExprCall / ExprCallFunc → function declaration
    if (expr is ExprCall) {
        let c = expr as ExprCall
        if (c.func != null) {
            if (c.func.flags.builtIn) {
                var cppName : string
                let bfn = c.func as BuiltInFunction
                if (bfn != null) {
                    cppName = string(bfn.cppName)
                }
                return DefinitionResult(
                    found = true,
                    kind = "builtin",
                    name = string(c.func.name),
                    snippet = build_string() $(var w) {
                        write(w, "builtin def {c.func.name}")
                        write_func_signature(w, *c.func)
                    },
                    cppName = cppName
                )
            }
            let file = lineinfo_file(c.func.at)
            let line = int(c.func.at.line)
            return DefinitionResult(
                found = true,
                kind = "function",
                name = string(c.func.name),
                file = file,
                line = line,
                column = int(c.func.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
    }
    // ExprField / ExprSafeField → struct field declaration
    if (expr is ExprField || expr is ExprSafeField) {
        var field_name : string
        var value_type : TypeDecl const?
        if (expr is ExprField) {
            let f = expr as ExprField
            field_name = string(f.name)
            if (f.value != null && f.value._type != null) {
                value_type = f.value._type
            }
        } else {
            let f = expr as ExprSafeField
            field_name = string(f.name)
            if (f.value != null && f.value._type != null) {
                value_type = f.value._type
            }
        }
        // unwrap pointer types (Foo? → Foo)
        if (value_type != null && value_type.structType == null && value_type.firstType != null) {
            value_type = value_type.firstType
        }
        if (value_type != null && value_type.structType != null) {
            let st = value_type.structType
            for (fld in st.fields) {
                if (fld.name == field_name) {
                    let file = lineinfo_file(fld.at)
                    let line = int(fld.at.line)
                    return DefinitionResult(
                        found = true,
                        kind = "field",
                        name = "{st.name}.{field_name}",
                        file = file,
                        line = line,
                        column = int(fld.at.column),
                        snippet = read_source_snippet(file, line)
                    )
                }
            }
        }
    }
    // ExprConstEnumeration → enum declaration
    if (expr is ExprConstEnumeration) {
        let e = expr as ExprConstEnumeration
        if (e.enumType != null) {
            let en = e.enumType
            let file = lineinfo_file(en.at)
            let line = int(en.at.line)
            return DefinitionResult(
                found = true,
                kind = "enum",
                name = "{en.name}.{e.value}",
                file = file,
                line = line,
                column = int(en.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
    }
    // Type alias → resolve to typedef declaration
    if (expr._type != null) {
        let td = expr._type
        if (!empty(td.alias)) {
            let file = lineinfo_file(td.at)
            let line = int(td.at.line)
            return DefinitionResult(
                found = true,
                kind = "typedef",
                name = string(td.alias),
                file = file,
                line = line,
                column = int(td.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
    }
    // Fallback: resolve type reference from _type
    if (expr._type != null) {
        let td = expr._type
        if (td.isHandle && td.annotation != null) {
            let ann = td.annotation
            return DefinitionResult(
                found = true,
                kind = "handled",
                name = string(ann.name),
                cppName = string(ann.cppName)
            )
        }
        if (td.structType != null) {
            let st = td.structType
            let file = lineinfo_file(st.at)
            let line = int(st.at.line)
            return DefinitionResult(
                found = true,
                kind = "struct",
                name = string(st.name),
                file = file,
                line = line,
                column = int(st.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
        if (td.enumType != null) {
            let en = td.enumType
            let file = lineinfo_file(en.at)
            let line = int(en.at.line)
            return DefinitionResult(
                found = true,
                kind = "enum",
                name = string(en.name),
                file = file,
                line = line,
                column = int(en.at.column),
                snippet = read_source_snippet(file, line)
            )
        }
    }
    return DefinitionResult()
}

def format_result(defn : DefinitionResult; hit : CursorHit; with_cpp_source : bool; cpp_dirs : array<string>) : string {
    return build_string() $(var w) {
        write(w, "Symbol: {defn.name}\n")
        write(w, "Kind: {defn.kind}\n")
        if (!empty(defn.file)) {
            write(w, "File: {defn.file}\n")
            write(w, "Line: {defn.line}\n")
            write(w, "Column: {defn.column}\n")
        }
        if (!empty(defn.cppName)) {
            write(w, "C++: {defn.cppName}")
            if (with_cpp_source) {
                var match : CppMatch
                let found = empty(cpp_dirs) ? cpp_lookup_by_name(defn.cppName, match) : cpp_lookup_by_name_scoped(defn.cppName, cpp_dirs, match)
                if (found) {
                    write(w, " ({match.file}:{match.line})")
                } elif (empty(cpp_dirs)) {
                    let why = cpp_index_status()
                    if (!empty(why)) {
                        write(w, " (index unavailable: {why})")
                    } else {
                        write(w, " (not located)")
                    }
                } else {
                    let dirs_csv = join(cpp_dirs, ",")
                    write(w, " (not located in {dirs_csv})")
                }
            }
            write(w, "\n")
        }
        if (hit.func != null) {
            write(w, "In function: {hit.func.name}\n")
        }
        if (!empty(defn.snippet)) {
            write(w, "\n{defn.snippet}")
        }
    }
}

def run_goto_definition(file : string; line_str, col_str, no_opt_str, with_cpp_source_str, cpp_dirs, project : string) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    let line = to_int(line_str)
    let col = to_int(col_str)
    if (line <= 0 || col <= 0) return make_tool_result("Invalid line or column (must be >= 1)", true)
    let no_opt = no_opt_str == "true"
    let with_cpp_source = with_cpp_source_str == "true"
    var cpp_dirs_arr : array<string>
    if (!empty(cpp_dirs)) {
        for (part in split(cpp_dirs, ",")) {
            let trimmed = strip(part)
            if (!empty(trimmed)) {
                cpp_dirs_arr |> push(trimmed)
            }
        }
    }
    return compile_program(file, true, no_opt, project) $(program; _issues) {
        var hits <- find_at_cursor(program, file, line, col)
        if (empty(hits)) return "No expression found at {file}:{line}:{col}"
        for (hit in hits) {
            let defn = resolve_definition(hit)
            if (defn.found) return format_result(defn, hit, with_cpp_source, cpp_dirs_arr)
        }
        return build_string() $(var w) {
            write(w, "Found expression at cursor but could not resolve definition:\n")
            for (hit in hits) {
                write(w, "  {describe(hit)}\n")
            }
        }
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 7) {
        print(make_tool_result("goto_definition subtool: expected 7 args (file, line, col, no_opt, with_cpp_source, cpp_dirs, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let line_str = args[1]
    let col_str = args[2]
    let no_opt_str = args[3]
    let with_cpp_source_str = args[4]
    let cpp_dirs = args[5]
    let project = args[6]
    print(run_goto_definition(file, line_str, col_str, no_opt_str, with_cpp_source_str, cpp_dirs, project))
}
