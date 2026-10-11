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
        peek_data(ln) $(d) {
            var keep = length(d)
            while (keep > 0 && (int(d[keep - 1]) == '\n' || int(d[keep - 1]) == '\r')) {
                keep--
            }
            ln = slice(d, 0, keep)
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

def variable_definition(variable : Variable const?) : DefinitionResult {
    let file = lineinfo_file(variable.at)
    let line = int(variable.at.line)
    return DefinitionResult(
        found = true,
        kind = "variable",
        name = source_name(string(variable.name)),
        file = file,
        line = line,
        column = int(variable.at.column),
        snippet = read_source_snippet(file, line)
    )
}

//! the definition a declaration is; the cursor's own word column when the declaration sits on the cursor `line`
def definition_of(d : DeclarationHit; line : int) : DefinitionResult {
    return DefinitionResult() if (d.kind == DeclarationKind.none)
    if (d.kind == DeclarationKind.variable) return variable_definition(d.variable)
    if (d.kind == DeclarationKind.handled) return DefinitionResult(found = true, kind = "handled", name = d.name, cppName = string(d.annotation.cppName))
    let kind = d.kind == DeclarationKind.func ? "function" : d.kind == DeclarationKind.structure ? "struct" : d.kind == DeclarationKind.enumeration ? "enum" : d.kind == DeclarationKind.field ? "field" : "typedef"
    let decl_file = lineinfo_file(d.at)
    let decl_line = int(d.at.line)
    return DefinitionResult(
        found = true,
        kind = kind,
        name = d.kind == DeclarationKind.func ? string(d.func.name) : d.name,
        file = decl_file,
        line = decl_line,
        column = decl_line == line && d.word_column >= 0 ? d.word_column : declared_name_column(d.at, d.name),
        snippet = read_source_snippet(decl_file, decl_line)
    )
}

def type_definition(program : smart_ptr<Program>; td : TypeDecl const?) : DefinitionResult {
    return definition_of(declaration_of(program, td), 0)
}

def resolve_definition(program : smart_ptr<Program>; hit : CursorHit) : DefinitionResult { // nolint:STYLE037,STYLE038 — exact-type `is` ladder over the AST node classes; each arm builds one DefinitionResult
    if (hit.variable != null) return variable_definition(hit.variable)
    if (hit.type_decl != null) return type_definition(program, hit.type_decl)
    let expr = hit.expr
    // ExprVar → variable declaration
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.variable != null) {
            return variable_definition(v.variable)
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
        let field_name = string(expr is ExprField ? (expr as ExprField).name : (expr as ExprSafeField).name)
        let st = value_struct(expr is ExprField ? (expr as ExprField).value : (expr as ExprSafeField).value)
        if (st != null) {
            let method = find_method_function(st, field_name)
            if (method != null) {
                let file = lineinfo_file(method.at)
                let line = int(method.at.line)
                return DefinitionResult(found = true, kind = "function", name = string(method.name), file = file, line = line,
                    column = int(method.at.column), snippet = read_source_snippet(file, line))
            }
            let at = field_at(st, field_name)
            if (int(at.line) > 0) {
                let file = lineinfo_file(at)
                let line = int(at.line)
                return DefinitionResult(found = true, kind = "field", name = "{st.name}.{field_name}", file = file, line = line,
                    column = int(at.column), snippet = read_source_snippet(file, line))
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
    // Fallback: resolve type reference from _type
    if (expr._type != null) {
        return type_definition(program, expr._type)
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
    let col0 = col - 1
    if (line <= 0 || col0 < 0) return make_tool_result("Invalid line or column (must be >= 1)", true)
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
        let decl = definition_of(find_declaration_at_cursor(program, file, line, col0, source_line(file, line)), line)
        if (decl.found) return format_result(decl, CursorHit(), with_cpp_source, cpp_dirs_arr)
        var hits <- find_at_cursor(program, file, line, col0, true, source_line(file, line))
        if (empty(hits)) return "No expression found at {file}:{line}:{col}"
        for (hit in hits) {
            let defn = resolve_definition(program, hit)
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
