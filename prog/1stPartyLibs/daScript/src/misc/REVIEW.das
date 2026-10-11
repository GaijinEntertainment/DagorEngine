options gen2

require strings
require daslib/strings_boost
require daslib/fio
require dastest/review_gate

// The mechanical half of src/misc/REVIEW.md (contract: REVIEW_COMMON.md at the repo root).
// Run from the repo root: bin/daslang src/misc/REVIEW.das - exit 0 clean, 1 with findings.

let private NETWORK_CPP = "src/misc/network.cpp"
let private ERROR_SOURCE = "last_socket_error"
let private WOULD_BLOCK_SOURCE = "socket_would_block"

def private is_ident_byte(b : int) : bool {
    return is_alnum(b) || b == '_'
}

// Byte offset of the first whole-word occurrence of `tok` in `line`; -1 when absent.
def private token_index(line : string; tok : string) : int {
    var at = -1
    let tlen = length(tok)
    peek_data(line) $(d) {
        let n = length(d)
        var from = 0
        while (at < 0) {
            let idx = find(d, tok, from)
            break if (idx < 0)
            let before_ok = idx == 0 || !is_ident_byte(int(d[idx - 1]))
            let after_ok = idx + tlen >= n || !is_ident_byte(int(d[idx + tlen]))
            if (before_ok && after_ok) {
                at = idx
            }
            from = idx + 1
        }
    }
    return at
}

def private has_token(line : string; tok : string) : bool {
    return token_index(line, tok) >= 0
}

def private is_write_of(line : string; tok : string) : bool {
    let idx = find(line, tok)
    return false if (idx < 0)
    let rest = strip(slice(line, idx + length(tok)))
    return starts_with(rest, "=") && !starts_with(rest, "==")
}

def private brace_delta(line : string) : int {
    var delta = 0
    peek_data(line) $(d) {
        for (b in d) {
            if (b == '{') {
                delta++
            } elif (b == '}') {
                delta--
            }
        }
    }
    return delta
}

// 1-based line range of the function whose definition line names `fname` and opens a brace;
// int2(0, 0) when the file defines no such function. A call site has a `(` before the name,
// a definition has only its return type there.
def private body_range(lines : array<string>; fname : string) : int2 {
    for (i, ln in range(length(lines)), lines) {
        let at = token_index(ln, fname)
        continue if (at < 0 || find(ln, "\{") < 0 || find(ln, ";") >= 0
            || find(slice(ln, 0, at), "(") >= 0 || find(slice(ln, at), "(") < 0)
        var depth = 0
        for (j in range(i, length(lines))) {
            depth += brace_delta(lines[j])
            if (depth <= 0) {
                return int2(i + 1, j + 1)
            }
        }
    }
    return int2(0, 0)
}

def private inside(span : int2; line : int) : bool {
    return span.x > 0 && line >= span.x && line <= span.y
}

def private check_network_error_sources {
    if (!fexist(NETWORK_CPP)) {
        return
    }
    let text = strip_line_comments(fread(NETWORK_CPP))
    var inscope lines <- split(text, "\n")
    let error_body = body_range(lines, ERROR_SOURCE)
    let would_block_body = body_range(lines, WOULD_BLOCK_SOURCE)
    if (error_body.x == 0) {
        gate_finding(NETWORK_CPP, "no {ERROR_SOURCE}() - Winsock reports through WSAGetLastError(), and this helper is the only place that reads it")
    }
    if (would_block_body.x == 0) {
        gate_finding(NETWORK_CPP, "no {WOULD_BLOCK_SOURCE}() - Winsock returns WSAEWOULDBLOCK, and this helper is the only place that names the would-block codes")
    }
    for (i, ln in range(length(lines)), lines) {
        let line_no = i + 1
        let is_preprocessor = starts_with(strip(ln), "#")
        if (!is_preprocessor && has_token(ln, "errno") && !is_write_of(ln, "errno") && !inside(error_body, line_no)) {
            gate_finding(NETWORK_CPP, line_no, "reads errno outside {ERROR_SOURCE}() - Winsock leaves errno at 0 after a failed socket call; call {ERROR_SOURCE}()")
        }
        for (code in ["EAGAIN", "EWOULDBLOCK", "EINTR", "WSAEWOULDBLOCK"]) {
            if (has_token(ln, code) && !inside(would_block_body, line_no)) {
                gate_finding(NETWORK_CPP, line_no, "names {code} outside {WOULD_BLOCK_SOURCE}() - Winsock and POSIX disagree on the would-block code; call {WOULD_BLOCK_SOURCE}()")
                break
            }
        }
    }
}

[export]
def main() : int {
    if (!fexist("src/misc/REVIEW.das") || !fexist("CMakeLists.txt")) {
        to_log(LOG_ERROR, "src/misc/REVIEW.das: run from the repo root\n")
        return 2
    }
    check_network_error_sources()
    return gate_verdict("src/misc")
}
