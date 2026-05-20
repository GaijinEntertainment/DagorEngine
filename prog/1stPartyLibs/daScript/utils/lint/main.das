options gen2

require daslib/lint
require daslib/perf_lint
require daslib/style_lint
require daslib/ast_boost
require daslib/fio
require daslib/clargs
require math
require strings
require daslib/strings_boost

struct LintResult {
    file : string
    count : int
    errors : array<string>
    failed : bool
    compile_errors : string
}

[CommandLineArgs]
struct Config {
    @clarg_short = "q"
    @clarg_doc = "Suppress PASS lines and progress messages"
    quiet : bool

    @clarg_doc = "Enable STYLE005 postfix-conditional check"
    postfix_conditionals : bool

    @clarg_doc = "Enable STYLE014/STYLE015 comment-length checks"
    comment_hygiene : bool

    @clarg_doc = "Only run paranoid lint (skip perf and style)"
    paranoid_only : bool

    @clarg_doc = "Only run performance lint (skip paranoid and style)"
    perf_only : bool

    @clarg_doc = "Only run style lint (skip paranoid and perf)"
    style_only : bool

    @clarg_short = "?"
    @clarg_doc = "Show this help and exit"
    help : bool
}

def write_deduped(var writer : StringBuilderWriter; messages : array<string>; prefix : string = "  ") {
    var counts : table<string; int>
    var order : array<string>
    for (msg in messages) {
        if (key_exists(counts, msg)) {
            counts[msg]++
        } else {
            counts[msg] = 1
            order |> push(msg)
        }
    }
    for (msg in order) {
        let n = counts[msg]
        if (n > 1) {
            writer |> write("{prefix}{msg} (x{n})\n")
        } else {
            writer |> write("{prefix}{msg}\n")
        }
    }
}

def is_skip_file(name : string) : bool {
    return name == "builtin.das"
}

def is_skip_dir(name : string) : bool {
    return name |> starts_with(".")
}

def has_expect_directive(file : string) : bool {
    // Files with `expect <code>:<count>` declare intentional compile errors
    // for dastest. Lint cannot evaluate them — the program never reaches
    // the AST stage paranoid/perf/style passes need. Skip them.
    let text = fread(file)
    if (empty(text)) {
        return false
    }
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        if (trimmed |> starts_with("expect ")) {
            return true
        }
    }
    return false
}

def scan_das_files(path : string; var files : array<string>; var cache : table<string; void?>) {
    let st = stat(path)
    if (!st.is_valid) {
        return
    }
    if (st.is_reg) {
        if (path |> ends_with(".das") && !cache |> key_exists(path)) {
            // extract filename from path
            let last_slash = max(rfind(path, "/"), rfind(path, "\\"))
            let fname = last_slash >= 0 ? slice(path, last_slash + 1) : path
            if (!is_skip_file(fname)) {
                cache |> insert(path, null)
                files |> push(path)
            }
        }
        return
    }
    if (!st.is_dir) {
        return
    }
    fio::dir(path) $(name) {
        if (name == "." || name == "..") {
            return
        }
        if (name |> starts_with("_") || is_skip_dir(name)) {
            return
        }
        let full = "{path}/{name}"
        let fst = stat(full)
        if (!fst.is_valid) {
            return
        }
        if (fst.is_dir) {
            scan_das_files(full, files, cache)
        } elif (fst.is_reg && name |> ends_with(".das") && !is_skip_file(name) && !cache |> key_exists(full)) {
            cache |> insert(full, null)
            files |> push(full)
        }
    }
}

def lint_file(file : string; run_paranoid, run_perf, run_style, postfix_conditionals, comment_hygiene : bool) : LintResult {
    var result = LintResult(file = file)
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    result.failed = true
                    result.compile_errors = string(issues)
                    return
                }
                if (run_paranoid) {
                    var paranoid_issues : array<string>
                    result.count += paranoid_collect(program, paranoid_issues)
                    for (w in paranoid_issues) {
                        result.errors |> push(w)
                    }
                }
                if (run_perf) {
                    var perf_issues : array<string>
                    result.count += perf_lint_collect(program, perf_issues)
                    for (w in perf_issues) {
                        result.errors |> push(w)
                    }
                }
                if (run_style) {
                    var style_issues : array<string>
                    result.count += style_lint_collect(program, style_issues, postfix_conditionals, comment_hygiene)
                    for (w in style_issues) {
                        result.errors |> push(w)
                    }
                }
            }
        }
    }
    return <- result
}

[export]
def main() : int {
    var cfg = Config()
    let err = parse_args(cfg)
    if (cfg.help) {
        print("Unified lint checker for daslang files.\n")
        print("Runs paranoid, performance, and style lint passes with recursive directory scanning.\n\n")
        print_help(get_command_info(type<Config>), "utils/lint/main.das")
        print("\nPositional arguments: one or more files or directories to lint.\n")
        return 0
    }
    if (err != "") {
        print("error: {err}\n\n")
        print_help(get_command_info(type<Config>), "utils/lint/main.das")
        return 1
    }
    let cli_args <- get_user_args()
    var paths : array<string>
    for (arg in cli_args) {
        if (!starts_with(arg, "-")) {
            paths |> push(arg)
        }
    }
    if (length(paths) == 0) {
        print("Error: no files or directories specified\n\n")
        print_help(get_command_info(type<Config>), "utils/lint/main.das")
        return 1
    }
    let any_only = cfg.paranoid_only || cfg.perf_only || cfg.style_only
    let run_paranoid = !any_only || cfg.paranoid_only
    let run_perf = !any_only || cfg.perf_only
    let run_style = !any_only || cfg.style_only
    // collect files
    var files : array<string>
    var cache : table<string; void?>
    for (p in paths) {
        scan_das_files(p, files, cache)
    }
    if (length(files) == 0) {
        print("Error: no .das files found\n")
        return 1
    }
    if (!cfg.quiet) {
        print("Linting {length(files)} file(s)...\n\n")
    }
    var total_issues = 0
    var total_errors = 0
    var total_skipped = 0
    for (file in files) {
        if (!cfg.quiet) {
            print("checking {file}...\n")
        }
        if (has_expect_directive(file)) {
            total_skipped++
            if (!cfg.quiet) {
                print("SKIP {file} (intentional compile errors via `expect`)\n")
            }
            continue
        }
        let result = lint_file(file, run_paranoid, run_perf, run_style, cfg.postfix_conditionals, cfg.comment_hygiene)
        if (result.failed) {
            total_errors++
            print("FAIL {file}\n")
            if (!empty(result.compile_errors)) {
                print("  {result.compile_errors}\n")
            }
        } elif (result.count > 0) {
            total_issues += result.count
            print("WARN {file} ({result.count})\n")
            print(build_string() $(var w) {
                write_deduped(w, result.errors)
            })
        } elif (!cfg.quiet) {
            print("PASS {file}\n")
        }
    }
    print("\n--- Summary ---\n")
    if (total_skipped > 0) {
        print("{length(files)} files, {total_issues} issue(s), {total_errors} error(s), {total_skipped} skipped\n")
    } else {
        print("{length(files)} files, {total_issues} issue(s), {total_errors} error(s)\n")
    }
    if (total_errors > 0) {
        return 1
    } elif (total_issues > 0) {
        return 2
    } else {
        return 0
    }
}
