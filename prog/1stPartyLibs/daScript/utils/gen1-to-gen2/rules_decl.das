options gen2
options indenting = 4

require daslib/ast
require strings
require daslib/strings_boost
require offsets
require comments
require scan
require patch


def private body_last_line(src : Source; lex : Lexical; headerLine, headerIndent : int) : int {
    var last = -1
    var ln = headerLine + 1
    while (ln <= line_count(src)) {
        if (!line_has_code(src, lex, ln)) {
            ln++
            continue
        }
        if (indent_of(src, line_begin(src, ln)) <= headerIndent) break
        last = ln
        ln++
    }
    return last
}


def private braced_body_follows(src : Source; headerLine : int) : bool {
    var ln = headerLine + 1
    while (ln <= line_count(src)) {
        let b = line_begin(src, ln)
        let e = line_end(src, ln)
        if (is_blank(src, b, e)) {
            ln++
            continue
        }
        let first = b + indent_of(src, b)
        return text_of(src, first, first + 1) == "\{"
    }
    return false
}


def public rule_decl_braces(src : Source; lex : Lexical; headerOffset : int; var set : EditSet;
                            bodyless : bool = false) {
    //! Wraps an indent-delimited declaration body in braces.
    //!
    //! Declines when the header line already carries a `{`, so a file that is half converted
    //! (or was always gen2) is left alone. `bodyless` says the declaration needs a body in gen2
    //! even when v1 wrote none - a struct or an enum, but not a typedef.
    let headerLine = line_at(src, headerOffset)
    let headerEnd = line_end(src, headerLine)
    if (text_of(src, line_begin(src, headerLine), headerEnd) |> find("\{") >= 0) return
    let headerIndent = indent_of(src, line_begin(src, headerLine))
    let last = body_last_line(src, lex, headerLine, headerIndent)
    if (last < 0) {
        if (bodyless && !braced_body_follows(src, headerLine)) {
            set |> insert(line_edge(src, lex, headerOffset), " \{\}", "decl-empty",
                RANK_BLOCK_OPEN, headerOffset)
        }
        return
    }
    let brace = line_edge(src, lex, headerOffset)
    set |> insert(brace, " \{", "decl-open", RANK_BLOCK_OPEN, headerOffset)
    set |> insert(line_end(src, last), "{src.eol}{repeat(" ", headerIndent)}\}", "decl-close",
        RANK_CLOSE_OUTER, headerOffset)
}


def private is_global_header(text : string) : bool {
    var first = true
    var ok = false
    for (w in text |> replace("\t", " ") |> split(" ")) {
        if (w |> empty) continue
        if (first) {
            if (w != "let" && w != "var") return false
            first = false
            ok = true
        } elif (w != "shared" && w != "public" && w != "private") {
            return false
        }
    }
    return ok
}


def public global_block_header(src : Source; lex : Lexical; varOffset : int) : int {
    //! Offset of the bare `let`/`var` line heading the block that `varOffset` belongs to, or -1.
    //!
    //! A global's `at` is its NAME, so `let x = 1` finds the keyword on its own line: only a
    //! header that declares nothing of its own opens an indented block.
    let varLine = line_at(src, varOffset)
    let varIndent = indent_of(src, line_begin(src, varLine))
    var ln = varLine - 1
    while (ln >= 1) {
        let b = line_begin(src, ln)
        let e = line_end(src, ln)
        if (is_blank(src, b, e)) {
            ln--
            continue
        }
        if (indent_of(src, b) >= varIndent) {
            ln--
            continue
        }
        return is_global_header(text_of(src, b, line_edge(src, lex, b)) |> strip) ? b : -1
    }
    return -1
}


def public typedef_block_header(src : Source; lex : Lexical; aliasOffset : int) : int {
    //! Offset of the bare `typedef` line heading the block that `aliasOffset` belongs to, or -1.
    //!
    //! The tree records only the aliased TYPE, so the header is found by walking back over the
    //! block's own lines — the first shallower line above them is the header or nothing is.
    let aliasLine = line_at(src, aliasOffset)
    let aliasIndent = indent_of(src, line_begin(src, aliasLine))
    var ln = aliasLine - 1
    while (ln >= 1) {
        let b = line_begin(src, ln)
        let e = line_end(src, ln)
        if (is_blank(src, b, e)) {
            ln--
            continue
        }
        if (indent_of(src, b) >= aliasIndent) {
            ln--
            continue
        }
        return text_of(src, b, line_edge(src, lex, b)) |> strip == "typedef" ? b : -1
    }
    return -1
}


def public rule_typedef_block(src : Source; lex : Lexical; headerOffset : int; var set : EditSet) {
    //! `typedef` + indented `A = B` lines becomes one `typedef A = B` per line.
    //!
    //! The bare `typedef` line goes away, which is a deletion — so a comment parked on it makes
    //! the engine refuse the edit and the block is left as it is.
    let headerLine = line_at(src, headerOffset)
    let headerStart = line_begin(src, headerLine)
    let headerText = text_of(src, headerStart, line_edge(src, lex, headerStart)) |> strip
    if (headerText != "typedef") return
    let headerIndent = indent_of(src, headerStart)
    let last = body_last_line(src, lex, headerLine, headerIndent)
    if (last < 0) return
    set |> remove(headerStart, line_begin(src, headerLine + 1), "typedef-header")
    for (ln in range(headerLine + 1, last + 1)) {
        let b = line_begin(src, ln)
        let e = line_end(src, ln)
        if (is_blank(src, b, e)) continue
        let firstChar = b + indent_of(src, b)
        set |> replace(b, firstChar, "{repeat(" ", headerIndent)}typedef ", "typedef-line")
    }
}


def private find_back(src : Source; lex : Lexical; from : int; token : string) : int {
    let n = length(token)
    var found = -1
    peek_data(src.text) $(bytes) {
        peek_data(token) $(needle) {
            var i = from - n
            while (i >= 0) {
                var hit = true
                for (k in range(n)) {
                    if (bytes[i + k] != needle[k]) {
                        hit = false
                        break
                    }
                }
                if (hit && !has_comment_or_string(lex, i, i + n)) {
                    found = i
                    break
                }
                i--
            }
        }
    }
    return found
}


def public rule_argument_annotation(src : Source; lex : Lexical; at : LineInfo; var set : EditSet) {
    //! `[[optional]] var i : int?` -> `@optional var i : int?`.
    var i = span_begin(src, at)
    if (i <= 0) return
    i = skip_back_blank(src, i)
    if (i >= 3 && text_of(src, i - 3, i) == "var") {
        i = skip_back_blank(src, i - 3)
    }
    if (i < 2 || text_of(src, i - 2, i) != "]]") return
    let close = i - 2
    let open = find_back(src, lex, close, "[[")
    if (open < 0) return
    let body = text_of(src, open + 2, close)
    if (empty(body |> strip) || has_comment_or_string(lex, open, close + 2)) return
    let glued = text_of(src, close + 2, close + 3)
    let pad = (glued == "" || glued == " " || glued == "\t" || glued == "\r" || glued == "\n") ? "" : " "
    set |> replace(open, close + 2, "@{body}{pad}", "argument-annotation")
}
