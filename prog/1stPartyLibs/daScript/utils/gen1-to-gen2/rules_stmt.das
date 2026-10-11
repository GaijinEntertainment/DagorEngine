options gen2
options indenting = 4

require daslib/ast
require math
require strings
require daslib/strings_boost
require offsets
require comments
require patch
require scan
require extent


def public is_braced(src : Source; at : LineInfo) : bool {
    let b = span_begin(src, at)
    return text_of(src, b, b + 1) == "\{" || text_of(src, skip_blank(src, b, b + 4), skip_blank(src, b, b + 4) + 1) == "\{"
}


def private block_end(src : Source; lex : Lexical; reported, start : int) : int {
    var e = reported
    var ln = line_at(src, e)
    while (ln > 1 && is_blank(src, line_begin(src, ln), e)) {
        ln--
        e = line_end(src, ln)
        if (e <= start) break
    }
    let eol = line_end(src, line_at(src, e))
    return (eol > e && blank_or_comments(src, lex, e, eol)) ? eol : e
}


def public block_close_at(src : Source; lex : Lexical; at : LineInfo) : int {
    //! Where a block's closing brace goes, and therefore where the gap to an `else` / `recover`
    //! that follows it starts. A block the author already braced keeps its own end: its `\}` is
    //! in the source, and a comment after that one is placed where they put it.
    let reported = span_end(src, at)
    if (is_braced(src, at)) return reported
    return block_end(src, lex, reported, span_begin(src, at))
}


struct private LiteralPipe {
    at : int = -1
    hasParams : bool
    ok : bool
}


def private group_open_before(src : Source; lex : Lexical; from : int;
                              closer, opener : string) : int {
    var depth = 0
    var found = -1
    var cl = 0
    var op = 0
    peek_data(closer) $(cb) {
        cl = !empty(cb) ? int(cb[0]) : 0
    }
    peek_data(opener) $(ob) {
        op = !empty(ob) ? int(ob[0]) : 0
    }
    peek_data(src.text) $(bytes) {
        var i = min(from, length(bytes) - 1)
        while (i >= 0) {
            if (!has_comment_or_string(lex, i, i + 1)) {
                let ch = int(bytes[i])
                if (ch == cl) {
                    depth++
                } elif (ch == op) {
                    depth--
                    if (depth == 0) {
                        found = i
                        break
                    }
                }
            }
            i--
        }
    }
    return found
}


def private paren_open_before(src : Source; lex : Lexical; from : int) : int {
    return group_open_before(src, lex, from, ")", "(")
}


def private literal_pipe(src : Source; lex : Lexical; start : int) : LiteralPipe {
    var i = skip_back_blank(src, start)
    if (i > 0 && text_of(src, i - 1, i) != ")") {
        var idStart = i
        peek_data(src.text) $(bytes) {
            while (idStart > 0) {
                let c = int(bytes[idStart - 1])
                if (!is_alpha(c) && !is_number(c) && c != '_' && c != '?' && c != '#') break
                idStart--
            }
        }
        let colon = skip_back_blank(src, idStart)
        if (idStart < i && colon > 0 && text_of(src, colon - 1, colon) == ":") {
            i = skip_back_blank(src, colon - 1)
        }
    }
    var params = false
    while (i > 0) {
        let ch = text_of(src, i - 1, i)
        if (ch == ")") {
            let lp = paren_open_before(src, lex, i - 1)
            if (lp < 0) return LiteralPipe(ok = false)
            params = true
            i = skip_back_blank(src, lp)
        } elif (ch == "]") {
            let lb = group_open_before(src, lex, i - 1, "]", "[")
            if (lb < 0) return LiteralPipe(ok = false)
            i = skip_back_blank(src, lb)
        } else {
            let k = skip_back_ident(src, i)
            if (k == i) break
            i = skip_back_blank(src, k)
        }
    }
    if (i < 2 || text_of(src, i - 2, i) != "<|") return LiteralPipe(ok = false)
    let j = skip_back_blank(src, i - 2)
    if (j < 1) return LiteralPipe(ok = false)
    let owner = text_of(src, j - 1, j)
    if (owner != "$" && owner != "@") return LiteralPipe(ok = false)
    return LiteralPipe(at = i - 2, hasParams = params, ok = true)
}


def private skip_back_ident(src : Source; from : int) : int {
    var i = from
    peek_data(src.text) $(bytes) {
        while (i > 0) {
            let c = int(bytes[i - 1])
            if (!is_alpha(c) && !is_number(c) && c != 95) break
            i--
        }
    }
    return i
}


def public rule_block_braces(src : Source; lex : Lexical; blk : ExprBlock?; var set : EditSet) {
    //! Wraps an indent-delimited block in braces.
    //!
    //! Blocks that already carry braces (gen2 code, lambda bodies) are left alone, and so is a
    //! one-line block whose span never leaves its header line.
    if (blk.genFlags.generated) return
    let start = span_begin(src, blk.at)
    if (is_braced(src, blk.at)) {
        let braced_pipe = literal_pipe(src, lex, start)
        if (braced_pipe.ok) {
            set |> remove(skip_back_blank(src, braced_pipe.at), braced_pipe.at + 2, "literal-pipe")
        }
        return
    }
    let stop = block_close_at(src, lex, blk.at)
    if (stop <= start) return
    let headerLine = line_at(src, start)
    if (line_at(src, stop) == headerLine) return
    let brace = line_edge(src, lex, start)
    if (brace < start && !is_blank(src, brace, start)) return
    let indent = indent_of(src, line_begin(src, headerLine))
    let piped = literal_pipe(src, lex, start)
    if (piped.ok) {
        set |> remove(skip_back_blank(src, piped.at), piped.at + 2, "literal-pipe")
        if (!piped.hasParams) {
            set |> insert(brace, "()", "literal-pipe-params", RANK_CLOSE_INNER, start)
        }
        set |> insert(brace, " \{", "block-open", RANK_BLOCK_OPEN, start)
    } else {
        let bare_pipe = preceded_by(src, brace, "<|")
        set |> insert(brace, bare_pipe ? " $() \{" : " \{", "block-open", RANK_BLOCK_OPEN, start)
    }
    set |> insert(stop, "{src.eol}{repeat(" ", indent)}\}", "block-close", RANK_CLOSE_OUTER, start)
}


def public preceded_by(src : Source; from : int; word : string) : bool {
    var i = from
    peek_data(src.text) $(bytes) {
        while (i > 0) {
            let c = int(bytes[i - 1])
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                i--
                continue
            }
            if (c != '/' || i < 2 || int(bytes[i - 2]) != '*') break
            var j = i - 2
            while (j >= 2 && !(int(bytes[j - 2]) == '/' && int(bytes[j - 1]) == '*')) {
                j--
            }
            if (j < 2) break
            i = j - 2
        }
    }
    let n = length(word)
    return i >= n && text_of(src, i - n, i) == word
}


def private wrap_expr(src : Source; lex : Lexical; at : LineInfo; expr : ExpressionPtr;
                      keyword : string; rule : string; var set : EditSet) {
    if (!keyword_at(src, at, keyword)) return
    let ext = extent_of(src, lex, expr)
    if (!ext.ok) return
    let head = expr_head_before(src, lex, ext.from)
    wrap_in_parens(src, lex, open_paren_before(src, lex, head, ext.to), ext.to, rule, set)
}


def private expr_head_before(src : Source; lex : Lexical; from : int) : int {
    var i = from
    while (i > 0) {
        let k = skip_back_ident(src, i)
        if (k != i) {
            i = k
            continue
        }
        let ch = text_of(src, i - 1, i)
        if (ch == "." || ch == "[") {
            i--
        } elif (ch == "]") {
            let lb = group_open_before(src, lex, i - 1, "]", "[")
            if (lb < 0) return i
            i = lb
        } else {
            break
        }
    }
    return i
}


def private keyword_at(src : Source; at : LineInfo; keyword : string) : bool {
    let from = span_begin(src, at)
    return text_of(src, from, from + length(keyword)) == keyword
}


def private open_paren_before(src : Source; lex : Lexical; from, to : int) : int {
    var start = from
    while (start > 0) {
        var prev = start
        peek_data(src.text) $(bytes) {
            while (prev > 0) {
                let c = int(bytes[prev - 1])
                if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break
                prev--
            }
        }
        if (prev <= 0 || text_of(src, prev - 1, prev) != "(") break
        let close = paren_close(src, lex, prev - 1)
        if (close < 0 || close >= to) break
        start = prev - 1
    }
    return start
}


def private wrap_return(src : Source; lex : Lexical; from, to : int; var set : EditSet) {
    wrap_in_parens(src, lex, from, to, "return-tuple", set, RANK_CLOSE_INNER)
}


def private wrap_in_parens(src : Source; lex : Lexical; from, to : int; rule : string;
                           var set : EditSet; closeRank : int = RANK_CLOSE_OUTER) {
    if (to <= from || (text_of(src, from, from + 1) == "(" && paren_close(src, lex, from) == to - 1)) return
    set |> insert(from, "(", "{rule}-open")
    set |> insert(to, ")", "{rule}-close", closeRank)
}


def private if_keyword(src : Source; expr : ExprIfThenElse?) : string {
    if (keyword_at(src, expr.at, "static_elif")) return "static_elif"
    if (keyword_at(src, expr.at, "static_if")) return "static_if"
    return keyword_at(src, expr.at, "elif") ? "elif" : "if"
}


def public rule_if_parens(src : Source; lex : Lexical; expr : ExprIfThenElse?; var set : EditSet) {
    //! `if cond` -> `if (cond)`.
    if (expr.genFlags.generated || expr.cond == null) return
    wrap_expr(src, lex, expr.at, expr.cond, if_keyword(src, expr), "if-cond", set)
}


def public rule_while_parens(src : Source; lex : Lexical; expr : ExprWhile?; var set : EditSet) {
    //! `while cond` -> `while (cond)`.
    if (expr.genFlags.generated || expr.cond == null) return
    wrap_expr(src, lex, expr.at, expr.cond, "while", "while-cond", set)
}


def public rule_with_parens(src : Source; lex : Lexical; expr : ExprWith?; var set : EditSet) {
    //! `with obj` -> `with (obj)`.
    if (expr.genFlags.generated || expr._with == null) return
    wrap_expr(src, lex, expr.at, expr._with, "with", "with-subject", set)
}


def public rule_for_parens(src : Source; lex : Lexical; expr : ExprFor?; var set : EditSet) {
    //! `for x in xs` -> `for (x in xs)`.
    //!
    //! The head runs from the first iterator name to the end of the last source; both ends come
    //! from the tree (`iteratorsAt`, `sources`), so nothing has to be searched for.
    if (expr.genFlags.generated || empty(expr.iteratorsAt) || empty(expr.sources)) return
    var from = span_begin(src, expr.iteratorsAt[0])
    if (!preceded_by(src, from, "for")) {
        let kw = span_begin(src, expr.at)
        if (text_of(src, kw, kw + 3) != "for") return
        from = skip_blank(src, kw + 3, line_end(src, line_at(src, kw)))
        if (from <= kw + 3) return
    }
    var to = from
    for (s in expr.sources) {
        let ext = extent_of(src, lex, s)
        if (ext.ok) {
            to = max(to, ext.to)
        }
    }
    if (to <= from && expr.body != null) {
        to = line_edge(src, lex, span_begin(src, expr.body.at))
    }
    wrap_in_parens(src, lex, from, to, "for-head", set)
}


def public rule_glue_keyword(src : Source; fromEnd, toStart : int; keyword : string;
                             nextBraced : bool; var set : EditSet) {
    //! Pulls `} else {` (or `recover`, `finally`) onto one line.
    //!
    //! Only when the gap between the two blocks holds nothing but that keyword: a comment in
    //! there is somebody's note about the branch, and moving it is worse than leaving the
    //! formatting alone.
    if (toStart <= fromEnd) return
    let gap = text_of(src, fromEnd, toStart)
    if (gap |> strip != keyword) return
    if (nextBraced) {
        set |> replace(fromEnd, toStart, " {keyword} ", "glue-{keyword}", RANK_TERMINATOR)
        return
    }
    set |> replace(fromEnd, skip_back_blank(src, toStart), " {keyword}",
        "glue-{keyword}", RANK_TERMINATOR)
}


def public rule_call_macro_parens(src : Source; lex : Lexical; call : ExprCallMacro?;
                                  var set : EditSet) {
    //! `match x` + an indented block -> `match(x) \{ ... \}`.
    //!
    //! A call macro registered as a keyword (`match`, `static_match`) takes its subject bare in
    //! v1; gen2 spells the call, so the subject needs the parentheses a condition would get.
    if (call.genFlags.generated || empty(call.arguments)) return
    let at = span_begin(src, call.at)
    let name = string(call.name)
    let after = at + length(name)
    if (text_of(src, at, after) != name || text_of(src, after, after + 1) == "(") return
    let ext = extent_of(src, lex, call.arguments[0])
    if (!ext.ok || ext.from <= after || !is_blank(src, after, ext.from)) return
    set |> replace(after, ext.from, "(", "call-macro-open")
    set |> insert(ext.to, ")", "call-macro-close", RANK_CLOSE_INNER)
}


def public rule_typeinfo(src : Source; lex : Lexical; expr : ExprTypeInfo?; var set : EditSet) {
    //! v1 `typeinfo(trait expr)` -> gen2 `typeinfo trait(expr)`.
    //!
    //! The trait name moves out of the parentheses; the closing one is already where gen2 wants
    //! it, so only the opener and the gap after the trait change.
    if (expr.genFlags.generated) return
    let at = span_begin(src, expr.at)
    if (text_of(src, at, at + 8) != "typeinfo") return
    let lp = find_token(src, lex, at + 8, "(")
    if (lp < 0 || !(text_of(src, at + 8, lp) |> strip |> empty)) return
    let traitFrom = skip_blank(src, lp + 1, lp + 32)
    var traitTo = traitFrom
    peek_data(src.text) $(bytes) {
        while (traitTo < length(bytes)) {
            let c = int(bytes[traitTo])
            if (!is_alpha(c) && !is_number(c) && c != '_') break
            traitTo++
        }
    }
    if (traitTo <= traitFrom) return
    set |> replace(lp, lp + 1, " ", "typeinfo-open")
    let argFrom = skip_blank(src, traitTo, paren_close(src, lex, lp))
    if (argFrom > traitTo && is_blank(src, traitTo, argFrom)) {
        set |> replace(traitTo, argFrom, "(", "typeinfo-trait")
        return
    }
    set |> insert(traitTo, "(", "typeinfo-trait")
}


def public rule_let_tuple(src : Source; lex : Lexical; expr : ExprLet?; var set : EditSet) {
    //! v1 tuple destructuring `var [[a, b]] = t` -> gen2 `var (a, b) = t`.
    //!
    //! The brackets are not a make-node here — they name the variables being bound — so no
    //! make rule claims them and this one owns both ends.
    if (expr.genFlags.generated) return
    let at = span_begin(src, expr.at)
    let keyword = text_of(src, at, at + 3)
    if (keyword != "var" && keyword != "let") return
    let bracket = skip_blank(src, at + 3, at + 35)
    if (text_of(src, bracket, bracket + 2) != "[[") return
    let closer = find_closer(src, lex, bracket + 2, "]]")
    if (closer < 0) return
    set |> replace(bracket, bracket + 2, "(", "let-tuple-open")
    set |> replace(closer, closer + 2, ")", "let-tuple-close", RANK_CLOSE_OUTER)
}


def public rule_enum_dot(src : Source; lex : Lexical; at : LineInfo; generated : bool;
                         var set : EditSet) {
    //! v1 names an enumeration or bitfield value `Color green`; gen2 writes `Color.green`.
    //!
    //! The node anchors on the VALUE, so the type name is the identifier before it and the gap
    //! between them is what becomes the dot.
    if (generated) return
    let valStart = span_begin(src, at)
    if (valStart <= 0) return
    var typeEnd = valStart
    peek_data(src.text) $(bytes) {
        while (typeEnd > 0) {
            let c = int(bytes[typeEnd - 1])
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break
            typeEnd--
        }
    }
    if (typeEnd == valStart || text_of(src, typeEnd - 1, typeEnd) == "."
            || has_comment_or_string(lex, typeEnd, valStart)) return
    set |> replace(typeEnd, valStart, ".", "enum-dot")
}


def public rule_type_function_call(src : Source; lex : Lexical; expr : ExprCall?; var set : EditSet) {
    //! v1 calls a `[type_function]` as `take<int>(1, 2)`; gen2 passes the type as an argument:
    //! `take(type<int>, 1, 2)`.
    if (expr.genFlags.generated || empty(expr.arguments)
            || expr.arguments[0].__rtti != "ExprTypeDecl") return
    let nameEnd = span_end(src, expr.at)
    let lt = find_token(src, lex, nameEnd, "<")
    if (lt < 0 || !is_blank(src, nameEnd, lt)) return
    let gt = find_token(src, lex, span_end(src, expr.arguments[0].at), ">")
    if (gt < 0) return
    let lp = find_token(src, lex, gt + 1, "(")
    if (lp < 0 || !is_blank(src, gt + 1, lp)) return
    set |> replace(lt, lt + 1, "(type<", "type-function-open")
    let more = length(expr.arguments) > 1
    set |> replace(gt, lp + 1, more ? ">, " : ">", "type-function-args")
}


def public rule_piped_make(src : Source; lex : Lexical; call : ExprCall?; var set : EditSet) {
    //! `\{a, b\} |> f(x)` -> `f(\{a, b\}, x)`.
    //!
    //! gen2 takes no make-literal on the left of a pipe. The parser has already lowered the pipe
    //! into the call, so the rewrite is to spell the call the way the tree already reads it.
    if (call.genFlags.generated || empty(call.arguments)) return
    let mk = span_begin(src, call.arguments[0].at)
    if (text_of(src, mk, mk + 1) != "\{") return
    let name = span_begin(src, call.at)
    let nameEnd = span_end(src, call.at)
    if (name <= mk) return
    let close = brace_close(src, lex, mk)
    if (close < 0 || close >= name || text_of(src, close + 1, name) |> strip != "|>") return
    let lp = find_token(src, lex, nameEnd, "(")
    if (lp < 0 || !is_blank(src, nameEnd, lp)) return
    set |> insert(mk, "{text_of(src, name, nameEnd)}(", "piped-make-name")
    set |> replace(close + 1, lp + 1, length(call.arguments) > 1 ? ", " : "", "piped-make-pipe")
}


def public rule_return_tuple(src : Source; lex : Lexical; expr : ExprReturn?; var set : EditSet) {
    //! v1 returns several values bare — `return 1, "2", 3.0`; gen2 wants the tuple written as one.
    if (expr.genFlags.generated || expr.subexpr == null
            || expr.subexpr.__rtti != "ExprMakeTuple") return
    let ext = extent_of(src, lex, expr.subexpr)
    if (!ext.ok) return
    wrap_return(src, lex, ext.from, ext.to, set)
}


def private angle_close(src : Source; lex : Lexical; from : int) : int {
    var depth = 0
    var i = from
    while (i < src.size) {
        if (!has_comment_or_string(lex, i, i + 1)) {
            let ch = text_of(src, i, i + 1)
            if (ch == "<") {
                depth++
            } elif (ch == ">") {
                depth--
                if (depth == 0) return i
            }
        }
        i++
    }
    return -1
}


def public rule_cast_parens(src : Source; lex : Lexical; expr : ExprCast?; var set : EditSet) {
    //! v1 writes `reinterpret<int&> a.b`; gen2 casts are call-style — `reinterpret<int&>(a.b)`.
    if (expr.genFlags.generated || expr.subexpr == null) return
    let lt = find_token(src, lex, span_end(src, expr.at), "<")
    if (lt < 0 || !is_blank(src, span_end(src, expr.at), lt)) return
    let gt = angle_close(src, lex, lt)
    if (gt < 0) return
    let ext = extent_of(src, lex, expr.subexpr)
    if (!ext.ok || ext.from <= gt
            || (is_blank(src, gt + 1, ext.from) && text_of(src, gt + 1, gt + 2) == "(")) return
    if (ext.from > gt + 1 && is_blank(src, gt + 1, ext.from)) {
        set |> replace(gt + 1, ext.from, "(", "cast-open")
    } else {
        set |> insert(gt + 1, "(", "cast-open")
    }
    set |> insert(ext.to, ")", "cast-close", RANK_CLOSE_INNER)
}


def private continues_with_operator(src : Source; lex : Lexical; from, to : int) : bool {
    var line = line_at(src, from) + 1
    let last = line_at(src, to - 1)
    while (line <= last) {
        let b = skip_blank(src, line_begin(src, line), line_end(src, line))
        if (b >= from && b < to && !has_comment_or_string(lex, b, b + 1)) {
            let c = text_of(src, b, b + 1)
            if (c == "+" || c == "-" || c == "*" || c == "/" || c == "%" || c == "|" || c == "&"
                    || c == "^" || c == "<" || c == ">" || c == "=" || c == "?") return true
        }
        line++
    }
    return false
}


def public rule_multiline_init(src : Source; lex : Lexical; expr : ExprLet?; var set : EditSet) {
    //! `let x = 1` + a continuation line `+ 2` -> `let x = (1 + 2)`, spelled across the same lines.
    if (expr.genFlags.generated || empty(expr.variables)) return
    for (v in expr.variables) {
        if (v.init == null) continue
        let ext = extent_of(src, lex, v.init)
        if (!ext.ok || contains_block(v.init)
                || !continues_with_operator(src, lex, ext.from, ext.to)) continue
        wrap_in_parens(src, lex, ext.from, ext.to, "multiline-init", set, RANK_CLOSE_INNER)
    }
}
