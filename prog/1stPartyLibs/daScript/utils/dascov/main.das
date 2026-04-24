options gen2

options no_coverage

require daslib/coverage
require daslib/ast_boost
require daslib/debugger
require daslib/fio
require strings

def public compile_and_simulate(input : string, var access; var mg : ModuleGroup?, cop, blk) {
    //! Compiles a daslang file, simulates it, and invokes a callback with the program and context.
    compile_file(input, access, mg, cop) $(ok; var program : smart_ptr<Program>; issues) {
        if (!ok) {
            print("failed to compile {input}\n{issues}\n")
            return
        }
        simulate(program) $(sok; var pctx : smart_ptr<Context>; serrors) {
            if (!sok) {
                panic("Failed to simulate {serrors}")
            }
            blk(program, pctx)
        }
    }
}


[export]
def main() {
    // Usage:
    //   ./bin/daslang utils/dascov/main.das -- <lcov_file> [--exclude <prefix>]...
    //     - print merged coverage summary from an existing LCOV file
    //     - --exclude skips files starting with <prefix> (e.g. --exclude tests/)
    //   ./bin/daslang utils/dascov/main.das -- <output file> <regular command>
    //     - if output file == `-` print short statistic to stdout
    //     - if output file is a regular file print to file in lcov format.
    let args <- get_command_line_arguments()
    let sep = find_index(args, "--") + 1
    let numArgs = args |> length() - sep
    // Collect --exclude prefixes.
    var excludes : array<string>
    for (i in range(sep, args |> length() - 1)) {
        if (args[i] == "--exclude") {
            excludes |> push(args[i + 1])
        }
    }
    if (numArgs == 1 || (numArgs >= 2 && args[sep + 1] == "--exclude")) {
        // LCOV summary mode: parse existing LCOV file and print summary.
        print(coverage::summary_from_lcov_file(args[sep], excludes))
        return
    }
    var coverage_file = args[sep]
    var input_file = args[sep + 1]
    var new_args = array<string>()
    for (i in range(args |> length())) {
        if (i == 1) {
            new_args |> push(input_file)
        } elif (i == sep + 1 || i == sep) {
            // Do nothing
        } else {
            new_args |> push(args[i])
        }
    }
    using <| $(var cop : CodeOfPolicies) {
        using() $(var mg : ModuleGroup) {
            var inscope access <- make_file_access("")
            cop.threadlock_context = true
            access |> add_extra_module("coverage", "{get_das_root()}/daslib/coverage.das")
            var report : string
            compile_and_simulate(input_file, access, unsafe(addr(mg)), cop) $(program : ProgramPtr; var pctx : smart_ptr<Context>) {
                with_argv(new_args) {
                    unsafe(invoke_in_context(pctx, "main"))
                }
                if (coverage_file == "-") {
                    var rep = ""
                    unsafe(invoke_in_context(pctx, "fill_coverage_summary", unsafe(addr(rep)), true))
                    // Clone to current context.
                    report = clone_string(rep)
                    print(report)
                } else {
                    var rep = ""
                    unsafe(invoke_in_context(pctx, "fill_coverage_report", unsafe(addr(rep)), ""))
                    // Clone to current context.
                    report = clone_string(rep)
                    fopen(coverage_file, "wb") <| $(fw) {
                        if (fw != null) {
                            to_log(LOG_INFO, "Coverage to {coverage_file}\n")
                            fwrite(fw, report)
                        } else {
                            to_log(LOG_ERROR, "Couldn't create output file {coverage_file}\n")
                        }
                    }
                }
            }
        }
    }
}
