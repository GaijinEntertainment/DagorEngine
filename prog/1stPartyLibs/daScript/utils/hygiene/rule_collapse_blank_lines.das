options gen2

module rule_collapse_blank_lines shared private

require strings
require daslib/strings_boost


def public collapse_blank_lines(src : string) : string {
    var lines <- split(src, "\n")
    var crlf = false
    for (raw_line in lines) {
        if (ends_with(raw_line, "\r")) {
            crlf = true
            break
        }
    }
    let nl = crlf ? "\r\n" : "\n"
    var out : array<string>
    var prev_blank = false
    for (raw_line in lines) {
        let line = ends_with(raw_line, "\r") ? slice(raw_line, 0, length(raw_line) - 1) : raw_line
        let is_blank = empty(strip(line))
        if (is_blank && prev_blank) continue
        out |> push(line)
        prev_blank = is_blank
    }
    return out |> join(nl)
}
