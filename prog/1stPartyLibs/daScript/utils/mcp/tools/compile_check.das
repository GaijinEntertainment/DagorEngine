options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/json_boost

struct CompileResult {
    file : string
    success : bool
    errors : string
    warnings : string
}

def compile_check_single(file : string; project : string = "") : string {
    return compile_and_simulate(file, project) <| $(program; issues) {
        var warnings = ""
        if (!empty(issues)) {
            warnings = "Warnings:\n{issues}\n"
        }
        var funcs = build_string() <| $(var writer) {
            write_function_list(writer, program)
        }
        return "{warnings}Compilation OK.\n{funcs}"
    }
}

def private compile_check_single_json(file : string; project : string = "") : CompileResult {
    var result = CompileResult(file = file)
    var inscope access <- make_file_access(project)
    using() <| $(var mg : ModuleGroup) {
        using() <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            compile_file(file, access, unsafe(addr(mg)), cop) <| $(ok; program; issues) {
                if (!ok) {
                    result.errors = string(issues)
                } else {
                    simulate(program) <| $(sok; ctx; serrors) {
                        if (!sok) {
                            result.errors = string(serrors)
                        } else {
                            result.success = true
                            if (!empty(issues)) {
                                result.warnings = string(issues)
                            }
                        }
                    }
                }
            }
        }
    }
    return result
}

def do_compile_check(file : string; project : string = ""; json : bool = false) : string {
    var files : array<string>
    parse_file_list(file, files)
    if (empty(files)) {
        if (json) {
            return make_tool_result("[]", true)
        }
        return make_tool_result("no files matched: {file}", true)
    }
    if (!json) {
        // Original text behavior
        if (length(files) == 1) {
            return compile_check_single(files[0], project)
        }
        var passed = 0
        var failed = 0
        var output = build_string() <| $(var w) {
            for (f in files) {
                let result = compile_check_single(f, project)
                let is_error = find(result, "\"isError\":true") >= 0
                if (is_error) {
                    failed++
                    write(w, "FAIL {f}\n")
                } else {
                    passed++
                    write(w, "PASS {f}\n")
                }
            }
            write(w, "\n{passed + failed} files, {passed} passed, {failed} failed\n")
        }
        return make_tool_result(output, failed > 0)
    }
    // JSON mode
    var results : array<CompileResult>
    results |> reserve(length(files))
    for (f in files) {
        results |> emplace(compile_check_single_json(f, project))
    }
    return make_tool_result(sprint_json(results, false))
}
