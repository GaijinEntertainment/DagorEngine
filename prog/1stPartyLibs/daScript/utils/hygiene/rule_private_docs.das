options gen2

module rule_private_docs shared private

require strings
require daslib/regex public
require daslib/regex_boost
require daslib/strings_boost


var private HEADER_RE <- %regex~^\s*(?:def|struct|enum|variant|class)\s+private\b%%
var private TRAILING_RE <- %regex~\s*//!<?[^\n]*$%%


def private count_braces(s : string; var block_depth : int&) : tuple<int; int> {
    // Returns (opens, closes) of `{`/`}` outside string and char literals,
    // `//` line comments, and `/* ... */` block comments (which nest).
    // `block_depth` is in/out: caller passes the depth at line start; on
    // return it reflects the depth at line end (0 = not inside a block).
    var opens = 0
    var closes = 0
    var in_string = false
    var quote = uint8(0)
    var escape = false
    var prev_slash = false
    var prev_star = false
    peek_data(s) $(arr) {
        for (b in arr) {
            if (block_depth > 0) {
                // Inside a block comment — only `*/` (close) and `/*` (nest)
                // matter; strings, escapes, `//` are inert.
                if (prev_star && b == uint8('/')) {
                    block_depth--
                    prev_star = false
                    prev_slash = false
                } elif (prev_slash && b == uint8('*')) {
                    block_depth++
                    prev_slash = false
                    prev_star = false
                } else {
                    prev_star = b == uint8('*')
                    prev_slash = b == uint8('/')
                }
                continue
            }
            if (escape) {
                escape = false
                prev_slash = false
                prev_star = false
                continue
            }
            if (in_string) {
                if (b == uint8('\\')) {
                    escape = true
                } elif (b == quote) {
                    in_string = false
                }
                prev_slash = false
                prev_star = false
                continue
            }
            if (b == uint8('"') || b == uint8('\'')) {
                in_string = true
                quote = b
                prev_slash = false
                prev_star = false
                continue
            }
            if (prev_slash && b == uint8('/')) return  // `//` line comment — rest of line is comment text
            if (prev_slash && b == uint8('*')) {
                block_depth = 1
                prev_slash = false
                prev_star = false
                continue
            }
            prev_slash = b == uint8('/')
            prev_star = false
            if (b == uint8('{')) {
                opens++
            } elif (b == uint8('}')) {
                closes++
            }
        }
    }
    return (opens, closes)
}


def private is_private_decl_start(line : string) : bool {
    return regex_match(HEADER_RE, line) >= 0
}


def private strip_trailing_doc(line : string) : string {
    let pos = regex_search(TRAILING_RE, line)
    if (pos.x < 0) return line
    let new_body = slice(line, 0, pos.x)
    if (empty(strip(new_body))) return ""
    return new_body
}


def public trim_private_docstrings(src : string) : string {
    // The brace-depth counter is reliable once string/char literals, `//`
    // line comments, and `/* ... */` block comments (which nest in daslang)
    // are skipped — see `count_braces`. `block_depth` is threaded across
    // lines so multi-line block comments are handled correctly.
    var lines <- split(src, "\n")
    // Detect line-ending style so CRLF input round-trips byte-for-byte instead
    // of being silently rewritten to LF on every run.
    var crlf = false
    for (raw_line in lines) {
        if (ends_with(raw_line, "\r")) {
            crlf = true
            break
        }
    }
    let nl = crlf ? "\r\n" : "\n"
    var out : array<string>
    var state = 0   // 0 = normal, 1 = awaiting_brace, 2 = in_body
    var body_depth = 0
    var block_depth = 0
    for (raw_line in lines) {
        let line = ends_with(raw_line, "\r") ? slice(raw_line, 0, length(raw_line) - 1) : raw_line
        // If the line starts inside a `/* ... */` block comment, no
        // line-level decision (private-decl detect, //! strip) applies —
        // the contents are inert. Just walk to update block_depth and push.
        if (block_depth > 0) {
            count_braces(line, block_depth)
            out |> push(line)
            continue
        }
        if (state == 0) {
            out |> push(line)
            if (is_private_decl_start(line)) {
                let braces = count_braces(line, block_depth)
                if (braces._0 > 0) {
                    body_depth = braces._0 - braces._1
                    state = body_depth > 0 ? 2 : 0
                } else {
                    state = 1
                }
            } else {
                count_braces(line, block_depth)
            }
            continue
        }
        if (state == 1) {
            out |> push(line)
            let braces = count_braces(line, block_depth)
            if (braces._0 > 0) {
                body_depth = braces._0 - braces._1
                state = body_depth > 0 ? 2 : 0
            }
            continue
        }
        // state == 2 (inside a private declaration body)
        let stripped = strip(line)
        if (starts_with(stripped, "//!")) continue
        let new_line = strip_trailing_doc(line)
        if (empty(new_line)) continue
        let braces = count_braces(new_line, block_depth)
        body_depth += braces._0 - braces._1
        out |> push(new_line)
        if (body_depth <= 0) {
            state = 0
        }
    }
    return out |> join(nl)
}
