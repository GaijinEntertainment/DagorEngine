options gen2
options indenting = 4
options strict_smart_pointers = true

module fts5_query shared

//! In-memory parser + evaluator for SQLite's FTS5 query grammar:
//! bare terms, prefix `*`, quoted phrases (with optional trailing `*`),
//! `AND`/`OR`/`NOT` (binary `A NOT B`), parens, and `NEAR(... [, k])`.
//! Column filters (`col:term`, `{a b}:term`) parse but are ignored at
//! match time — the SQL side passes the query through to SQLite which
//! handles column scoping against `[sql_fts5]` tables.

require daslib/utf8_utils
require daslib/result public
require daslib/strings_boost
require math

// ===== AST =====

enum public Fts5NodeKind {
    Term       //!< Single word, optional `*` prefix.
    Phrase     //!< Quoted multi-word phrase, optional trailing `*`.
    AndOp      //!< `A AND B` / whitespace-implicit.
    OrOp       //!< `A OR B`.
    NotOp      //!< `A NOT B` (binary).
    NearOp     //!< `NEAR(t1 t2 ... , k)`.
    ColFilter  //!< `col:expr` or `{c1 c2}:expr`.
}

struct public Fts5Node {
    //! Per-kind use: Term/Phrase/NearOp store words[]; AndOp/OrOp/NotOp/ColFilter
    //! use left/right node indices; NearOp uses near_k; ColFilter uses columns.
    kind : Fts5NodeKind = Fts5NodeKind.Term
    words : array<string>
    is_prefix : bool = false
    left : int = -1
    right : int = -1
    near_k : int = 10
    columns : array<string>
}

struct public Fts5Query {
    //! `root == -1` means empty query (matches everything).
    nodes : array<Fts5Node>
    root : int = -1
}

// ===== Parser =====

struct private ParseState {
    src : array<uint>          // UTF-32 codepoints
    pos : int = 0
    err : string
    nodes : array<Fts5Node>    // accumulator for the AST
}

def private _push_node(var st : ParseState; var n : Fts5Node) : int {
    let idx = length(st.nodes)
    st.nodes |> emplace(n)
    return idx
}

def private _is_space(cp : uint) : bool {
    return cp == 0x20u || cp == 0x09u || cp == 0x0Au || cp == 0x0Du
}

def private _skip_ws(var st : ParseState) {
    let n = length(st.src)
    while (st.pos < n && _is_space(st.src[st.pos])) {
        st.pos++
    }
}

def private _peek(var st : ParseState) : uint {
    if (st.pos >= length(st.src)) return 0u
    return st.src[st.pos]
}

def private _is_word_cp(cp : uint) : bool {
    return utf32_is_word_char(cp)
}

def private _consume_name(var st : ParseState) : string {
    var bytes : array<uint8>
    let n = length(st.src)
    while (st.pos < n && _is_word_cp(st.src[st.pos])) {
        bytes |> utf8_encode(utf32_to_lower(st.src[st.pos]))
        st.pos++
    }
    return string(bytes)
}

def private _peek_keyword(var st : ParseState; kw : string) : bool {
    // True iff next token (after ws) is the case-sensitive keyword `kw` (FTS5 keywords are uppercase).
    _skip_ws(st)
    let n = length(st.src)
    let kw_codepoints = utf8_decode(kw)
    let m = length(kw_codepoints)
    if (st.pos + m > n) return false
    for (i in range(m)) {
        if (st.src[st.pos + i] != kw_codepoints[i]) return false
    }
    // Boundary: kw must be followed by non-word char or EOF.
    let after = st.pos + m
    return !(after < n && _is_word_cp(st.src[after]))
}

def private _consume_keyword(var st : ParseState; kw : string) {
    let kw_codepoints = utf8_decode(kw)
    st.pos += length(kw_codepoints)
}

def private _set_err(var st : ParseState; msg : string) {
    if (empty(st.err)) {
        st.err = "{msg} at position {st.pos}"
    }
}

def private _parse_phrase(var st : ParseState) : int {
    // Caller has already verified next char is '"'.
    st.pos++   // skip opening "
    var n = Fts5Node(kind = Fts5NodeKind.Phrase)
    let len = length(st.src)
    while (st.pos < len) {
        _skip_ws(st)
        if (st.pos >= len) break
        let cp = st.src[st.pos]
        if (cp == uint('"')) {
            st.pos++
            // optional trailing *
            if (st.pos < len && st.src[st.pos] == uint('*')) {
                n.is_prefix = true
                st.pos++
            }
            return _push_node(st, n)
        }
        if (!_is_word_cp(cp)) {
            _set_err(st, "unexpected character in phrase")
            return -1
        }
        n.words |> push(_consume_name(st))
    }
    _set_err(st, "unterminated phrase")
    return -1
}

def private _parse_near(var st : ParseState) : int {
    // Caller verified next is "NEAR" keyword. Consume it.
    _consume_keyword(st, "NEAR")
    _skip_ws(st)
    if (_peek(st) != uint('(')) {
        _set_err(st, "expected '(' after NEAR")
        return -1
    }
    st.pos++
    var n = Fts5Node(kind = Fts5NodeKind.NearOp, near_k = 10)
    let len = length(st.src)
    var closed_paren = false
    while (st.pos < len) {
        _skip_ws(st)
        let cp = _peek(st)
        if (cp == uint(')')) {
            st.pos++
            closed_paren = true
            break
        }
        if (cp == uint(',')) {
            st.pos++
            _skip_ws(st)
            // parse k as integer
            var k = 0
            var any_digit = false
            while (st.pos < len) {
                let dc = st.src[st.pos]
                if (dc < uint('0') || dc > uint('9')) break
                k = k * 10 + int(dc - uint('0'))
                any_digit = true
                st.pos++
            }
            if (!any_digit) {
                _set_err(st, "expected integer after ',' in NEAR")
                return -1
            }
            n.near_k = k
            _skip_ws(st)
            if (_peek(st) != uint(')')) {
                _set_err(st, "expected ')' after NEAR distance")
                return -1
            }
            st.pos++
            closed_paren = true
            break
        }
        if (cp == uint('"')) {
            // FTS5 NEAR can take phrases too — flatten by taking the words.
            st.pos++
            var closed_quote = false
            while (st.pos < len) {
                _skip_ws(st)
                if (_peek(st) == uint('"')) {
                    st.pos++
                    closed_quote = true
                    break
                }
                if (!_is_word_cp(_peek(st))) {
                    _set_err(st, "unexpected character in NEAR phrase")
                    return -1
                }
                n.words |> push(_consume_name(st))
            }
            if (!closed_quote) {
                _set_err(st, "unterminated phrase in NEAR")
                return -1
            }
            continue
        }
        if (!_is_word_cp(cp)) {
            _set_err(st, "unexpected character in NEAR")
            return -1
        }
        n.words |> push(_consume_name(st))
    }
    if (!closed_paren) {
        _set_err(st, "unterminated NEAR — expected ')'")
        return -1
    }
    if (length(n.words) < 2) {
        _set_err(st, "NEAR requires at least two terms")
        return -1
    }
    return _push_node(st, n)
}

def private _parse_term(var st : ParseState) : int {
    var n = Fts5Node(kind = Fts5NodeKind.Term)
    n.words |> push(_consume_name(st))
    if (st.pos < length(st.src) && st.src[st.pos] == uint('*')) {
        n.is_prefix = true
        st.pos++
    }
    return _push_node(st, n)
}

def private _parse_col_filter_inner(var st : ParseState; var columns : array<string>) : int {
    _skip_ws(st)
    if (_peek(st) != uint(':')) {
        _set_err(st, "expected ':' after column name")
        return -1
    }
    st.pos++
    let inner = _parse_unary(st)
    if (inner == -1) return -1
    var n = Fts5Node(kind = Fts5NodeKind.ColFilter, left = inner)
    n.columns <- columns
    return _push_node(st, n)
}

def private _parse_unary(var st : ParseState) : int {
    _skip_ws(st)
    let len = length(st.src)
    if (st.pos >= len) {
        _set_err(st, "unexpected end of query")
        return -1
    }
    let cp = st.src[st.pos]
    // Parenthesized subexpr
    if (cp == uint('(')) {
        st.pos++
        let inner = _parse_or_expr(st)
        _skip_ws(st)
        if (_peek(st) != uint(')')) {
            _set_err(st, "expected ')'")
            return -1
        }
        st.pos++
        return inner
    }
    // Quoted phrase
    if (cp == uint('"')) return _parse_phrase(st)
    // Column-set filter: { col1 col2 ... } : expr
    if (cp == uint('{')) {
        st.pos++
        var cols : array<string>
        while (st.pos < len) {
            _skip_ws(st)
            let pcp = _peek(st)
            if (pcp == uint('}')) {
                st.pos++
                break
            }
            if (!_is_word_cp(pcp)) {
                _set_err(st, "expected column name in column-set filter")
                return -1
            }
            cols |> push(_consume_name(st))
        }
        return _parse_col_filter_inner(st, cols)
    }
    // NEAR keyword
    if (_peek_keyword(st, "NEAR")) return _parse_near(st)
    // Bare name — term, operator keyword (caller's job), or "name : expr" (single-column filter).
    if (!_is_word_cp(cp)) {
        _set_err(st, "unexpected character")
        return -1
    }
    let save = st.pos
    let name = _consume_name(st)
    // If immediately followed by ':' (no whitespace), treat as col-filter.
    if (_peek(st) == uint(':')) {
        var cols : array<string>
        cols |> push(name)
        return _parse_col_filter_inner(st, cols)
    }
    // Otherwise it's a term — back up and reparse to also catch '*'.
    st.pos = save
    return _parse_term(st)
}

def private _at_expr_terminator(var st : ParseState) : bool {
    _skip_ws(st)
    if (st.pos >= length(st.src)) return true
    let cp = _peek(st)
    return cp == uint(')') || cp == uint('}') || cp == uint(',')
}

def private _parse_and_expr(var st : ParseState) : int {
    var left = _parse_unary(st)
    if (left == -1 || !empty(st.err)) return left
    while (true) {
        if (_at_expr_terminator(st) || _peek_keyword(st, "OR") || _peek_keyword(st, "NOT")) break
        let had_explicit_and = _peek_keyword(st, "AND")
        if (had_explicit_and) {
            _consume_keyword(st, "AND")
            _skip_ws(st)
        }
        let right = _parse_unary(st)
        if (right == -1 || !empty(st.err)) return -1
        var n = Fts5Node(kind = Fts5NodeKind.AndOp, left = left, right = right)
        left = _push_node(st, n)
    }
    return left
}

def private _parse_not_expr(var st : ParseState) : int {
    let left = _parse_and_expr(st)
    if (left == -1 || !empty(st.err) || !_peek_keyword(st, "NOT")) return left
    _consume_keyword(st, "NOT")
    _skip_ws(st)
    let right = _parse_and_expr(st)
    if (right == -1 || !empty(st.err)) return -1
    var n = Fts5Node(kind = Fts5NodeKind.NotOp, left = left, right = right)
    return _push_node(st, n)
}

def private _parse_or_expr(var st : ParseState) : int {
    var left = _parse_not_expr(st)
    if (left == -1 || !empty(st.err)) return left
    while (_peek_keyword(st, "OR")) {
        _consume_keyword(st, "OR")
        _skip_ws(st)
        let right = _parse_not_expr(st)
        if (right == -1 || !empty(st.err)) return -1
        var n = Fts5Node(kind = Fts5NodeKind.OrOp, left = left, right = right)
        left = _push_node(st, n)
    }
    return left
}

// ===== Public API =====

def public try_compile_text_match(query : string) : Result<Fts5Query; string> {
    //! Parse an FTS5 query into an `Fts5Query`. Empty / whitespace-only is
    //! valid and yields `root == -1` (matches everything). `Err` carries
    //! the position of the first parse error.
    var st = default<ParseState>
    st.src <- utf8_decode(query)
    _skip_ws(st)
    if (st.pos >= length(st.src)) return ok(default<Fts5Query>, type<string>)
    let root_idx = _parse_or_expr(st)
    if (!empty(st.err)) return err(st.err, type<Fts5Query>)
    _skip_ws(st)
    if (st.pos < length(st.src)) return err("trailing input at position {st.pos}", type<Fts5Query>)
    var q <- Fts5Query(nodes <- st.nodes, root = root_idx) // nolint:LINT003
    return move_ok(q, type<string>)
}

def public compile_text_match(query : string) : Fts5Query {
    //! Strict variant — panics on parse error. Use `try_compile_text_match` for user input.
    var r <- try_compile_text_match(query)
    if (is_err(r)) {
        panic("invalid FTS5 query: {unwrap_err(r)}")
    }
    return <- move_unwrap(r)
}

// ===== Evaluator =====

def private _term_matches_token(term : string; is_prefix : bool; tok : string) : bool {
    if (is_prefix) return tok |> starts_with(term)
    return tok == term
}

def private _phrase_match_pos(words : array<string>; is_prefix : bool;
                              tokens : array<string>; start : int) : bool {
    let m = length(words)
    if (start + m > length(tokens)) return false
    for (i in range(m)) {
        let last = (i == m - 1)
        if (last && is_prefix) {
            if (!(tokens[start + i] |> starts_with(words[i]))) return false
        } else {
            if (tokens[start + i] != words[i]) return false
        }
    }
    return true
}

def private _phrase_match(words : array<string>; is_prefix : bool;
                          tokens : array<string>) : bool {
    if (empty(words)) return true
    let n = length(tokens)
    for (i in range(n)) {
        if (_phrase_match_pos(words, is_prefix, tokens, i)) return true
    }
    return false
}

def private _near_match(words : array<string>; k : int; tokens : array<string>) : bool {
    // FTS5 NEAR: every word appears within k tokens of every other.
    let m = length(words)
    if (m == 0) return true
    var positions : array<array<int>>
    positions |> resize(m)
    for (i in range(m)) {
        for (j in range(length(tokens))) {
            if (tokens[j] == words[i]) {
                positions[i] |> push(j)
            }
        }
        if (empty(positions[i])) return false
    }
    for (p0 in positions[0]) {
        var lo = p0
        var hi = p0
        var ok = true
        for (i in range(1, m)) {
            var best = -1
            var best_span = -1
            for (q in positions[i]) {
                let new_lo = min(lo, q)
                let new_hi = max(hi, q)
                let span = new_hi - new_lo
                if (span <= k && (best_span == -1 || span < best_span)) {
                    best = q
                    best_span = span
                }
            }
            if (best == -1) {
                ok = false
                break
            }
            lo = min(lo, best)
            hi = max(hi, best)
        }
        if (ok) return true
    }
    return false
}

def private _eval(q : Fts5Query; idx : int; tokens : array<string>) : bool {
    if (idx < 0) return true
    let kind = q.nodes[idx].kind
    if (kind == Fts5NodeKind.Term) {
        let w = q.nodes[idx].words[0]
        let is_prefix = q.nodes[idx].is_prefix
        if (empty(w)) return true
        for (tok in tokens) {
            if (_term_matches_token(w, is_prefix, tok)) return true
        }
        return false
    }
    if (kind == Fts5NodeKind.Phrase) return _phrase_match(q.nodes[idx].words, q.nodes[idx].is_prefix, tokens)
    if (kind == Fts5NodeKind.AndOp) return _eval(q, q.nodes[idx].left, tokens) && _eval(q, q.nodes[idx].right, tokens)
    if (kind == Fts5NodeKind.OrOp) return _eval(q, q.nodes[idx].left, tokens) || _eval(q, q.nodes[idx].right, tokens)
    if (kind == Fts5NodeKind.NotOp) return _eval(q, q.nodes[idx].left, tokens) && !_eval(q, q.nodes[idx].right, tokens)
    if (kind == Fts5NodeKind.NearOp) return _near_match(q.nodes[idx].words, q.nodes[idx].near_k, tokens)
    if (kind == Fts5NodeKind.ColFilter) return _eval(q, q.nodes[idx].left, tokens)
    return false
}

def public text_match(text : string; q : Fts5Query) : bool {
    //! Match `text` against a precompiled `Fts5Query`.
    if (q.root < 0) return true
    var tokens : array<string>
    each_word(text, tokens)
    return _eval(q, q.root, tokens)
}

def public text_match(text, query : string) : bool {
    //! Compile + match. For repeated matching, call `compile_text_match` once and reuse.
    let q <- compile_text_match(query)
    return text_match(text, q)
}
