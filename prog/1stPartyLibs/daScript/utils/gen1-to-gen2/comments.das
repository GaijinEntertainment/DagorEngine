options gen2
options indenting = 4

require math
require strings
require offsets


struct public Span {
    //! A byte range [from, to) in the source.
    from : int
    to : int
}


struct public Lexical {
    //! Comment and string-literal ranges of one source, in ascending order.
    comments : array<Span>
    strings : array<Span>
}


def private is_at(bytes : array<uint8> const#; i : int; a, b : int) : bool {
    return i + 1 < length(bytes) && bytes[i] == uint8(a) && bytes[i + 1] == uint8(b)
}

def private end_of_quoted(bytes : array<uint8> const#; start : int; quote : int) : int {
    var i = start + 1
    while (i < length(bytes)) {
        if (bytes[i] == uint8('\\')) {
            i += 2
            continue
        }
        if (bytes[i] == uint8(quote)) return i + 1
        i++
    }
    return length(bytes)
}


def public scan_lexical(src : Source) : Lexical {
    //! Finds every comment and string/character literal in `src`.
    //!
    //! Character literals are tracked because `'"'` would otherwise open a phantom string, and
    //! string literals are tracked because `"/* not a comment */"` would otherwise open one.
    var lex : Lexical
    peek_data(src.text) $(bytes) {
        var i = 0
        while (i < length(bytes)) {
            if (is_at(bytes, i, '/', '/')) {
                var e = i + 2
                while (e < length(bytes) && bytes[e] != uint8('\n')) {
                    e++
                }
                if (e > i + 2 && bytes[e - 1] == uint8('\r')) {
                    e--
                }
                lex.comments |> push(Span(from = i, to = e))
                i = e
            } elif (is_at(bytes, i, '/', '*')) {
                var e = i + 2
                while (e < length(bytes) && !is_at(bytes, e, '*', '/')) {
                    e++
                }
                e = min(e + 2, length(bytes))
                lex.comments |> push(Span(from = i, to = e))
                i = e
            } elif (bytes[i] == uint8('"') || bytes[i] == uint8('\'')) {
                let e = end_of_quoted(bytes, i, int(bytes[i]))
                lex.strings |> push(Span(from = i, to = e))
                i = e
            } else {
                i++
            }
        }
    }
    return <- lex
}


def private overlaps(spans : array<Span>; from, to : int) : bool {
    for (s in spans) {
        if (s.from >= to) break
        if (s.to > from) return true
    }
    return false
}


def public has_comment_or_string(lex : Lexical; from, to : int) : bool {
    //! True when [from, to) touches any comment or string literal. A deletion must not.
    return overlaps(lex.comments, from, to) || overlaps(lex.strings, from, to)
}


def public inside_comment(lex : Lexical; offset : int) : int {
    //! End offset of the comment containing `offset`, or `offset` when it is outside one.
    //!
    //! An insertion anchored at a line edge can otherwise land inside a block comment that
    //! spans lines, splitting it.
    for (c in lex.comments) {
        if (c.from > offset) break
        if (offset > c.from && offset < c.to) return c.to
    }
    return offset
}


def public comment_texts(src : Source; lex : Lexical) : array<string> {
    //! The comment texts in source order — the sequence the conservation gate compares.
    return <- [for (c in lex.comments); text_of(src, c.from, c.to)]
}


def public line_edge(src : Source; lex : Lexical; offset : int) : int {
    //! Where a statement-trailing insertion goes: end of `offset`'s line, before the newline and
    //! any trailing whitespace.
    //!
    //! A trailing `//` comment pulls the edge in front of it — text appended after one would be
    //! swallowed by it. A block comment does not: `def f()/**/ \{` is where the brace belongs,
    //! and a block comment that runs past the line pushes the edge to where it closes.
    let ln = line_at(src, offset)
    var e = line_end(src, ln)
    let pushed = inside_comment(lex, e)
    if (pushed != e) {
        return pushed
    }
    let lineStart = line_begin(src, ln)
    for (c in lex.comments) {
        if (c.from >= e) break
        if (c.from >= lineStart && c.to >= e && text_of(src, c.from, c.from + 2) == "//") {
            e = c.from
            break
        }
    }
    let b = line_begin(src, ln)
    peek_data(src.text) $(bytes) {
        while (e > b) {
            let c = int(bytes[e - 1])
            if (c != ' ' && c != '\t' && c != '\r') break
            e--
        }
    }
    return e
}


def public blank_or_comments(src : Source; lex : Lexical; from, to : int) : bool {
    //! True when [from, to) holds nothing but whitespace and whole comments.
    //!
    //! This is what may sit between a block's last statement and the end of its line, and the
    //! closing brace goes after it — a `}` in front of a trailing `//` would be swallowed by it.
    //! A comment that starts inside the range and ends past it fails, so the brace never lands
    //! in the middle of a block comment.
    var ok = true
    peek_data(src.text) $(bytes) {
        var i = max(from, 0)
        let stop = min(to, length(bytes))
        while (i < stop) {
            var jumped = false
            for (c in lex.comments) {
                if (c.from > i) break
                if (c.from == i) {
                    if (c.to > stop) {
                        ok = false
                    } else {
                        i = c.to
                        jumped = true
                    }
                    break
                }
            }
            if (!ok) break
            if (jumped) continue
            let ch = int(bytes[i])
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                ok = false
                break
            }
            i++
        }
    }
    return ok
}


def public is_blank(src : Source; from, to : int) : bool {
    //! True when [from, to) holds nothing but whitespace — the gap-collapse guard. A gap
    //! holding a comment fails this, which is what keeps the comment where the author put it.
    var blank = true
    peek_data(src.text) $(bytes) {
        for (i in range(max(from, 0), min(to, length(bytes)))) {
            let c = int(bytes[i])
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                blank = false
                break
            }
        }
    }
    return blank
}
