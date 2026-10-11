options gen2

require common public

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
    // the converter is a daslang script in the tree, run through this same daslang
    let exe = args[0]
    let converter = "{get_das_root()}/utils/gen1-to-gen2/main.das"
    let inplace_flag = inplace ? " -i" : ""
    let cmd = "{exe} \"{converter}\" -- -p \"{file}\"{inplace_flag}"
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
    if (exit_code != 0) return make_tool_result("Conversion failed (exit code {exit_code}):\n{output}", true)
    if (inplace) return make_tool_result("Converted {file} to gen2 syntax in place")
    return make_tool_result(output)
}
