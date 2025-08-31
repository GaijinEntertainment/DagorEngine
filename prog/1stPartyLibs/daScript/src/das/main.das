require ast/ast_aot_cpp
require ast/standalone_contexts

require fio
require strings

require daslib/rtti


def updateCOP(var cop : CodeOfPolicies; gen2 : bool; gen2_make : bool) {
    cop.threadlock_context = false
    cop.aot = false;
    cop.aot_module = true;
    cop.fail_on_lack_of_aot_export = true;
    cop.ignore_shared_modules = true;
    cop.version_2_syntax = gen2;
    cop.gen2_make_syntax = gen2_make;
}


def find_argument_or(args : array<string>; key : string, def_val : string) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            return args[i + 1]
        }
    }
    return def_val
}

def find_argument(args : array<string>; key : string) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            return args[i + 1]
        }
    }
    panic("expected {key} argument!")
    return ""
}

def find_bool(args : array<string>; key : string) {
    let idx = find_index(args, key)
    return idx >= 0 && idx + 1 < length(args)
}

[export]
def main() {
    let args <- get_command_line_arguments()

    let mode = find_argument(args, "--mode");
    let cross_platform = find_bool(args, "-cross_platform");
    let gen1 = find_bool(args, "-gen1");
    let gen2_make = find_bool(args, "-gen2-make");
    using <| $(var cop : CodeOfPolicies) {
        updateCOP(cop, !gen1, gen2_make)
        if (mode == "aot") {
            let input = find_argument(args, "--input");
            let output = find_argument(args, "--output");
            print("aot {input} to {output}\n")
            let res = aot(input, true, false, cross_platform, cop)
            if (res |> length() > 0) {
                fopen(output, "wb") <| $(fw) {
                    if (fw != null) {
                        fwrite(fw, res)
                    } else {
                        print("Couldn't create output file {output}")
                    }
                }
            }
        } elif (mode == "standalone_ctx") {
            let input = find_argument(args, "--input");
            let output = find_argument(args, "--output");
            print("standalone ctx {input} to {output} cross_platform={cross_platform}\n")
            let res = standalone_aot(input, output, false, cross_platform, false, cop)
            if (res |> length() > 0) {
                fopen(output, "wb") <| $(fw) {
                    if (fw != null) {
                        fwrite(fw, res)
                    } else {
                        print("Couldn't create output file {output}")
                    }
                }
            }
        } elif (mode == "ast_print") {
            panic("unimplemented")
        } else {
            panic("Unknown mode {mode}")
        }
    }
}
