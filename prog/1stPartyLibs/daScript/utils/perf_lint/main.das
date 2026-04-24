options gen2

require daslib/perf_lint
require daslib/ast_boost
require daslib/fio
require strings
require daslib/strings_boost

[export]
def main() {
    let args <- get_command_line_arguments()
    let sep = find_index(args, "--") + 1
    if (sep <= 0 || sep >= length(args)) {
        print("Usage: daslang utils/perf_lint/main.das -- file1.das [file2.das ...] [--quiet]\n")
        print("\nPerformance lint checker for daslang files.\n")
        print("Compiles each file and reports performance anti-patterns.\n")
        print("\nRules:\n")
        print("  PERF001  string += in loop (O(n^2))\n")
        print("  PERF002  character_at in loop with loop variable index\n")
        print("  PERF003  character_at anywhere (info)\n")
        print("  PERF004  string interpolation reassignment in loop (O(n^2))\n")
        print("  PERF005  length(string) in while condition\n")
        print("  PERF006  push/emplace in loop without reserve()\n")
        print("  PERF007  unnecessary string(das_string) in comparison\n")
        print("  PERF008  unnecessary get_ptr() for is/as type checks\n")
        print("  PERF009  redundant move-init variable immediately returned\n")
        unsafe {
            fio::exit(1)
        }
        return
    }
    var quiet = false
    var files : array<string>
    files |> reserve(length(args) - sep)
    for (i in range(sep, length(args))) {
        if (args[i] == "--quiet") {
            quiet = true
        } else {
            files |> push(args[i])
        }
    }
    if (length(files) == 0) {
        print("Error: no files specified\n")
        unsafe {
            fio::exit(1)
        }
        return
    }
    var total_warnings = 0
    var total_files = 0
    var failed_files = 0
    for (file in files) {
        if (!quiet) {
            print("checking {file}...\n")
        }
        total_files++
        {
            var inscope access <- make_file_access("")
            using() <| $(var mg : ModuleGroup) {
                using() <| $(var cop : CodeOfPolicies) {
                    cop.ignore_shared_modules = true
                    cop.export_all = true
                    cop.no_optimizations = true
                    compile_file(file, access, unsafe(addr(mg)), cop) <| $(ok; program; issues) {
                        if (!ok) {
                            let issues_str = string(issues)
                            print("  COMPILE ERROR:\n{issues_str}\n")
                            failed_files++
                        } else {
                            // Run perf_lint directly on the compiled program
                            total_warnings += perf_lint(program, false)
                        }
                    }
                }
            }
        }
    }
    if (!quiet || total_warnings > 0 || failed_files > 0) {
        print("\n--- Summary ---\n")
        print("Files checked: {total_files}\n")
        if (failed_files > 0) {
            print("Compile errors: {failed_files}\n")
        }
        print("Performance warnings: {total_warnings}\n")
    }
    unsafe {
        fio::exit(failed_files > 0 ? 1 : 0)
    }
}
