options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/das_source_formatter

def do_format_file(file : string) : string {
    var unformatted = ""
    var formatted = ""
    var read_ok = false
    fopen(file, "rb") <| $(fr) {
        if (fr == null) {
            return
        }
        read_ok = true
        fmap(fr) <| $(data) {
            unformatted = string(data)
            formatted = format_source(data)
        }
    }
    if (!read_ok) {
        return make_tool_result("Cannot open file: {file}", true)
    }
    if (empty(formatted)) {
        return make_tool_result("Formatting produced empty output for: {file}", true)
    }
    if (formatted == unformatted) {
        return make_tool_result("File already formatted: {file}")
    }
    var write_ok = false
    fopen(file, "wb") <| $(fw) {
        if (fw == null) {
            return
        }
        write_ok = true
        fw |> fprint(formatted)
    }
    if (!write_ok) {
        return make_tool_result("Cannot write file: {file}", true)
    }
    return make_tool_result("Formatted: {file}")
}
