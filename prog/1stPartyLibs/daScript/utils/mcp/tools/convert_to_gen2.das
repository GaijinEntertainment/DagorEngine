options gen2

require common public

// Strip the "input file=..." diagnostic line das-fmt prints to stdout
def strip_das_fmt_header(output : string) : string {
    if (starts_with(output, "input file=")) {
        let nl = find(output, "\n")
        if (nl >= 0) return slice(output, nl + 1)
        return ""
    }
    return output
}

def do_convert_to_gen2(file : string; inplace : bool) : string {
    if (empty(file)) return make_tool_result("missing 'file' argument", true)
    // check file exists
    var file_ok = false
    fopen(file, "rb") $(f) {
        if (f != null) {
            file_ok = true
        }
    }
    if (!file_ok) return make_tool_result("file not found: {file}", true)
    let args <- get_command_line_arguments()
    if (empty(args)) return make_tool_result("Cannot determine executable path", true)
    // das-fmt lives next to daslang.exe
    let exe = args[0]
    let exe_dir = dir_name(exe)
    let fmt_exe = "{exe_dir}/das-fmt"
    let inplace_flag = inplace ? " -i" : ""
    let cmd = "{fmt_exe} -v2{inplace_flag} {file}"
    var output : string
    let exit_code = unsafe(popen(cmd) $(f) {
            if (f == null) {
                output = "Failed to execute: {cmd}"
                return
            }
            output = build_string() $(var w) {
                while (!feof(f)) {
                    write(w, fgets(f))
                }
            }
        }
    )
    output = strip_das_fmt_header(output)
    if (exit_code != 0) return make_tool_result("Conversion failed (exit code {exit_code}):\n{output}", true)
    if (inplace) return make_tool_result("Converted {file} to gen2 syntax in place")
    return make_tool_result(output)
}
