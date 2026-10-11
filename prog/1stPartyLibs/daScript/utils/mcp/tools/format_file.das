options gen2

require common public
require daslib/das_source_formatter
require daslib/json_boost
require daslib/fio
require daslib/lint_config

struct FormatResult {
    file : string
    status : string
    message : string
    comments_stripped : bool
}

def private format_file_single(file : string; project, project_root : string; load_modules : array<string>) : FormatResult {
    var result = FormatResult(file = file)
    let policy = format_policy_for(file)
    if (!policy.format_enabled) {
        result.status = "skipped"
        result.message = "formatting disabled by .lint_config: {file}"
        return result
    }
    var unformatted = ""
    var formatted = ""
    var stripped_any = false
    var read_ok = false
    fopen(file, "rb") $(fr) {
        if (fr == null) {
            return
        }
        read_ok = true
        fmap(fr) $(data) {
            unformatted = string(data)
            formatted = format_source(data, policy.force_clean_comments)
            if (policy.force_clean_comments) {
                stripped_any = formatted != format_source(data)
            }
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
    if (!fwrite(file, formatted)) {
        result.status = "error"
        result.message = "Cannot write file: {file}"
        return result
    }
    if (stripped_any) {
        let verify = run_mcp_subtool("compile_check", [file, project, "false"], project_root, load_modules)
        var envelope : ToolResult
        if (!sscan_json(verify, envelope) || envelope.isError) {
            let restored = fwrite(file, unformatted)
            result.status = restored ? "reverted" : "error"
            var why = ""
            if (!empty(envelope.content)) {
                let txt = envelope.content[0].text
                let nl = find(txt, '\n')
                why = " ({nl < 0 ? txt : slice(txt, 0, nl)})"
            }
            if (restored) {
                result.message = "Comment strip broke compile_check{why}; original restored: {file}"
            } else {
                result.message = "Comment strip broke compile_check{why}; RESTORE WRITE FAILED - file left stripped: {file}"
            }
            return result
        }
    }
    result.status = "formatted"
    result.comments_stripped = stripped_any
    result.message = stripped_any ? "Formatted (comments stripped): {file}" : "Formatted: {file}"
    return result
}

def do_format_file(file : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    var files : array<string>
    parse_file_list(file, files)
    if (empty(files)) {
        let err = [FormatResult(file = file, status = "error", message = "no files matched: {file}")]
        return make_tool_result(sprint_json(err, false), true)
    }
    var results : array<FormatResult>
    results |> reserve(length(files))
    var any_errors = false
    var stripped_count = 0
    for (f in files) {
        let r = format_file_single(f, project, project_root, load_modules)
        if (r.status == "error" || r.status == "reverted") {
            any_errors = true
        }
        if (r.comments_stripped) {
            stripped_count++
        }
        results |> push(r)
    }
    if (stripped_count > 0) {
        results |> push(FormatResult(status = "rescue_advisory",
            message = "comments were deleted from {stripped_count} file(s) — before committing, run the harvester agent over the strip diff of the entries with comments_stripped = true"))
    }
    return make_tool_result(sprint_json(results, false), any_errors)
}
