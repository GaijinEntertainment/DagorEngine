options gen2
options indenting = 4
options strict_smart_pointers = false

module toml shared public

//! TOML 1.0 parser.
//!
//! Reads `TOML 1.0 <https://toml.io/en/v1.0.0>`_ into the same
//! ``JsonValue?`` tree shape produced by ``daslib/json``, so existing
//! ``json_boost`` accessors (``v ?? def``, ``from_JV``, etc.) work on
//! TOML inputs as-is.
//!
//! Mapping to ``JsValue``:
//!
//!   - TOML table / inline table  → ``JsValue._object``
//!   - TOML array / array-of-tables → ``JsValue._array``
//!   - TOML string (basic / literal / multi-line forms) → ``JsValue._string`` (already unescaped)
//!   - TOML integer (decimal / hex / octal / binary) → ``JsValue._longint``
//!   - TOML float (decimal / inf / nan)            → ``JsValue._number``
//!   - TOML boolean                                 → ``JsValue._bool``
//!   - TOML date-time / local-date / local-time     → ``JsValue._string`` (preserves the
//!     RFC-3339 lexical form, since JSON has no native date type)
//!
//! Error model — fail-fast: any malformed input causes ``read_toml`` to
//! return ``null`` with a descriptive message in the ``error`` out-param,
//! matching ``daslib/json::read_json``.

require strings
require daslib/strings_boost
require daslib/utf8_utils
require daslib/math_bits
require daslib/json public

// IEEE-754 double constants. Built from bit patterns (via daslib/math_bits)
// so that the AOT generator does not constant-fold a division-by-zero literal
// into a bare `inf` token in the emitted C++.
let private {
    DBL_POS_INF = uint64_bits_to_double(0x7FF0000000000000ul)
    DBL_NEG_INF = uint64_bits_to_double(0xFFF0000000000000ul)
    DBL_NAN     = uint64_bits_to_double(0x7FF8000000000000ul)
}

// ---------------------------------------------------------------------------
// TomlToken model
// ---------------------------------------------------------------------------

variant private TomlToken {
    _symbol   : int       //! single-char punctuation: '=' '[' ']' '\{' '}' ',' '.'
    _newline  : void?     //! line terminator (significant for grammar)
    _bare     : string    //! unquoted identifier — key, or keyword-value (true/false/inf/nan)
    _string   : string    //! quoted string value (already unescaped)
    _integer  : int64     //! integer literal
    _float    : double    //! float literal
    _datetime : string    //! RFC-3339 date/time, raw lexical form
    _error    : string    //! lexer error
}

struct private TomlTokenAt {
    value : TomlToken
    line, row : int
}

let private {
    Tok_symbol   = typeinfo variant_index<_symbol>(type<TomlToken>)
    Tok_newline  = typeinfo variant_index<_newline>(type<TomlToken>)
    Tok_bare     = typeinfo variant_index<_bare>(type<TomlToken>)
    Tok_string   = typeinfo variant_index<_string>(type<TomlToken>)
    Tok_integer  = typeinfo variant_index<_integer>(type<TomlToken>)
    Tok_float    = typeinfo variant_index<_float>(type<TomlToken>)
    Tok_datetime = typeinfo variant_index<_datetime>(type<TomlToken>)
    Tok_error    = typeinfo variant_index<_error>(type<TomlToken>)
}

// ---------------------------------------------------------------------------
// Character class helpers
// ---------------------------------------------------------------------------

def private is_bare_char(c : int) : bool {
    // TOML 1.0 §Keys: A-Z / a-z / 0-9 / - / _ (ASCII-only per spec).
    return is_alpha(c) || is_number(c) || c == '-' || c == '_'
}

def private hex_value(c : int) : int {
    if (is_number(c)) return c - '0'
    if (c >= 'a' && c <= 'f') return c - 'a' + 10
    if (c >= 'A' && c <= 'F') return c - 'A' + 10
    return -1
}

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

struct private LexState {
    //! Position tracking for a peek_data-driven lexer.
    pos  : int       //! current byte offset into the source
    n    : int       //! source length
    line : int       //! 1-based current line
    row  : int       //! 0-based column on current line
}

def private at(data : array<uint8>&; st : LexState) : int {
    return st.pos < st.n ? int(data[st.pos]) : -1
}

def private peek(data : array<uint8>&; st : LexState; offset : int) : int {
    let p = st.pos + offset
    return p < st.n ? int(data[p]) : -1
}

def private advance(data : array<uint8>&; var st : LexState&) : int {
    if (st.pos >= st.n) return -1
    let c = int(data[st.pos])
    st.pos++
    if (c == '\n') {
        st.line++
        st.row = 0
    } else {
        st.row++
    }
    return c
}

def private starts_with_at(data : array<uint8>&; pos : int; s : string) : bool {
    let n = data |> length
    let m = length(s)
    if (pos + m > n) return false
    var ok = true
    peek_data(s) $(sd) {
        for (i in range(m)) {
            if (int(data[pos + i]) != int(sd[i])) {
                ok = false
                break
            }
        }
    }
    return ok
}

def private skip_inline_ws(data : array<uint8>&; var st : LexState&) : void {
    while (st.pos < st.n) {
        let c = int(data[st.pos])
        if (c == ' ' || c == '\t') {
            advance(data, st)
        } else {
            break
        }
    }
}

def private skip_comment_to_eol(data : array<uint8>&; var st : LexState&) : void {
    while (st.pos < st.n && int(data[st.pos]) != '\n') {
        advance(data, st)
    }
}

def private read_bare_key(data : array<uint8>&; var st : LexState&) : string {
    var buf : array<uint8>
    while (st.pos < st.n && is_bare_char(int(data[st.pos]))) {
        push(buf, data[st.pos])
        advance(data, st)
    }
    return string(buf)
}

def private read_basic_string(data : array<uint8>&; var st : LexState&; var err : string&) : string {
    // st.pos is just past the opening '"'
    var buf : array<uint8>
    buf |> reserve(32)   // amortize alloc churn for typical config-shape strings
    while (st.pos < st.n) {
        let c = int(data[st.pos])
        if (c == '"') {
            advance(data, st)
            return string(buf)
        }
        if (c == '\n') {
            err = "newline in basic string at {st.line}:{st.row}"
            return ""
        }
        if (c == '\\') {
            advance(data, st)
            if (st.pos >= st.n) {
                err = "string escape past end of input at {st.line}:{st.row}"
                return ""
            }
            let esc = int(data[st.pos])
            advance(data, st)
            if (esc == 'b') push(buf, uint8('\b'))
            elif (esc == 't') push(buf, uint8('\t'))
            elif (esc == 'n') push(buf, uint8('\n'))
            elif (esc == 'f') push(buf, uint8('\f'))
            elif (esc == 'r') push(buf, uint8('\r'))
            elif (esc == '"') push(buf, uint8('"'))
            elif (esc == '\\') push(buf, uint8('\\'))
            elif (esc == 'u' || esc == 'U') {
                let digits = esc == 'u' ? 4 : 8
                var cp = uint(0)
                for (_ in range(digits)) {
                    if (st.pos >= st.n) {
                        err = "incomplete \\{esc == 'u' ? "u" : "U"} escape at {st.line}:{st.row}"
                        return ""
                    }
                    let hc = int(data[st.pos])
                    let hv = hex_value(hc)
                    if (hv < 0) {
                        err = "non-hex character in \\{esc == 'u' ? "u" : "U"} escape at {st.line}:{st.row}"
                        return ""
                    }
                    cp = (cp << uint(4)) | uint(hv)
                    advance(data, st)
                }
                utf8_encode(buf, cp)
            } else {
                err = "unknown escape '\\{to_char(esc)}' at {st.line}:{st.row}"
                return ""
            }
        } else {
            push(buf, data[st.pos])
            advance(data, st)
        }
    }
    err = "unterminated basic string at {st.line}:{st.row}"
    return ""
}

def private read_ml_basic_string(data : array<uint8>&; var st : LexState&; var err : string&) : string {
    // st.pos is just past the opening """
    // Per TOML §Multi-line Basic Strings: an immediate newline after opening """ is trimmed.
    if (st.pos < st.n && int(data[st.pos]) == '\r' && st.pos + 1 < st.n && int(data[st.pos + 1]) == '\n') {
        advance(data, st); advance(data, st)
    } elif (st.pos < st.n && int(data[st.pos]) == '\n') {
        advance(data, st)
    }
    var buf : array<uint8>
    buf |> reserve(64)
    while (st.pos < st.n) {
        // Closing delimiter: 3+ quotes (greedy — up to 5 are allowed; last 3 close).
        if (int(data[st.pos]) == '"' && st.pos + 2 < st.n
                && int(data[st.pos + 1]) == '"' && int(data[st.pos + 2]) == '"') {
            advance(data, st); advance(data, st); advance(data, st)
            // Up to 2 trailing quotes belong to the string content (TOML allows up to 5 quotes).
            if (st.pos < st.n && int(data[st.pos]) == '"') {
                push(buf, uint8('"'))
                advance(data, st)
                if (st.pos < st.n && int(data[st.pos]) == '"') {
                    push(buf, uint8('"'))
                    advance(data, st)
                }
            }
            return string(buf)
        }
        let c = int(data[st.pos])
        if (c == '\\') {
            // Line-ending backslash trims newline + leading whitespace on next line.
            let nx = peek(data, st, 1)
            if (nx == '\n' || nx == '\r' || nx == ' ' || nx == '\t') {
                // Look ahead: is everything until newline whitespace?
                var look = st.pos + 1
                while (look < st.n && (int(data[look]) == ' ' || int(data[look]) == '\t')) {
                    look++
                }
                if (look < st.n && (int(data[look]) == '\n' || int(data[look]) == '\r')) {
                    // Line-ending backslash. Consume the backslash, the trailing ws,
                    // the newline, and all leading whitespace on subsequent lines.
                    advance(data, st)  // consume backslash
                    while (st.pos < st.n) {
                        let cc = int(data[st.pos])
                        if (cc == ' ' || cc == '\t' || cc == '\n' || cc == '\r') {
                            advance(data, st)
                        } else {
                            break
                        }
                    }
                    continue
                }
            }
            // Normal escape — reuse single-line logic.
            advance(data, st)
            if (st.pos >= st.n) {
                err = "string escape past end of input at {st.line}:{st.row}"
                return ""
            }
            let esc = int(data[st.pos])
            advance(data, st)
            if (esc == 'b') push(buf, uint8('\b'))
            elif (esc == 't') push(buf, uint8('\t'))
            elif (esc == 'n') push(buf, uint8('\n'))
            elif (esc == 'f') push(buf, uint8('\f'))
            elif (esc == 'r') push(buf, uint8('\r'))
            elif (esc == '"') push(buf, uint8('"'))
            elif (esc == '\\') push(buf, uint8('\\'))
            elif (esc == 'u' || esc == 'U') {
                let digits = esc == 'u' ? 4 : 8
                var cp = uint(0)
                for (_ in range(digits)) {
                    if (st.pos >= st.n) {
                        err = "incomplete \\{esc == 'u' ? "u" : "U"} escape at {st.line}:{st.row}"
                        return ""
                    }
                    let hc = int(data[st.pos])
                    let hv = hex_value(hc)
                    if (hv < 0) {
                        err = "non-hex character in \\{esc == 'u' ? "u" : "U"} escape at {st.line}:{st.row}"
                        return ""
                    }
                    cp = (cp << uint(4)) | uint(hv)
                    advance(data, st)
                }
                utf8_encode(buf, cp)
            } else {
                err = "unknown escape '\\{to_char(esc)}' at {st.line}:{st.row}"
                return ""
            }
            continue
        }
        push(buf, data[st.pos])
        advance(data, st)
    }
    err = "unterminated multi-line basic string at {st.line}:{st.row}"
    return ""
}

def private read_literal_string(data : array<uint8>&; var st : LexState&; var err : string&) : string {
    // st.pos is just past the opening '
    var buf : array<uint8>
    buf |> reserve(32)
    while (st.pos < st.n) {
        let c = int(data[st.pos])
        if (c == '\'') {
            advance(data, st)
            return string(buf)
        }
        if (c == '\n') {
            err = "newline in literal string at {st.line}:{st.row}"
            return ""
        }
        push(buf, data[st.pos])
        advance(data, st)
    }
    err = "unterminated literal string at {st.line}:{st.row}"
    return ""
}

def private read_ml_literal_string(data : array<uint8>&; var st : LexState&; var err : string&) : string {
    // st.pos is just past the opening '''
    if (st.pos < st.n && int(data[st.pos]) == '\r' && st.pos + 1 < st.n && int(data[st.pos + 1]) == '\n') {
        advance(data, st); advance(data, st)
    } elif (st.pos < st.n && int(data[st.pos]) == '\n') {
        advance(data, st)
    }
    var buf : array<uint8>
    buf |> reserve(64)
    while (st.pos < st.n) {
        if (int(data[st.pos]) == '\'' && st.pos + 2 < st.n
                && int(data[st.pos + 1]) == '\'' && int(data[st.pos + 2]) == '\'') {
            advance(data, st); advance(data, st); advance(data, st)
            if (st.pos < st.n && int(data[st.pos]) == '\'') {
                push(buf, uint8('\''))
                advance(data, st)
                if (st.pos < st.n && int(data[st.pos]) == '\'') {
                    push(buf, uint8('\''))
                    advance(data, st)
                }
            }
            return string(buf)
        }
        push(buf, data[st.pos])
        advance(data, st)
    }
    err = "unterminated multi-line literal string at {st.line}:{st.row}"
    return ""
}

def private looks_like_datetime(data : array<uint8>&; pos : int) : bool {
    // YYYY-MM-DD prefix: 4 digits + '-' + 2 digits + '-' + 2 digits.
    let n = data |> length
    if (pos + 10 > n) return false
    for (i in range(4)) {
        if (!is_number(int(data[pos + i]))) return false
    }
    return (int(data[pos + 4]) == '-' &&
            is_number(int(data[pos + 5])) && is_number(int(data[pos + 6])) &&
            int(data[pos + 7]) == '-' &&
            is_number(int(data[pos + 8])) && is_number(int(data[pos + 9])))
}

def private looks_like_time(data : array<uint8>&; pos : int) : bool {
    // HH:MM:SS prefix: 2 digits + ':' + 2 digits + ':' + 2 digits.
    let n = data |> length
    if (pos + 8 > n) return false
    return (is_number(int(data[pos])) && is_number(int(data[pos + 1])) &&
            int(data[pos + 2]) == ':' &&
            is_number(int(data[pos + 3])) && is_number(int(data[pos + 4])) &&
            int(data[pos + 5]) == ':' &&
            is_number(int(data[pos + 6])) && is_number(int(data[pos + 7])))
}

def private read_datetime(data : array<uint8>&; var st : LexState&; allow_time : bool) : string {
    // Consume a TOML date-time / local-date / local-time. Max length is
    // ~35 (date+T+time+frac+offset); reserve once to avoid PERF006 churn.
    var buf : array<uint8>
    buf |> reserve(40)
    let n = st.n
    // Date portion (YYYY-MM-DD) — already verified by caller for the date entry.
    if (!allow_time) {
        for (_ in range(10)) {
            push(buf, data[st.pos])
            advance(data, st)
        }
        // Optional time portion (T or space separator + HH:MM:SS[.frac][offset]).
        if (st.pos < n) {
            let sep = int(data[st.pos])
            let next_is_time = (sep == 'T' || sep == 't' || sep == ' ') && st.pos + 8 < n
            if (next_is_time && looks_like_time(data, st.pos + 1)) {
                push(buf, data[st.pos])
                advance(data, st)
                for (_ in range(8)) {
                    push(buf, data[st.pos])
                    advance(data, st)
                }
                // Optional fractional seconds.
                if (st.pos < n && int(data[st.pos]) == '.') {
                    push(buf, data[st.pos])
                    advance(data, st)
                    while (st.pos < n && is_number(int(data[st.pos]))) {
                        push(buf, data[st.pos])
                        advance(data, st)
                    }
                }
                // Optional offset: Z or +/-HH:MM.
                if (st.pos < n) {
                    let oc = int(data[st.pos])
                    if (oc == 'Z' || oc == 'z') {
                        push(buf, data[st.pos])
                        advance(data, st)
                    } elif (oc == '+' || oc == '-') {
                        // Must be exactly HH:MM after the sign for an offset.
                        if (st.pos + 5 < n
                                && is_number(int(data[st.pos + 1])) && is_number(int(data[st.pos + 2]))
                                && int(data[st.pos + 3]) == ':'
                                && is_number(int(data[st.pos + 4])) && is_number(int(data[st.pos + 5]))) {
                            for (_ in range(6)) {
                                push(buf, data[st.pos])
                                advance(data, st)
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Local-time only: HH:MM:SS[.frac]
        for (_ in range(8)) {
            push(buf, data[st.pos])
            advance(data, st)
        }
        if (st.pos < n && int(data[st.pos]) == '.') {
            push(buf, data[st.pos])
            advance(data, st)
            while (st.pos < n && is_number(int(data[st.pos]))) {
                push(buf, data[st.pos])
                advance(data, st)
            }
        }
    }
    return string(buf)
}

def private strip_underscores(s : string) : string {
    var buf : array<uint8>
    peek_data(s) $(d) {
        for (b in d) {
            if (int(b) != '_') {
                push(buf, b)
            }
        }
    }
    return string(buf)
}

def private parse_int_prefixed(rest : string; radix : int; var err : string&) : int64 {
    // rest already underscore-free; parse digits in `radix`.
    let cleaned = strip_underscores(rest)
    if (empty(cleaned)) {
        err = "empty {radix == 16 ? "hex" : (radix == 8 ? "octal" : "binary")} literal"
        return 0l
    }
    var acc = 0l
    peek_data(cleaned) $(d) {
        for (b in d) {
            let c = int(b)
            var v = -1
            if (radix == 16) {
                v = hex_value(c)
            } elif (radix == 8) {
                v = (c >= '0' && c <= '7') ? c - '0' : -1
            } elif (radix == 2) {
                v = (c == '0' || c == '1') ? c - '0' : -1
            }
            if (v < 0) {
                err = "invalid digit '{to_char(c)}' in literal"
                return
            }
            acc = acc * int64(radix) + int64(v)
        }
    }
    return acc
}

def private rewind_to_bare(data : array<uint8>&; var st : LexState&; start_pos, start_line, start_row : int; var out : TomlTokenAt&) : void {
    // Rewinds the lexer to `start_pos` and re-scans a bare key. If the leading
    // byte at start_pos is `+` (not a TOML bare-key char), the rewind would
    // produce an empty token (F13). Per TOML 1.0 ABNF, `+` cannot begin a key —
    // emit a stray-sign error so the parser sees the failure at the right
    // location.
    st.pos = start_pos
    st.line = start_line
    st.row = start_row
    let c0 = int(data[start_pos])
    if (c0 == '+') {
        advance(data, st)
        out = TomlTokenAt(value = TomlToken(_error = "stray sign '+' at {start_line}:{start_row}"), line = start_line, row = start_row)
        return
    }
    let bs = read_bare_key(data, st)
    out = TomlTokenAt(value = TomlToken(_bare = bs), line = start_line, row = start_row)
}

def private classify_number_or_datetime(data : array<uint8>&; var st : LexState&; var out : TomlTokenAt&) : bool {
    // Decides whether to lex a numeric / datetime token at st.pos. Returns true if consumed.
    //
    // Key vs value ambiguity: the scan may run, then discover the token is
    // actually a bare key with a numeric prefix (e.g. `2024-01-15`, `123abc`,
    // `infinity_count`). When the cursor would land on a bare-char
    // continuation after a numeric/datetime/inf/nan match, we rewind to
    // `start_pos` and re-emit as `_bare` so the parser sees the full
    // identifier. Unsigned `inf`/`nan` always emit `_bare` — the parser
    // re-interprets them as float values at value position, or accepts them
    // as keys at key position. Signed `+inf`/`-inf`/`+nan`/`-nan` keep
    // emitting `_float` since `+` is not a bare-key char (those are
    // unambiguously values).
    let start_line = st.line
    let start_row = st.row
    let n = st.n
    let start_pos = st.pos
    let c0 = int(data[st.pos])
    // Date or date-time?
    if (is_number(c0) && looks_like_datetime(data, start_pos)) {
        let s = read_datetime(data, st, false)
        // Bare-key fallback: '2024-01-15a' continues as a bare key.
        if (st.pos < n && is_bare_char(int(data[st.pos]))) {
            rewind_to_bare(data, st, start_pos, start_line, start_row, out)
            return true
        }
        // F16: `2024-01-15:...` (a stray `:` directly after a date) almost
        // certainly means the user mis-formatted a date-time. Surface a
        // dedicated diagnostic instead of letting the parser hit the `:`
        // as a generic "unexpected character".
        if (st.pos < n && int(data[st.pos]) == ':') {
            out = TomlTokenAt(value = TomlToken(_error = "':' after date '{s}' — expected 'T' or space before the time at {start_line}:{start_row}"), line = start_line, row = start_row)
            return true
        }
        out = TomlTokenAt(value = TomlToken(_datetime = s), line = start_line, row = start_row)
        return true
    }
    // Local time?
    if (is_number(c0) && looks_like_time(data, start_pos)) {
        let s = read_datetime(data, st, true)
        if (st.pos < n && is_bare_char(int(data[st.pos]))) {
            rewind_to_bare(data, st, start_pos, start_line, start_row, out)
            return true
        }
        // F16: trailing colon on a time value (`12:34:56:`) — clear diagnostic.
        if (st.pos < n && int(data[st.pos]) == ':') {
            out = TomlTokenAt(value = TomlToken(_error = "trailing ':' after time '{s}' at {start_line}:{start_row}"), line = start_line, row = start_row)
            return true
        }
        out = TomlTokenAt(value = TomlToken(_datetime = s), line = start_line, row = start_row)
        return true
    }
    // Numeric: optional sign, then digits / hex prefix / inf / nan.
    var buf : array<uint8>
    var sign_int : int64 = 1l
    if (c0 == '+' || c0 == '-') {
        if (c0 == '-') {
            sign_int = -1l
        }
        push(buf, data[st.pos])
        advance(data, st)
    }
    if (st.pos >= n) {
        out = TomlTokenAt(value = TomlToken(_error = "stray sign at {start_line}:{start_row}"), line = start_line, row = start_row)
        return true
    }
    let c1 = int(data[st.pos])
    // inf / nan
    if (c1 == 'i' && starts_with_at(data, st.pos, "inf")) {
        for (_ in range(3)) {
            advance(data, st)
        }
        // Bare-key fallback: 'infinity_count' continues past 'inf'.
        if (st.pos < n && is_bare_char(int(data[st.pos]))) {
            rewind_to_bare(data, st, start_pos, start_line, start_row, out)
            return true
        }
        // Unsigned `inf`: emit _bare so parser can choose key vs value semantics.
        if (empty(buf)) {
            out = TomlTokenAt(value = TomlToken(_bare = "inf"), line = start_line, row = start_row)
            return true
        }
        let val = sign_int > 0l ? DBL_POS_INF : DBL_NEG_INF
        out = TomlTokenAt(value = TomlToken(_float = val), line = start_line, row = start_row)
        return true
    }
    if (c1 == 'n' && starts_with_at(data, st.pos, "nan")) {
        for (_ in range(3)) {
            advance(data, st)
        }
        if (st.pos < n && is_bare_char(int(data[st.pos]))) {
            rewind_to_bare(data, st, start_pos, start_line, start_row, out)
            return true
        }
        if (empty(buf)) {
            out = TomlTokenAt(value = TomlToken(_bare = "nan"), line = start_line, row = start_row)
            return true
        }
        out = TomlTokenAt(value = TomlToken(_float = DBL_NAN), line = start_line, row = start_row)
        return true
    }
    // Hex / Oct / Bin (no sign permitted before these per TOML spec).
    if (empty(buf) && c1 == '0' && st.pos + 1 < n) {
        let prefix = int(data[st.pos + 1])
        if (prefix == 'x' || prefix == 'o' || prefix == 'b') {
            advance(data, st); advance(data, st)
            var tail : array<uint8>
            // Per TOML 1.0: `_` is only allowed between two valid digits.
            // Leading (`0x_ff`), trailing (`0xff_`), and doubled (`0xff__aa`)
            // underscores are rejected. If the run continues into bare-char
            // territory (e.g. `0xff_oops`), the bare-key fallback below
            // catches it as a single bare identifier instead.
            while (st.pos < n) {
                let c = int(data[st.pos])
                if (is_hex(c)) {
                    push(tail, data[st.pos])
                    advance(data, st)
                } elif (c == '_') {
                    let prev_hex = !empty(tail) && is_hex(int(tail[length(tail) - 1]))
                    let nx = peek(data, st, 1)
                    if (!prev_hex || !is_hex(nx)) {
                        if (is_bare_char(nx)) {
                            // Continuation into bare-key territory — rewind.
                            rewind_to_bare(data, st, start_pos, start_line, start_row, out)
                            return true
                        }
                        out = TomlTokenAt(value = TomlToken(_error = "underscore in {prefix == 'x' ? "hex" : (prefix == 'o' ? "octal" : "binary")} literal must be between digits at {start_line}:{start_row}"), line = start_line, row = start_row)
                        return true
                    }
                    push(tail, data[st.pos])
                    advance(data, st)
                } else {
                    break
                }
            }
            // Bare-key fallback: '0xff_extra' (non-hex alpha continuation).
            if (st.pos < n && is_bare_char(int(data[st.pos]))) {
                rewind_to_bare(data, st, start_pos, start_line, start_row, out)
                return true
            }
            var ierr : string
            let radix = prefix == 'x' ? 16 : (prefix == 'o' ? 8 : 2)
            let val = parse_int_prefixed(string(tail), radix, ierr)
            if (!empty(ierr)) {
                out = TomlTokenAt(value = TomlToken(_error = "{ierr} at {start_line}:{start_row}"), line = start_line, row = start_row)
                return true
            }
            out = TomlTokenAt(value = TomlToken(_integer = val), line = start_line, row = start_row)
            return true
        }
    }
    // Decimal integer or float. Per TOML 1.0: `_` must sit between two digits.
    // If the run continues into bare-char territory, rewind to a bare-key
    // scan rather than emit a number-syntax error.
    var saw_digit = false
    while (st.pos < n) {
        let c = int(data[st.pos])
        if (is_number(c)) {
            saw_digit = true
            push(buf, data[st.pos])
            advance(data, st)
        } elif (c == '_') {
            let prev_digit = !empty(buf) && is_number(int(buf[length(buf) - 1]))
            let nx = peek(data, st, 1)
            if (!prev_digit || !is_number(nx)) {
                if (is_bare_char(nx)) {
                    rewind_to_bare(data, st, start_pos, start_line, start_row, out)
                    return true
                }
                out = TomlTokenAt(value = TomlToken(_error = "underscore in number must be between digits at {start_line}:{start_row}"), line = start_line, row = start_row)
                return true
            }
            push(buf, data[st.pos])
            advance(data, st)
        } else {
            break
        }
    }
    var is_float = false
    if (st.pos < n && int(data[st.pos]) == '.') {
        // Distinguish '.' as decimal point from '.' as dotted-key separator: a digit must follow.
        if (st.pos + 1 < n && is_number(int(data[st.pos + 1]))) {
            is_float = true
            push(buf, data[st.pos])
            advance(data, st)
            while (st.pos < n) {
                let c = int(data[st.pos])
                if (is_number(c)) {
                    saw_digit = true
                    push(buf, data[st.pos])
                    advance(data, st)
                } elif (c == '_') {
                    let prev_digit = !empty(buf) && is_number(int(buf[length(buf) - 1]))
                    let nx = peek(data, st, 1)
                    if (!prev_digit || !is_number(nx)) {
                        out = TomlTokenAt(value = TomlToken(_error = "underscore in fractional part must be between digits at {start_line}:{start_row}"), line = start_line, row = start_row)
                        return true
                    }
                    push(buf, data[st.pos])
                    advance(data, st)
                } else {
                    break
                }
            }
        }
    }
    if (st.pos < n && (int(data[st.pos]) == 'e' || int(data[st.pos]) == 'E')) {
        is_float = true
        push(buf, data[st.pos])
        advance(data, st)
        if (st.pos < n && (int(data[st.pos]) == '+' || int(data[st.pos]) == '-')) {
            push(buf, data[st.pos])
            advance(data, st)
        }
        // Exponent must have at least one digit; track separately from mantissa
        // so `1e` / `1e+` (where the mantissa already saw a digit) fail-fast.
        var exp_saw_digit = false
        while (st.pos < n) {
            let c = int(data[st.pos])
            if (is_number(c)) {
                exp_saw_digit = true
                saw_digit = true
                push(buf, data[st.pos])
                advance(data, st)
            } elif (c == '_') {
                let prev_digit = !empty(buf) && is_number(int(buf[length(buf) - 1]))
                let nx = peek(data, st, 1)
                if (!prev_digit || !is_number(nx)) {
                    out = TomlTokenAt(value = TomlToken(_error = "underscore in exponent must be between digits at {start_line}:{start_row}"), line = start_line, row = start_row)
                    return true
                }
                push(buf, data[st.pos])
                advance(data, st)
            } else {
                break
            }
        }
        if (!exp_saw_digit) {
            out = TomlTokenAt(value = TomlToken(_error = "exponent has no digits at {start_line}:{start_row}"), line = start_line, row = start_row)
            return true
        }
    }
    if (!saw_digit) {
        out = TomlTokenAt(value = TomlToken(_error = "invalid number at {start_line}:{start_row}"), line = start_line, row = start_row)
        return true
    }
    // Bare-key fallback: '123abc' — digits followed by alpha/dash/underscore.
    // Only applies to unsigned tokens; signed '+'/'-' aren't valid bare-key prefixes
    // (well, '-' is, but a '-'-prefix bare key wouldn't enter classify_* since the
    // next_token gate routes those to the bare-key path).
    if (st.pos < n && is_bare_char(int(data[st.pos]))) {
        rewind_to_bare(data, st, start_pos, start_line, start_row, out)
        return true
    }
    let text = strip_underscores(string(buf))
    if (is_float) {
        out = TomlTokenAt(value = TomlToken(_float = double(text)), line = start_line, row = start_row)
    } else {
        out = TomlTokenAt(value = TomlToken(_integer = int64(text)), line = start_line, row = start_row)
    }
    return true
}

def private next_token(data : array<uint8>&; var st : LexState&; var out : TomlTokenAt&) : bool {
    // Skip inline whitespace + comments. Newline IS a token.
    while (st.pos < st.n) {
        let c = int(data[st.pos])
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(data, st)
        } elif (c == '#') {
            skip_comment_to_eol(data, st)
        } else {
            break
        }
    }
    if (st.pos >= st.n) return false
    let start_line = st.line
    let start_row = st.row
    let c = int(data[st.pos])
    if (c == '\n') {
        advance(data, st)
        out = TomlTokenAt(value = TomlToken(_newline = null), line = start_line, row = start_row)
        return true
    }
    if (c == '=' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == '.') {
        advance(data, st)
        out = TomlTokenAt(value = TomlToken(_symbol = c), line = start_line, row = start_row)
        return true
    }
    if (c == '"') {
        // Basic string or multi-line basic string.
        if (st.pos + 2 < st.n && int(data[st.pos + 1]) == '"' && int(data[st.pos + 2]) == '"') {
            advance(data, st); advance(data, st); advance(data, st)
            var serr : string
            let s = read_ml_basic_string(data, st, serr)
            if (!empty(serr)) {
                out = TomlTokenAt(value = TomlToken(_error = serr), line = start_line, row = start_row)
                return true
            }
            out = TomlTokenAt(value = TomlToken(_string = s), line = start_line, row = start_row)
            return true
        }
        advance(data, st)
        var serr : string
        let s = read_basic_string(data, st, serr)
        if (!empty(serr)) {
            out = TomlTokenAt(value = TomlToken(_error = serr), line = start_line, row = start_row)
            return true
        }
        out = TomlTokenAt(value = TomlToken(_string = s), line = start_line, row = start_row)
        return true
    }
    if (c == '\'') {
        if (st.pos + 2 < st.n && int(data[st.pos + 1]) == '\'' && int(data[st.pos + 2]) == '\'') {
            advance(data, st); advance(data, st); advance(data, st)
            var serr : string
            let s = read_ml_literal_string(data, st, serr)
            if (!empty(serr)) {
                out = TomlTokenAt(value = TomlToken(_error = serr), line = start_line, row = start_row)
                return true
            }
            out = TomlTokenAt(value = TomlToken(_string = s), line = start_line, row = start_row)
            return true
        }
        advance(data, st)
        var serr : string
        let s = read_literal_string(data, st, serr)
        if (!empty(serr)) {
            out = TomlTokenAt(value = TomlToken(_error = serr), line = start_line, row = start_row)
            return true
        }
        out = TomlTokenAt(value = TomlToken(_string = s), line = start_line, row = start_row)
        return true
    }
    // Date / time / number — leading digit or sign or 'i' (inf) or 'n' (nan).
    if (is_number(c) || c == '+' || c == '-'
            || (c == 'i' && starts_with_at(data, st.pos, "inf"))
            || (c == 'n' && starts_with_at(data, st.pos, "nan"))) {
        // For sign-prefixed forms, lookahead: '+'/'-' must be followed by digit/inf/nan to
        // start a numeric value. Otherwise it's a bare key starting with '-' (valid per
        // TOML ABNF) or a stray '+' (illegal — '+' isn't a bare-key char).
        if (c == '+' || c == '-') {
            let nx = peek(data, st, 1)
            let is_value_continuation = (is_number(nx) ||
                (nx == 'i' && starts_with_at(data, st.pos + 1, "inf")) ||
                (nx == 'n' && starts_with_at(data, st.pos + 1, "nan")))
            if (!is_value_continuation) {
                if (c == '-' && is_bare_char(nx)) {
                    let bs = read_bare_key(data, st)
                    out = TomlTokenAt(value = TomlToken(_bare = bs), line = start_line, row = start_row)
                    return true
                }
                advance(data, st)
                out = TomlTokenAt(value = TomlToken(_error = "stray sign '{to_char(c)}' at {start_line}:{start_row}"), line = start_line, row = start_row)
                return true
            }
        }
        return classify_number_or_datetime(data, st, out)
    }
    // Bare identifier (could be key, true/false, inf/nan in value context).
    if (is_bare_char(c)) {
        let s = read_bare_key(data, st)
        out = TomlTokenAt(value = TomlToken(_bare = s), line = start_line, row = start_row)
        return true
    }
    advance(data, st)
    out = TomlTokenAt(value = TomlToken(_error = "unexpected character '{to_char(c)}' at {start_line}:{start_row}"), line = start_line, row = start_row)
    return true
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

struct private TomlTokenStream {
    //! Buffered token cursor — one-token lookahead.
    data    : array<uint8>
    state   : LexState
    has_ahead : bool
    ahead   : TomlTokenAt
}

def private new_stream(text : string) : TomlTokenStream {
    var ts : TomlTokenStream
    peek_data(text) $(d) {
        for (b in d) {
            push(ts.data, b)
        }
    }
    ts.state = LexState(pos = 0, n = length(ts.data), line = 1, row = 0)
    ts.has_ahead = false
    return <- ts
}

def private peek_tok(var ts : TomlTokenStream&) : bool {
    if (ts.has_ahead) return true
    var tk : TomlTokenAt
    if (!next_token(ts.data, ts.state, tk)) return false
    ts.ahead = tk
    ts.has_ahead = true
    return true
}

def private take_tok(var ts : TomlTokenStream&; var out : TomlTokenAt&) : bool {
    if (!peek_tok(ts)) return false
    out = ts.ahead
    ts.has_ahead = false
    return true
}

def private skip_newlines(var ts : TomlTokenStream&) : void {
    while (peek_tok(ts) && (ts.ahead.value is _newline)) {
        ts.has_ahead = false
    }
}

def private get_object(var v : JsonValue?) : table<string; JsonValue?>& {
    // Caller guarantees v != null && v.value is _object — returns a mutable ref to the table.
    unsafe {
        return v.value as _object
    }
}

def private get_array(var v : JsonValue?) : array<JsonValue?>& {
    unsafe {
        return v.value as _array
    }
}

def private ensure_table(var parent : JsonValue?; key : string; var err : string&; at_line, at_row : int) : JsonValue? {
    // Find or create a child table named `key` in `parent`'s _object.
    // Returns null on conflict (existing non-table value at that key).
    var tab & = get_object(parent)
    if (key_exists(tab, key)) {
        var child : JsonValue? = tab[key]
        if (child == null || !(child.value is _object)) {
            err = "key '{key}' redefined as table at {at_line}:{at_row}"
            return null
        }
        return child
    }
    var fresh : table<string; JsonValue?>
    var child : JsonValue? = JV(fresh)
    tab |> insert(key, child)
    return child
}

def private ensure_array_table(var parent : JsonValue?; key : string; var err : string&; at_line, at_row : int) : JsonValue? {
    var tab & = get_object(parent)
    var arr_holder : JsonValue?
    if (key_exists(tab, key)) {
        var existing : JsonValue? = tab[key]
        if (existing == null || !(existing.value is _array)) {
            err = "key '{key}' redefined as array-of-tables at {at_line}:{at_row}"
            return null
        }
        arr_holder = existing
    } else {
        var fresh : array<JsonValue?>
        arr_holder = JV(fresh)
        tab |> insert(key, arr_holder)
    }
    var arr & = get_array(arr_holder)
    var fresh_tab : table<string; JsonValue?>
    var entry : JsonValue? = JV(fresh_tab)
    arr |> push(entry)
    return entry
}

def private parse_key_path(var ts : TomlTokenStream&; var out : array<string>&; var err : string&; var at_line, at_row : int&) : bool {
    // Parses a dotted key path (key / "quoted" / 'lit') . (key / ...).
    out |> clear
    var tk : TomlTokenAt
    if (!take_tok(ts, tk)) {
        err = "unexpected end of input expecting key"
        return false
    }
    at_line = tk.line
    at_row  = tk.row
    if (!extract_key_segment(tk, out, err)) return false
    while (peek_tok(ts) && (ts.ahead.value is _symbol) && (ts.ahead.value as _symbol == '.')) {
        ts.has_ahead = false
        if (!take_tok(ts, tk)) {
            err = "unexpected end of input after dot in key"
            return false
        }
        if (!extract_key_segment(tk, out, err)) return false
    }
    return true
}

def private extract_key_segment(tk : TomlTokenAt; var out : array<string>&; var err : string&) : bool {
    // Accept any lexical form whose source text is a valid TOML bare key:
    //  - `_bare`     ordinary alphanumeric / dash / underscore identifier
    //  - `_string`   quoted (basic or literal) key, already unescaped
    //  - `_integer`  digit-only key (e.g. `123 = 1`)
    //  - `_datetime` date-shaped key (e.g. `2024-01-15 = 1`) — the lexer
    //    recognized the date shape; per TOML ABNF the same chars are a
    //    valid bare key, so we use the raw lexical form.
    if (tk.value is _bare) {
        out |> push(tk.value as _bare)
        return true
    }
    if (tk.value is _string) {
        out |> push(tk.value as _string)
        return true
    }
    if (tk.value is _integer) {
        out |> push("{tk.value as _integer}")
        return true
    }
    if (tk.value is _datetime) {
        out |> push(tk.value as _datetime)
        return true
    }
    err = "expected key at {tk.line}:{tk.row}"
    return false
}

def private parse_value(var ts : TomlTokenStream&; var err : string&) : JsonValue? {
    var tk : TomlTokenAt
    if (!take_tok(ts, tk)) {
        err = "unexpected end of input expecting value"
        return null
    }
    if (tk.value is _string) return JV(tk.value as _string)
    if (tk.value is _integer) return JV(tk.value as _integer)
    if (tk.value is _float) return JV(tk.value as _float)
    if (tk.value is _datetime) return JV(tk.value as _datetime)
    if (tk.value is _bare) {
        let name = tk.value as _bare
        if (name == "true") return JV(true)
        if (name == "false") return JV(false)
        if (name == "inf") return JV(DBL_POS_INF)
        if (name == "nan") return JV(DBL_NAN)
        err = "unexpected identifier '{name}' as value at {tk.line}:{tk.row}"
        return null
    }
    if (tk.value is _symbol) {
        let sym = tk.value as _symbol
        if (sym == '[') return parse_inline_array(ts, err)
        if (sym == '{') return parse_inline_table(ts, err)
        err = "unexpected symbol '{to_char(sym)}' as value at {tk.line}:{tk.row}"
        return null
    }
    if (tk.value is _error) {
        err = tk.value as _error
        return null
    }
    err = "unexpected token at {tk.line}:{tk.row}"
    return null
}

def private parse_inline_array(var ts : TomlTokenStream&; var err : string&) : JsonValue? {
    var arr : array<JsonValue?>
    skip_newlines(ts)
    if (peek_tok(ts) && (ts.ahead.value is _symbol) && (ts.ahead.value as _symbol == ']')) {
        ts.has_ahead = false
        return JV(arr)
    }
    while (true) {
        skip_newlines(ts)
        var v : JsonValue? = parse_value(ts, err)
        if (v == null) return null
        arr |> push(v)
        skip_newlines(ts)
        if (!peek_tok(ts)) {
            err = "unterminated inline array"
            return null
        }
        if (ts.ahead.value is _symbol) {
            let sym = ts.ahead.value as _symbol
            if (sym == ',') {
                ts.has_ahead = false
                skip_newlines(ts)
                if (peek_tok(ts) && (ts.ahead.value is _symbol) && (ts.ahead.value as _symbol == ']')) {
                    ts.has_ahead = false
                    return JV(arr)
                }
                continue
            } elif (sym == ']') {
                ts.has_ahead = false
                return JV(arr)
            }
        }
        err = "expected ',' or ']' in inline array at {ts.ahead.line}:{ts.ahead.row}"
        return null
    }
    return JV(arr)
}

def private parse_inline_table(var ts : TomlTokenStream&; var err : string&) : JsonValue? {
    // Inline-table-internal closure tracking (F6): once a key inside the literal
    // is bound to a nested `{...}` value, later dotted keys in the SAME literal
    // cannot walk through it. Pointer-keyed for the same reason as the
    // top-level set in TomlState.
    var tab : table<string; JsonValue?>
    var root : JsonValue? = JV(tab)
    var inline_closed_local : table<uint64>
    if (peek_tok(ts) && (ts.ahead.value is _symbol) && (ts.ahead.value as _symbol == '}')) {
        ts.has_ahead = false
        return root
    }
    while (true) {
        var path : array<string>
        var pline, prow : int
        if (!parse_key_path(ts, path, err, pline, prow)) return null
        var tk : TomlTokenAt
        if (!take_tok(ts, tk)) {
            err = "unexpected end of input expecting '=' after key in inline table"
            return null
        }
        if (!(tk.value is _symbol) || (tk.value as _symbol) != '=') {
            err = "expected '=' after key in inline table at {tk.line}:{tk.row}"
            return null
        }
        var v : JsonValue? = parse_value(ts, err)
        if (v == null) return null
        // Walk path inside root, with inline-closure checks at each step.
        var holder : JsonValue? = root
        let n = length(path)
        for (i in range(n - 1)) {
            var ht & = get_object(holder)
            if (key_exists(ht, path[i])) {
                var existing : JsonValue? = ht[path[i]]
                if (existing != null && (existing.value is _object) && key_exists(inline_closed_local, jvkey(existing))) {
                    err = "cannot extend inline table at '{path[i]}' (inline table is closed) at {pline}:{prow}"
                    return null
                }
            }
            holder = ensure_table(holder, path[i], err, pline, prow)
            if (holder == null) return null
        }
        let leaf = path[n - 1]
        var leaf_tab & = get_object(holder)
        if (key_exists(leaf_tab, leaf)) {
            err = "duplicate key '{leaf}' in inline table at {pline}:{prow}"
            return null
        }
        leaf_tab |> insert(leaf, v)
        if (v != null && (v.value is _object)) {
            mark_inline_subtree(inline_closed_local, v)
        }
        if (!peek_tok(ts)) {
            err = "unterminated inline table"
            return null
        }
        if (ts.ahead.value is _symbol) {
            let sym = ts.ahead.value as _symbol
            if (sym == ',') {
                ts.has_ahead = false
                continue
            } elif (sym == '}') {
                ts.has_ahead = false
                return root
            }
        }
        err = "expected ',' or '}' in inline table at {ts.ahead.line}:{ts.ahead.row}"
        return null
    }
    return root
}

// ---------------------------------------------------------------------------
// Top-level driver
// ---------------------------------------------------------------------------

struct private TomlState {
    root : JsonValue?
    current_path : array<string>  //! header path of the current section ([a.b.c] → ["a","b","c"])
    //! Tables defined by a [header] line. A second [header] for the same path is invalid.
    explicitly_defined : table<string>
    //! Tables created implicitly by a dotted-key statement (e.g. `b.c = 1` under
    //! [a] implicitly creates `a.b`). Per TOML 1.0 these cannot be re-opened by
    //! a later [header] line. Keyed by joined path (path_key).
    dotted_keyed : table<string>
    //! Tables whose value was assigned by an inline-table literal `{...}`. Per
    //! TOML 1.0 these are immutable — neither dotted keys nor [header] may
    //! extend them. Keyed by the JsonValue pointer (so AoT entries are tracked
    //! distinctly: items[0].b and items[1].b are different pointers).
    inline_closed : table<uint64>
}

def private jvkey(p : JsonValue?) : uint64 {
    // Identity key for an _object node — used to mark inline-table-literal
    // subtrees as immutable (TomlState.inline_closed). Distinct heap nodes get
    // distinct keys, so the same logical path under different AoT entries does
    // NOT collide.
    return intptr(unsafe(reinterpret<void?>(p)))
}

def private mark_inline_subtree(var closed : table<uint64>&; v : JsonValue?) : void {
    //! Recursively marks every `_object` JsonValue in `v`'s subtree as
    //! inline-closed. Caller-supplied target table lets us reuse the walker
    //! for both top-level state (apply_keyval) and inline-table-internal
    //! tracking (parse_inline_table).
    if (v == null || !(v.value is _object)) return
    let k = jvkey(v)
    if (!key_exists(closed, k)) {
        closed |> insert(k)
    }
    let obj & = unsafe(v.value as _object)
    for (child in values(obj)) {
        mark_inline_subtree(closed, child)
    }
}

def private path_key(path : array<string>) : string {
    var buf : array<uint8>
    for (i in range(length(path))) {
        if (i > 0) {
            push(buf, 1u8)
        }
        peek_data(path[i]) $(d) {
            for (b in d) {
                push(buf, b)
            }
        }
    }
    return string(buf)
}

def private resolve_header(var state : TomlState&; path : array<string>; is_array_of_tables : bool; var err : string&; at_line, at_row : int) : JsonValue? {
    // Walks `path` inside state.root, creating intermediate tables.
    // For array-of-tables: appends a new entry at the leaf.
    // For standard tables: creates (or accepts an existing empty) table at the leaf.
    var cur = state.root
    let n = length(path)
    for (i in range(n - 1)) {
        let key = path[i]
        var tab & = get_object(cur)
        if (key_exists(tab, key)) {
            var existing : JsonValue? = tab[key]
            if (existing == null) {
                err = "null at '{key}' on path"
                return null
            }
            // F5: header walk cannot descend into an inline-bound table.
            if (existing.value is _object && key_exists(state.inline_closed, jvkey(existing))) {
                err = "cannot open header [{path |> join(".")}] — '{key}' is an inline table (closed) at {at_line}:{at_row}"
                return null
            }
            if (existing.value is _array) {
                // Array of tables — descend into last entry (TOML semantics: super-path of
                // [[a]] is the LAST [[a]] element).
                var arr & = get_array(existing)                    // nolint:LINT003 — index must yield mutable for `cur = ...`
                if (empty(arr)) {
                    err = "empty array-of-tables traversal at '{key}'"
                    return null
                }
                cur = arr[length(arr) - 1]
            } elif (existing.value is _object) {
                cur = existing
            } else {
                err = "key '{key}' redefined as table at {at_line}:{at_row}"
                return null
            }
        } else {
            var fresh : table<string; JsonValue?>
            var child : JsonValue? = JV(fresh)
            tab |> insert(key, child)
            cur = child
        }
    }
    let leaf = path[n - 1]
    if (is_array_of_tables) {
        return ensure_array_table(cur, leaf, err, at_line, at_row)
    }
    // Standard table header.
    var tab & = get_object(cur)
    if (key_exists(tab, leaf)) {
        var existing : JsonValue? = tab[leaf]
        if (existing == null || !(existing.value is _object)) {
            err = "key '{leaf}' redefined as table at {at_line}:{at_row}"
            return null
        }
        // F5: header cannot open an inline-bound table.
        if (key_exists(state.inline_closed, jvkey(existing))) {
            err = "cannot open header [{path |> join(".")}] — '{leaf}' is an inline table (closed) at {at_line}:{at_row}"
            return null
        }
        let pk = path_key(path)
        if (key_exists(state.explicitly_defined, pk)) {
            err = "duplicate table header [{path |> join(".")}] at {at_line}:{at_row}"
            return null
        }
        // F4: header cannot re-open a table that was implicitly created by a
        // dotted-key statement. (Per TOML 1.0: "using dotted keys to redefine
        // tables already defined ... is not allowed", same in reverse.)
        if (key_exists(state.dotted_keyed, pk)) {
            err = "cannot open header [{path |> join(".")}] — table already implicitly defined by dotted key at {at_line}:{at_row}"
            return null
        }
        state.explicitly_defined |> insert(pk)
        return existing
    }
    var fresh : table<string; JsonValue?>
    var child : JsonValue? = JV(fresh)
    tab |> insert(leaf, child)
    state.explicitly_defined |> insert(path_key(path))
    return child
}

def private apply_keyval(var state : TomlState&; var current : JsonValue?; path : array<string>; var v : JsonValue?; var err : string&; at_line, at_row : int) : bool {
    // Walks `path` inside `current` (the active section's table). Each
    // intermediate step:
    //   - F5: rejects if the existing sub-table was bound by an inline literal.
    //   - F4: marks the prefix path (current section + path[0..i]) as
    //     implicitly created by a dotted key, so a later [header] line cannot
    //     re-open it.
    // The leaf write rejects duplicate keys and, when `v` is an inline-table
    // _object literal, marks the assigned subtree as inline-closed (F5/F6).
    var holder = current
    let n = length(path)
    for (i in range(n - 1)) {
        var tab & = get_object(holder)
        if (key_exists(tab, path[i])) {
            var existing : JsonValue? = tab[path[i]]
            if (existing != null && (existing.value is _object) && key_exists(state.inline_closed, jvkey(existing))) {
                err = "cannot extend inline table at '{path[i]}' (inline table is closed) at {at_line}:{at_row}"
                return false
            }
        }
        holder = ensure_table(holder, path[i], err, at_line, at_row)
        if (holder == null) return false
        // F4: tag this absolute prefix as implicitly created by dotted key.
        var prefix : array<string>
        prefix |> reserve(length(state.current_path) + i + 1)
        prefix |> push_from(state.current_path)
        for (j in range(i + 1)) {
            prefix |> push(path[j])
        }
        let pk = path_key(prefix)
        if (!key_exists(state.dotted_keyed, pk)) {
            state.dotted_keyed |> insert(pk)
        }
    }
    let leaf = path[n - 1]
    var tab & = get_object(holder)
    if (key_exists(tab, leaf)) {
        err = "duplicate key '{leaf}' at {at_line}:{at_row}"
        return false
    }
    tab |> insert(leaf, v)
    // If v is an inline-table literal, lock its entire subtree.
    if (v != null && (v.value is _object)) {
        mark_inline_subtree(state.inline_closed, v)
    }
    return true
}

def public read_toml(text : string implicit; var error : string&) : JsonValue? {
    //! Reads TOML 1.0 from ``text``. On parse failure returns ``null`` and
    //! populates ``error``; on success returns the root table as a
    //! ``JsonValue?`` (``_object`` variant).
    error = ""
    var state : TomlState
    var root_tab : table<string; JsonValue?>
    state.root = JV(root_tab)
    var ts <- new_stream(text)
    var current = state.root
    while (peek_tok(ts)) {
        // Newlines are record separators.
        if (ts.ahead.value is _newline) {
            ts.has_ahead = false
            continue
        }
        // Lexer error surfaces here as a token in-stream.
        if (ts.ahead.value is _error) {
            error = ts.ahead.value as _error
            return null
        }
        // Table header: '[' (single → std-table) or '[[' (double → array-of-tables).
        if ((ts.ahead.value is _symbol) && (ts.ahead.value as _symbol) == '[') {
            let head_line = ts.ahead.line
            let head_row  = ts.ahead.row
            ts.has_ahead = false
            var is_aot = false
            if (peek_tok(ts) && (ts.ahead.value is _symbol) && (ts.ahead.value as _symbol == '[')) {
                is_aot = true
                ts.has_ahead = false
            }
            var path : array<string>
            var pline, prow : int
            if (!parse_key_path(ts, path, error, pline, prow)) return null
            var tk : TomlTokenAt
            if (!take_tok(ts, tk)) {
                error = "unexpected end of input expecting ']' to close table header opened at {head_line}:{head_row}"
                return null
            }
            if (!(tk.value is _symbol) || (tk.value as _symbol) != ']') {
                error = "expected ']' to close table header at {tk.line}:{tk.row}"
                return null
            }
            if (is_aot) {
                if (!take_tok(ts, tk)) {
                    error = "unexpected end of input expecting ']]' to close array-of-tables header opened at {head_line}:{head_row}"
                    return null
                }
                if (!(tk.value is _symbol) || (tk.value as _symbol) != ']') {
                    error = "expected ']]' to close array-of-tables header at {tk.line}:{tk.row}"
                    return null
                }
            }
            current = resolve_header(state, path, is_aot, error, head_line, head_row)
            if (current == null) return null
            // Update logical section path for absolute-path tracking of dotted
            // keys (F4). The pointer `current` already encodes the AoT-entry
            // selection (last entry) for inline-closed checks (F5).
            state.current_path |> clear
            state.current_path |> push_from(path)
            // The rest of the header line must be whitespace / comment / newline.
            if (peek_tok(ts)) {
                if (!(ts.ahead.value is _newline)) {
                    if (ts.ahead.value is _error) {
                        error = ts.ahead.value as _error
                        return null
                    }
                    error = "unexpected token after table header at {ts.ahead.line}:{ts.ahead.row}"
                    return null
                }
                ts.has_ahead = false
            }
            continue
        }
        // key = value
        var path : array<string>
        var pline, prow : int
        if (!parse_key_path(ts, path, error, pline, prow)) return null
        var tk : TomlTokenAt
        if (!take_tok(ts, tk)) {
            error = "unexpected end of input expecting '=' after key at {pline}:{prow}"
            return null
        }
        if (!(tk.value is _symbol) || (tk.value as _symbol) != '=') {
            error = "expected '=' after key at {tk.line}:{tk.row}"
            return null
        }
        var v : JsonValue? = parse_value(ts, error)
        if (v == null || !apply_keyval(state, current, path, v, error, pline, prow)) return null
        // The rest of the line must be whitespace / comment / newline / EOF.
        if (peek_tok(ts)) {
            if (!(ts.ahead.value is _newline)) {
                if (ts.ahead.value is _error) {
                    error = ts.ahead.value as _error
                    return null
                }
                error = "unexpected token after value at {ts.ahead.line}:{ts.ahead.row}"
                return null
            }
            ts.has_ahead = false
        }
    }
    return state.root
}
