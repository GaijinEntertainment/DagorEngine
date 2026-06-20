options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/json_boost

//! Subprocess form of compile_check. Argv (after daslang exe + script path):
//!   <file> <project> <json>
//! where <json> is the literal "true" or "false". Prints the make_tool_result
//! envelope to stdout. Exits 0 on success (including tool-level "compile
//! failed" — those come back as isError=true inside the envelope).
//!
//! Lives here (not in tools/) because the in-process logic must NOT be
//! reachable from tools/compile_check.das, which is now a thin popen wrapper.
//! Putting it here breaks any circular import path through the wrapper.

struct CompileResult {
    file : string
    success : bool
    errors : string
    warnings : string
}

def compile_check_single(file : string; project : string = "") : string {
    return compile_and_simulate(file, project) $(program; issues) {
        var warnings = ""
        if (!empty(issues)) {
            warnings = "Warnings:\n{issues}\n"
        }
        let funcs = build_string() $(var writer) {
            write_function_list(writer, program)
        }
        return "{warnings}Compilation OK.\n{funcs}"
    }
}

// Caller must have already validated `project` via validate_project_arg.
// Hoisted to run_compile_check so an invalid project surfaces the focused
// error once instead of N times (or, in the non-JSON FAIL path, getting
// dropped entirely while only `FAIL <file>` lines print).
def private compile_check_single_json(file : string; project : string = "") : CompileResult {
    var result = CompileResult(file = file)
    var inscope access <- make_file_access(project)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    result.errors = string(issues)
                } else {
                    simulate(program) $(sok; _ctx; serrors) {
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

def run_compile_check(file : string; project : string = ""; json : bool = false) : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    var files : array<string>
    parse_file_list(file, files)
    if (empty(files)) {
        if (json) return make_tool_result("[]", true)
        return make_tool_result("no files matched: {file}", true)
    }
    if (!json) {
        if (length(files) == 1) return compile_check_single(files[0], project)
        var passed = 0
        var failed = 0
        let output = build_string() $(var w) {
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
    var results : array<CompileResult>
    results |> reserve(length(files))
    for (f in files) {
        results |> emplace(compile_check_single_json(f, project))
    }
    return make_tool_result(sprint_json(results, false))
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 3) {
        print(make_tool_result("compile_check subtool: expected 3 args (file, project, json), got {length(args)}", true))
        return
    }
    let file = args[0]
    let project = args[1]
    let json = args[2] == "true"
    print(run_compile_check(file, project, json))
}
