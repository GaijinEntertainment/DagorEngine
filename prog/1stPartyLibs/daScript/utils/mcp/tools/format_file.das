options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/das_source_formatter
require daslib/json_boost

struct FormatResult {
    file : string
    status : string  // "formatted" | "already_formatted" | "error"
    message : string
}

def private format_file_single(file : string) : FormatResult {
    var result = FormatResult(file = file)
    var unformatted = ""
    var formatted = ""
    var read_ok = false
    fopen(file, "rb") $(fr) {
        if (fr == null) {
            return
        }
        read_ok = true
        fmap(fr) $(data) {
            unformatted = string(data)
            formatted = format_source(data)
        }
    }
    if (!read_ok) {
        result.status = "error"
        result.message = "Cannot open file: {file}"
        return result
    }
    if (empty(formatted)) {
        result.status = "error"
        result.message = "Formatting produced empty output for: {file}"
        return result
    }
    if (formatted == unformatted) {
        result.status = "already_formatted"
        result.message = "File already formatted: {file}"
        return result
    }
    var write_ok = false
    fopen(file, "wb") $(fw) {
        if (fw == null) {
            return
        }
        write_ok = true
        fw |> fprint(formatted)
    }
    if (!write_ok) {
        result.status = "error"
        result.message = "Cannot write file: {file}"
        return result
    }
    result.status = "formatted"
    result.message = "Formatted: {file}"
    return result
}

def do_format_file(file : string) : string {
    var files : array<string>
    parse_file_list(file, files)
    if (empty(files)) {
        let err = [FormatResult(file = file, status = "error", message = "no files matched: {file}")]
        return make_tool_result(sprint_json(err, false), true)
    }
    var results : array<FormatResult>
    results |> reserve(length(files))
    var any_errors = false
    for (f in files) {
        let r = format_file_single(f)
        if (r.status == "error") {
            any_errors = true
        }
        results |> push(r)
    }
    return make_tool_result(sprint_json(results, false), any_errors)
}
