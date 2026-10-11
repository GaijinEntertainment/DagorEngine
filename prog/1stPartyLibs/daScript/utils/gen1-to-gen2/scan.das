options gen2
options indenting = 4

require math
require strings
require offsets
require comments


def private skip_lexical(lex : Lexical; pos : int) : int {
    for (c in lex.comments) {
        if (c.from <= pos && pos < c.to) return c.to
    }
    for (s in lex.strings) {
        if (s.from <= pos && pos < s.to) return s.to
    }
    return pos
}


def public group_close(src : Source; lex : Lexical; from : int; opener, closer : string) : int {
    //! Offset of the one-byte `closer` matching the `opener` at `from`, or -1. A delimiter inside
    //! a comment or a string literal does not count.
    var depth = 0
    var found = -1
    var op = 0
    var cl = 0
    peek_data(opener) $(ob) {
        op = !empty(ob) ? int(ob[0]) : 0
    }
    peek_data(closer) $(cb) {
        cl = !empty(cb) ? int(cb[0]) : 0
    }
    peek_data(src.text) $(bytes) {
        var i = from
        let n = length(bytes)
        while (i < n) {
            let skipped = skip_lexical(lex, i)
            if (skipped != i) {
                i = skipped
                continue
            }
            let ch = int(bytes[i])
            if (ch == op) {
                depth++
            } elif (ch == cl) {
                depth--
                if (depth == 0) {
                    found = i
                    break
                }
            }
            i++
        }
    }
    return found
}


def public brace_close(src : Source; lex : Lexical; from : int) : int {
    //! Offset of the `\}` that matches the `\{` at `from`, or -1.
    return group_close(src, lex, from, "\{", "\}")
}


def public paren_close(src : Source; lex : Lexical; from : int) : int {
    //! Offset of the `)` that matches the `(` at `from`, or -1.
    return group_close(src, lex, from, "(", ")")
}


def public find_token(src : Source; lex : Lexical; from : int; token : string) : int {
    //! Offset of the next `token` at or after `from`, ignoring comments and string literals.
    //! Returns -1 when it is not there.
    let n = length(token)
    if (n == 0) return -1
    var i = max(from, 0)
    var found = -1
    peek_data(src.text) $(bytes) {
        peek_data(token) $(needle) {
            while (i + n <= length(bytes)) {
                let skipped = skip_lexical(lex, i)
                if (skipped != i) {
                    i = skipped
                    continue
                }
                var hit = true
                for (k in range(n)) {
                    if (bytes[i + k] != needle[k]) {
                        hit = false
                        break
                    }
                }
                if (hit) {
                    found = i
                    break
                }
                i++
            }
        }
    }
    return found
}


def public find_closer(src : Source; lex : Lexical; from : int; closer : string) : int {
    //! Offset of the construct's own closer, skipping over nested make-constructs.
    //!
    //! `[[Bar a=[[Foo b=1]]]]` — the outer closer is not the first `]]` after the last field,
    //! it is the one that matches at depth zero. A single `[` or `(` inside the body counts
    //! too: in `[[Foo a=arr[0]]]` the index's `]` would otherwise pair with the closer's first
    //! byte and cut the construct one character short.
    var depth = 0
    var sub = 0
    var i = max(from, 0)
    var found = -1
    var want0 = 0
    var want1 = 0
    peek_data(closer) $(cb) {
        if (length(cb) >= 2) {
            want0 = int(cb[0])
            want1 = int(cb[1])
        }
    }
    peek_data(src.text) $(bytes) {
        while (i + 1 < length(bytes)) {
            let skipped = skip_lexical(lex, i)
            if (skipped != i) {
                i = skipped
                continue
            }
            let a = int(bytes[i])
            let b = int(bytes[i + 1])
            let opens = (a == '[' && (b == '[' || b == '{')) || (a == '{' && b == '{')
            if (opens) {
                depth++
                i += 2
                continue
            }
            if (a == '[' || a == '(') {
                sub++
                i++
                continue
            }
            if (sub > 0 && (a == ']' || a == ')')) {
                sub--
                i++
                continue
            }
            let closes = (a == ']' && b == ']') || (a == '}' && (b == ']' || b == '}'))
            if (closes) {
                if (depth == 0) {
                    found = (a == want0 && b == want1) ? i : -1
                    break
                }
                depth--
                i += 2
                continue
            }
            i++
        }
    }
    return found
}


def public line_has_code(src : Source; lex : Lexical; line : int) : bool {
    //! False when the line carries nothing but whitespace and comment text.
    //!
    //! v1 delimits a body by indentation, and a comment does not end one - not even a banner
    //! written at column 0 between two methods of a class.
    let b = line_begin(src, line)
    let e = line_end(src, line)
    var i = skip_blank(src, b, e)
    while (i < e) {
        var jumped = false
        for (c in lex.comments) {
            if (c.from > i) break
            if (c.to > i) {
                i = c.to
                jumped = true
                break
            }
        }
        if (!jumped) return true
        i = skip_blank(src, i, e)
    }
    return false
}


def public skip_blank(src : Source; from, to : int) : int {
    //! First non-whitespace offset in [from, to), or `to`.
    var i = max(from, 0)
    peek_data(src.text) $(bytes) {
        let e = min(to, length(bytes))
        while (i < e) {
            let c = int(bytes[i])
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break
            i++
        }
    }
    return i
}


def public skip_back_blank(src : Source; from : int) : int {
    //! Start of the whitespace run that ends at `from`.
    var i = from
    peek_data(src.text) $(bytes) {
        while (i > 0) {
            let c = int(bytes[i - 1])
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break
            i--
        }
    }
    return i
}


def public end_of_type_token(src : Source; from, limit : int) : int {
    //! End of the type spelling that starts at `from`.
    //!
    //! `TypeDecl.at` covers only the base name, so `[[Foo?]]` reports `Foo` and the `?` has to
    //! be picked up from the text. Angle brackets are counted, so `tuple<int;float>` stays one
    //! token.
    var i = max(from, 0)
    var angle = 0
    peek_data(src.text) $(bytes) {
        let e = min(limit, length(bytes))
        while (i < e) {
            let c = int(bytes[i])
            if (c == '<') {
                angle++
            } elif (c == '>') {
                if (angle == 0) break
                angle--
            } elif (angle == 0) {
                let word = (is_alpha(c) || is_number(c)
                    || c == '_' || c == '?' || c == '#' || c == ':' || c == '[' || c == ']' || c == '&')
                if (!word) break
            }
            i++
        }
    }
    return i
}
