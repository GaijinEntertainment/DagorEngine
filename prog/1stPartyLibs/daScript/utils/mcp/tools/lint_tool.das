options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/lint
require daslib/perf_lint
require strings

def do_lint_single(file : string; project : string = "") : string {
    return compile_only(file, true, true, project) <| $(program; issues) {
        var compile_warnings = ""
        if (!empty(issues)) {
            compile_warnings = "Compilation warnings:\n{issues}\n"
        }
        var all_issues : array<string>
        var count = paranoid_collect(program, all_issues)
        var perf_issues : array<string>
        count += perf_lint_collect(program, perf_issues)
        for (w in perf_issues) {
            all_issues |> push(w)
        }
        if (count == 0) {
            return "{compile_warnings}No lint issues."
        }
        return build_string() <| $(var w) {
            if (!empty(compile_warnings)) {
                w |> write(compile_warnings)
            }
            w |> write("{count} lint issue(s):\n")
            write_deduped(w, all_issues)
        }
    }
}

struct LintFileResult {
    file : string
    count : int
    errors : array<string>
    failed : bool
}

def do_lint_file(file : string; project : string = "") : LintFileResult {
    var result = LintFileResult(file = file)
    var inscope access <- make_file_access(project)
    using() <| $(var mg : ModuleGroup) {
        using() <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.no_optimizations = true
            compile_file(file, access, unsafe(addr(mg)), cop) <| $(ok; program; issues) {
                if (!ok) {
                    result.failed = true
                } else {
                    result.count = paranoid_collect(program, result.errors)
                    var perf_issues : array<string>
                    result.count += perf_lint_collect(program, perf_issues)
                    for (w in perf_issues) {
                        result.errors |> push(w)
                    }
                }
            }
        }
    }
    return <- result
}

def do_lint(file : string; project : string = "") : string {
    var files : array<string>
    parse_file_list(file, files)
    if (empty(files)) {
        return make_tool_result("no files matched: {file}", true)
    }
    if (length(files) == 1) {
        return do_lint_single(files[0], project)
    }
    // batch mode
    var total_issues = 0
    var total_errors = 0
    let output = build_string() <| $(var w) {
        for (f in files) {
            let result = do_lint_file(f, project)
            if (result.failed) {
                total_errors++
                w |> write("FAIL {f}\n")
            } elif (result.count > 0) {
                total_issues += result.count
                w |> write("WARN {f} ({result.count})\n")
                write_deduped(w, result.errors)
            } else {
                w |> write("PASS {f}\n")
            }
        }
        w |> write("\n{length(files)} files, {total_issues} issue(s), {total_errors} error(s)\n")
    }
    return make_tool_result(output, total_errors > 0)
}
