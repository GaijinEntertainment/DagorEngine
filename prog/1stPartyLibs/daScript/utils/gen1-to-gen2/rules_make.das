options gen2
options indenting = 4

require daslib/ast
require math
require strings
require daslib/strings_boost
require offsets
require comments
require extent
require patch
require scan


struct private TypeParts {
    text : string
    lead : string
    trail : string
    ok : bool
}


def private split_type(src : Source; lex : Lexical; msd : ExprMakeStruct?; limit : int) : TypeParts {
    var parts = TypeParts(ok = false)
    let open = span_begin(src, msd.at)
    let typeStart = span_begin(src, msd.makeType.at)
    if (typeStart < open + 2 || typeStart >= limit) return parts
    let typeEnd = end_of_type_token(src, typeStart, limit)
    if (typeEnd <= typeStart) return parts
    parts.text = text_of(src, typeStart, typeEnd)
    parts.lead = text_of(src, open + 2, typeStart)
    var rest = text_of(src, typeEnd, limit)
    if (msd.makeStructFlags.useInitializer) {
        let lp = find_token(src, lex, typeEnd, "(")
        let rp = lp >= 0 ? find_token(src, lex, lp + 1, ")") : -1
        if (lp >= 0 && rp > lp && rp < limit) {
            parts.lead += text_of(src, typeEnd, lp) + text_of(src, lp + 1, rp)
            rest = text_of(src, rp + 1, limit)
        }
    }
    parts.trail = rest
    parts.ok = true
    return parts
}


def private instance_range(src : Source; lex : Lexical; st : MakeStruct?; var from, to : int&) : bool {
    var seen = false
    for (fld in *st) {
        if (!seen) {
            from = span_begin(src, fld.at)
            seen = true
        }
        let ext = fld.value != null ? extent_of(src, lex, fld.value) : Extent(ok = false)
        to = ext.ok ? ext.to : (fld.value != null ? span_end(src, fld.value.at) : span_end(src, fld.at))
    }
    return seen
}


def private call_name(typeText : string) : string {
    return typeText |> ends_with("#") ? "struct<{typeText}>" : typeText
}


def public rule_make_struct(src : Source; lex : Lexical; msd : ExprMakeStruct?; wrapped : bool;
                            var set : EditSet) {
    //! `[[Foo ...]]` in every shape the v1 grammar allows.
    //!
    //! `wrapped` means the node sits inside `[{ ... }]`, whose own rule owns the brackets: this
    //! one then emits only the instance list, so `[{Foo a=1; a=2}]` becomes `[Foo(a=1), Foo(a=2)]`
    //! rather than a nested `fixed_array`.
    let open = span_begin(src, msd.at)
    let opener = text_of(src, open, open + 2)
    if (opener != "[[" && !(wrapped && opener == "[\{")) return
    let closer = find_closer(src, lex, open + 2, wrapped ? "\}]" : "]]")
    if (closer < 0) {
        set.refused |> push("make-struct at {open}: no matching closer")
        return
    }
    let nInstances = length(msd.structs)
    let editFrom = wrapped ? open + 2 : open

    if (nInstances == 0) {
        let parts = split_type(src, lex, msd, closer)
        if (!parts.ok) {
            set.refused |> push("make-struct at {open}: type spelling not found")
            return
        }
        if (!wrapped && msd.makeType != null && parts.text |> ends_with("?")) {
            set |> replace(open, closer + 2, "default<{parts.text}>{parts.lead}{parts.trail}", "make-struct-ptr")
            return
        }
        set |> replace(editFrom, wrapped ? closer : closer + 2,
            "{parts.lead}{call_name(parts.text)}({parts.trail})", "make-struct-empty")
        return
    }

    var firstFrom = 0
    var firstTo = 0
    if (!instance_range(src, lex, msd.structs[0], firstFrom, firstTo)) {
        set.refused |> push("make-struct at {open}: first instance has no fields")
        return
    }
    let parts = split_type(src, lex, msd, firstFrom)
    if (!parts.ok) {
        set.refused |> push("make-struct at {open}: type spelling not found")
        return
    }
    let body = "{parts.lead}{call_name(parts.text)}({parts.trail}"
    let head = (nInstances > 1 && !wrapped) ? "fixed_array({body}" : body
    set |> replace(editFrom, firstFrom, head, "make-struct-open")

    var prevTo = firstTo
    for (i in range(1, nInstances)) {
        var from = 0
        var to = 0
        if (!instance_range(src, lex, msd.structs[i], from, to)) {
            set.refused |> push("make-struct at {open}: instance {i} has no fields")
            return
        }
        let semi = find_token(src, lex, prevTo, ";")
        if (semi < 0 || semi >= from) {
            set.refused |> push("make-struct at {open}: no ';' between instances")
            return
        }
        set |> replace(semi, semi + 1, "), {call_name(parts.text)}(", "make-struct-next")
        prevTo = to
    }
    if (wrapped) {
        set |> insert(closer, ")", "make-struct-close")
        return
    }
    set |> replace(closer, closer + 2, nInstances > 1 ? "))" : ")", "make-struct-close", RANK_CLOSE_OUTER)
}


def private is_make_tuple(e : ExpressionPtr) : bool {
    return e.__rtti == "ExprMakeTuple"
}


def public rule_make_array(src : Source; lex : Lexical; mka : ExprMakeArray?; wrapped : bool;
                           var set : EditSet) {
    //! `[[auto 1; 2]]` -> `fixed_array(1, 2)`, and the one-value tuple form `[[auto 1, 2]]`.
    //!
    //! The gen2 spelling of a make-array is a call, so the `;` separators become `,`. A single
    //! value that is itself a tuple is the `[[auto a, b]]` form, which is just parentheses.
    if (mka.gen2) return
    let open = span_begin(src, mka.at)
    let opener = text_of(src, open, open + 2)
    if (opener != "[[" && !wrapped) return
    let closerText = opener == "[\{" ? "\}]" : (opener == "\{\{" ? "\}\}" : "]]")
    let closer = find_closer(src, lex, open + 2, closerText)
    if (closer < 0) {
        set.refused |> push("make-array at {open}: no matching closer")
        return
    }
    let editFrom = wrapped ? open + 2 : open
    let n = length(mka.values)
    let firstAt = n > 0 ? span_begin(src, mka.values[0].at) : closer
    let leadType = lead_type_text(src, lex, open + 2, firstAt)
    let typed = opener != "\{\{" && !empty(leadType) && !(leadType |> starts_with("auto"))
    let head = typed ? typed_head(src, lex, open, firstAt) : TypedHead()
    if (typed && !head.ok) {
        set.refused |> push("make-array at {open}: element type spelling not found")
        return
    }
    if (n == 0) {
        if (wrapped) return
        set |> replace(open, closer + 2, "fixed_array({text_of(src, open + 2, closer)})", "make-array-empty")
        return
    }
    let firstFrom = span_begin(src, mka.values[0].at)
    let single_tuple = n == 1 && is_make_tuple(mka.values[0])
    let lead = lead_text(src, lex, open + 2, firstFrom)
    if (wrapped) {
        set |> replace(editFrom, firstFrom, typed ? head.gap : lead, "make-array-open")
    } elif (typed) {
        set |> replace(open, firstFrom, "fixed_array<{head.text}>({head.gap}", "make-array-open")
    } else {
        set |> replace(open, firstFrom, single_tuple ? "{lead}(" : "fixed_array({lead}", "make-array-open")
    }
    let ends <- separators(src, lex, mka, closer, set)
    if (!(single_tuple && !wrapped) && opener != "\{\{") {
        tuple_parens(src, mka, ends, set)
    }
    drop_trailing_separator(src, closer, set)
    if (!wrapped) {
        set |> replace(closer, closer + 2, ")", "make-array-close", RANK_CLOSE_OUTER)
    }
}


def private lead_text(src : Source; lex : Lexical; from, to : int) : string {
    let raw = text_of(src, from, to)
    let autoAt = find_token(src, lex, from, "auto")
    if (autoAt < 0 || autoAt >= to) return raw
    let autoEnd = end_of_type_token(src, autoAt, to)
    return text_of(src, from, autoAt) + text_of(src, autoEnd, to)
}


struct private TypedHead {
    text : string
    gap : string
    ok : bool
}


def private typed_head(src : Source; lex : Lexical; open, firstAt : int) : TypedHead {
    let typeStart = skip_blank_and_comments(src, lex, open + 2, firstAt)
    let typeEnd = end_of_type_token(src, typeStart, firstAt)
    if (typeEnd <= typeStart) return TypedHead(ok = false)
    return TypedHead(text = text_of(src, typeStart, typeEnd),
        gap = text_of(src, open + 2, typeStart) + text_of(src, typeEnd, firstAt), ok = true)
}


def private skip_blank_and_comments(src : Source; lex : Lexical; from, to : int) : int {
    var i = max(from, 0)
    while (i < to) {
        var jumped = false
        for (c in lex.comments) {
            if (c.from > i) break
            if (c.from == i && c.to <= to) {
                i = c.to
                jumped = true
                break
            }
        }
        if (jumped) continue
        let ch = text_of(src, i, i + 1)
        if (ch != " " && ch != "\t" && ch != "\r" && ch != "\n") return i
        i++
    }
    return to
}


def private lead_type_text(src : Source; lex : Lexical; from, to : int) : string {
    return build_string() $(writer) {
        var i = max(from, 0)
        while (i < to) {
            var jumped = false
            for (c in lex.comments) {
                if (c.from > i) break
                if (c.from == i && c.to <= to) {
                    i = c.to
                    jumped = true
                    break
                }
            }
            if (jumped) continue
            let ch = text_of(src, i, i + 1)
            if (ch != " " && ch != "\t" && ch != "\r" && ch != "\n") {
                writer |> write(ch)
            }
            i++
        }
    }
}


def private separators(src : Source; lex : Lexical; mka : ExprMakeArray?; closer : int;
                       var set : EditSet) : array<int> {
    let n = length(mka.values)
    var ends : array<int>
    ends |> reserve(n)
    for (i in range(1, n)) {
        let prev = extent_of(src, lex, mka.values[i - 1])
        let semi = find_token(src, lex, prev.ok ? prev.to : span_end(src, mka.values[i - 1].at), ";")
        let valFrom = span_begin(src, mka.values[i].at)
        if (semi >= 0 && semi < valFrom) {
            set |> replace(semi, semi + 1, ",", "make-array-sep")
        }
        ends |> push(semi >= 0 && semi < valFrom ? semi : valFrom)
    }
    ends |> push(closer)
    return <- ends
}


def private drop_trailing_separator(src : Source; closer : int; var set : EditSet) {
    var tail = closer
    peek_data(src.text) $(bytes) {
        while (tail > 0) {
            let c = int(bytes[tail - 1])
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break
            tail--
        }
    }
    if (tail > 0 && text_of(src, tail - 1, tail) == ";") {
        set |> remove(tail - 1, tail, "make-array-trailing-sep")
    }
}


def private tuple_parens(src : Source; mka : ExprMakeArray?; ends : array<int>; var set : EditSet) {
    for (i in range(0, length(mka.values))) {
        if (!is_make_tuple(mka.values[i])) continue
        let valFrom = span_begin(src, mka.values[i].at)
        if (text_of(src, valFrom, valFrom + 1) == "(") continue
        set |> insert(valFrom, "(", "make-array-tuple-open")
        set |> insert(ends[i], ")", "make-array-tuple-close")
    }
}


def public rule_move_wrapper(src : Source; lex : Lexical; call : ExprCall?; var set : EditSet) : int {
    //! `[{ ... }]` and `\{\{ ... \}\}` — the v1 array-of and table-of forms.
    //!
    //! The parser has already turned them into `to_array_move` / `to_table_move` around the
    //! make-node, so the wrapper rewrites the brackets (`[{` -> `[`, `\}]` -> `]`) and the
    //! make-node inside emits only its elements. Returns the opener offset it claimed, or -1.
    if (call.name != "to_array_move" && call.name != "to_table_move") return -1
    let open = span_begin(src, call.at)
    let opener = text_of(src, open, open + 2)
    if (opener != "[\{" && opener != "\{\{") return -1
    let isTable = opener == "\{\{"
    let closer = find_closer(src, lex, open + 2, isTable ? "\}\}" : "\}]")
    if (closer < 0) {
        set.refused |> push("make-wrapper at {open}: no matching closer")
        return -1
    }
    set |> replace(open, open + 2, isTable ? "\{" : "[", "make-wrapper-open")
    set |> replace(closer, closer + 2, isTable ? "\}" : "]", "make-wrapper-close", RANK_CLOSE_OUTER)
    return open
}


def public rule_comprehension(src : Source; lex : Lexical; cmp : ExprArrayComprehension?; var set : EditSet) {
    //! `[{for x in r; e; where c}]` -> `[for x in r; e; where c]`; the two generator spellings
    //! `[[for ...]]` and `\{\{for ...\}\}` -> `[iterator for ...]`.
    //!
    //! Only the brackets change; everything between them is the author's text, copied verbatim.
    let forAt = span_begin(src, cmp.at)
    var open = -1
    for (back in range(1, 8)) {
        let cand = forAt - back - 1
        if (cand < 0) break
        let two = text_of(src, cand, cand + 2)
        if (two == "[\{" || two == "[[" || two == "\{\{") {
            open = cand
            break
        }
    }
    if (open < 0) return
    let opener = text_of(src, open, open + 2)
    let closerText = opener == "[[" ? "]]" : (opener == "[\{" ? "\}]" : "\}\}")
    let closer = find_closer(src, lex, open + 2, closerText)
    if (closer < 0) {
        set.refused |> push("comprehension at {open}: no matching closer")
        return
    }
    let isGenerator = opener != "[\{"
    set |> replace(open, open + 2, isGenerator ? "[iterator " : "[", "comprehension-open")
    set |> replace(closer, closer + 2, "]", "comprehension-close", RANK_CLOSE_OUTER)
}
