options gen2

require common public
require daslib/strings_boost

// Extract every `missing prerequisite '<NAME>'` occurrence from the daslang compiler's stderr.
// The compiler emits these for unresolved `require`s as `error[20605]`.
def private extract_missing_prerequisites(output : string) : array<string> {
    var names : array<string>
    let marker = "missing prerequisite '"
    var pos = 0
    while (true) {
        let i = find(output, marker, pos)
        if (i < 0) break
        let start = i + length(marker)
        let endq = find(output, "'", start)
        if (endq < 0) break
        names |> push(slice(output, start, endq))
        pos = endq + 1
    }
    return <- names
}

def private contains_string(arr : array<string>; value : string) : bool {
    for (s in arr) {
        if (s == value) return true
    }
    return false
}

// Rewrite a comma-list of module names: a token becomes `daslib/<token>` only when it is both
// unqualified AND the compiler reported it as a missing prerequisite. Other tokens stay verbatim.
// Returns (new list, true) if at least one token was rewritten, else (original, false).
def private rewrite_modules_in_set(req_modules : string; missing : array<string>) : tuple<string; bool> {
    var req_list <- split(req_modules, ",")
    var rewritten = false
    var out_parts : array<string>
    for (r in req_list) {
        let trimmed = strip(r)
        if (empty(trimmed)) continue
        if (!is_qualified_module_name(trimmed) && contains_string(missing, trimmed)) {
            out_parts |> push("daslib/{trimmed}")
            rewritten = true
        } else {
            out_parts |> push(trimmed)
        }
    }
    return join(out_parts, ", ") => rewritten
}

def private try_eval_expression(exe, expression, req_modules, project, project_root : string;
                                load_modules : array<string>; var output : string&) : int {
    let stub_path = make_temp_das_file()
    let script = build_string() $(var w) {
        write(w, "options gen2\n")
        if (!empty(req_modules)) {
            var req_list <- split(req_modules, ",")
            for (r in req_list) {
                let trimmed = strip(r)
                if (!empty(trimmed)) {
                    write(w, "require {trimmed}\n")
                }
            }
        }
        write(w, "[export]\ndef main() \{\n")
        write(w, "    let _res_ = ")
        write(w, expression)
        write(w, "\n    print(\"\{_res_\}\\n\")\n")
        write(w, "\}\n")
    }
    var write_ok = false
    fopen(stub_path, "wb") $(fw) {
        if (fw == null) return
        write_ok = true
        fw |> fwrite(script)
    }
    if (!write_ok) {
        remove(stub_path)
        output = "Cannot write temp file: {stub_path}"
        return -1
    }
    var argv <- [exe]
    if (!empty(project_root)) {
        argv |> push("-project_root")
        argv |> push(project_root)
    }
    argv |> reserve(length(argv) + 2 * length(load_modules) + 3)
    for (lm in load_modules) {
        argv |> push("-load_module")
        argv |> push(lm)
    }
    if (!empty(project)) {
        argv |> push("-project")
        argv |> push(project)
    }
    argv |> push(stub_path)
    let exit_code = run_and_capture(argv, output)
    remove(stub_path)
    return exit_code
}

def do_eval_expression(expression, req_modules : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    if (empty(expression)) return make_tool_result("missing 'expression' argument", true)
    let project_err = validate_project_arg(project)
    if (!empty(project_err)) return make_tool_result(project_err, true)
    let exe = get_daslang_exe()
    if (empty(exe)) return make_tool_result("Cannot determine daslang executable path", true)
    var output : string
    let exit_code = try_eval_expression(exe, expression, req_modules, project, project_root, load_modules, output)
    if (exit_code == 0) return make_tool_result(strip(output))
    // exit_code < 0 means try_eval_expression failed before invoking the daslang subprocess
    // (e.g. couldn't write the temp file). Surface the message directly — no retry would help.
    if (exit_code < 0) return make_tool_result(output, true)
    // Only retry under daslib/ when at least one unqualified token in `req_modules` was reported
    // by the compiler as `missing prerequisite '<X>'`. Builtin tokens that resolved (e.g. `math`)
    // stay verbatim, and qualified missing names (e.g. transitive `daslib/foo`) don't trigger a
    // retry at all — rewriting can't help them.
    if (!empty(req_modules)) {
        let missing <- extract_missing_prerequisites(output)
        let rewrite = rewrite_modules_in_set(req_modules, missing)
        let new_modules = rewrite._0
        let did_rewrite = rewrite._1
        if (did_rewrite) {
            var output2 : string
            let exit_code2 = try_eval_expression(exe, expression, new_modules, project, project_root, load_modules, output2)
            if (exit_code2 == 0) return make_tool_result("[resolved unqualified modules under daslib/: {new_modules}]\n{strip(output2)}")
            if (exit_code2 < 0) return make_tool_result(output2, true)
            // Retry also failed at the daslang level — surface the retry's output, since the rewrite
            // addressed the module-resolution side and any remaining error is downstream of that.
            return make_tool_result("Evaluation failed (exit code {exit_code2}) — also retried with require '{new_modules}':\n{output2}", true)
        }
    }
    return make_tool_result("Evaluation failed (exit code {exit_code}):\n{output}", true)
}
