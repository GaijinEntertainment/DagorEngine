options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/lint
require daslib/perf_lint
require daslib/style_lint
require daslib/lint_config
require daslib/ast_boost
require strings

//! Subprocess form of `lint`. Argv: <file> <project>.

def has_expect_directive(file : string) : bool {
    let text = fread(file)
    return false if (empty(text))
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        return true if (trimmed |> starts_with("expect "))
    }
    return false
}

def lint_single(file : string; project : string;
                disabled_codes, enabled_codes : table<string>) : string {
    return make_tool_result("SKIP {file} (intentional compile errors via `expect`)") if (has_expect_directive(file))
    return compile_only(file, true, true, project, true) $(program; issues) {
        var compile_warnings = ""
        if (!empty(issues)) {
            compile_warnings = "Compilation warnings:\n{issues}\n"
        }
        var all_issues : array<string>
        var count = paranoid_collect(program, all_issues, disabled_codes, enabled_codes)
        var perf_issues : array<string>
        count += perf_lint_collect(program, perf_issues, disabled_codes, enabled_codes)
        for (w in perf_issues) {
            all_issues |> push(w)
        }
        var style_issues : array<string>
        let hygiene = program._options |> find_arg("_comment_hygiene") ?as tBool ?? false
        count += style_lint_collect(program, style_issues, disabled_codes, enabled_codes, hygiene)
        for (w in style_issues) {
            all_issues |> push(w)
        }
        return "{compile_warnings}No lint issues." if (count == 0)
        return build_string() $(var w) {
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

// Caller must have already validated `project` via validate_project_arg.
// Multi-file callers do this once up front so an invalid project surfaces
// the focused error instead of N misleading FAIL lines (the per-file
// FAIL path drops result.errors).
def lint_file(file : string; project : string;
              disabled_codes, enabled_codes : table<string>) : LintFileResult {
    var result = LintFileResult(file = file)
    var inscope access <- make_file_access(project)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; _issues) {
                if (!ok) {
                    result.failed = true
                } else {
                    result.count = paranoid_collect(program, result.errors, disabled_codes, enabled_codes)
                    var perf_issues : array<string>
                    result.count += perf_lint_collect(program, perf_issues, disabled_codes, enabled_codes)
                    for (w in perf_issues) {
                        result.errors |> push(w)
                    }
                    var style_issues : array<string>
                    let hygiene = program._options |> find_arg("_comment_hygiene") ?as tBool ?? false
                    result.count += style_lint_collect(program, style_issues, disabled_codes, enabled_codes, hygiene)
                    for (w in style_issues) {
                        result.errors |> push(w)
                    }
                }
            }
        }
    }
    return <- result
}

def run_lint(file : string; project : string = "") : string {
    let proj_err = validate_project_arg(project)
    return make_tool_result(proj_err, true) if (!empty(proj_err))
    var files : array<string>
    parse_file_list(file, files)
    return make_tool_result("no files matched: {file}", true) if (empty(files))
    // Repo-level policy is invariant for the whole invocation — read .lint_config once.
    var disabled_codes : table<string>
    let enabled_codes : table<string>
    seed_default_disabled(disabled_codes)
    load_lint_config(disabled_codes)
    return lint_single(files[0], project, disabled_codes, enabled_codes) if (length(files) == 1)
    var total_issues = 0
    var total_errors = 0
    var total_skipped = 0
    let output = build_string() $(var w) {
        for (f in files) {
            if (has_expect_directive(f)) {
                total_skipped++
                w |> write("SKIP {f} (intentional compile errors via `expect`)\n")
                continue
            }
            let result = lint_file(f, project, disabled_codes, enabled_codes)
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
        if (total_skipped > 0) {
            w |> write("\n{length(files)} files, {total_issues} issue(s), {total_errors} error(s), {total_skipped} skipped\n")
        } else {
            w |> write("\n{length(files)} files, {total_issues} issue(s), {total_errors} error(s)\n")
        }
    }
    return make_tool_result(output, total_errors > 0)
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 2) {
        print(make_tool_result("lint subtool: expected 2 args (file, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let project = args[1]
    print(run_lint(file, project))
}
