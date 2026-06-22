options gen2

module linq_das

require strings
require daslib/strings_boost
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/linq_boost public

// ===== C#-style LINQ query syntax (linq_das) — from/where/orderby/join/from/select|group over any _fold source =====
//
// The `%linq! … %%` inline reader macro rewrites a C#-like query into a `_fold(...)` chain:
//
//     %linq! from c in cars where c.price > 100 orderby c.price select c.name %%
//        →   ( _fold( each(cars) |> _where($(c) => c.price > 100) |> _order_by($(c) => c.price) |> _select($(c) => c.name) |> to_array() ) )
//     %linq! from c in cars group c by c.brand %%
//        →   ( _fold( each(cars) |> _group_by_lazy($(c) => c.brand) |> to_array() ) )
//     %linq! from c in cars join d in dealers on c.brand equals d.brand select (N = c.name, City = d.city) %%
//        →   ( _fold( each(cars) |> _join(each(dealers), $(c, d) => c.brand == d.brand, $(c, d) => (N = c.name, City = d.city)) |> to_array() ) )
//     %linq! from c in cars from d in dealers select (N = c.name, City = d.city) %%
//        →   ( _fold( (cars) |> _cross_join((dealers), $(c, d) => (N = c.name, City = d.city)) |> to_array() ) )
//
// An untyped `from c in <arr>` is an array source. A typed `from c : Row in <src>` selects a non-array
// source: `decs` (a keyword marker → `from_decs_template`), or any value whose type the `from_in` call
// macro dispatches (a SQL runner → `select_from`, an XML node → `from_xml_node`). The range variable is
// spliced verbatim as the block parameter. `orderby <expr> [descending]` is a single sort key. `group c
// by <key>` is a terminal yielding `tuple<key; array<elem>>` per bucket — IGrouping, in-memory sources
// only (over SQL it errors: SQL needs an aggregating projection). A trailing `iterator` keyword yields an
// `iterator<T>` over the materialized (fused) result (via `to_sequence()`) instead of the default `array<T>`.
//
// `join <d> [: RowB] in <B> on <keyA> equals <keyB>` is a single inner equi-join (see transpile_join). A
// `where` before `join` filters the left source (single range var, pushes down to SQL). A plain `join …
// select <proj>` is select-terminal — the `select` becomes the join's result row, both `c` and `d` in
// scope. A scalar or named-tuple projection pushes down to SQL; a whole-row `select c` is in-memory only
// (over SQL it has no column form). Post-join `where`/`orderby`, and a `group` terminal, carry both `c`
// and `d` past the join as a `(c, d)` pair (transparent identifier) and run in-memory (over SQL the
// carried whole-row tuple errors — project columns in a select-terminal join).
//
// A second `from <d> [: RowB] in <B>` is SelectMany (see transpile_cross) — the same clause grammar and
// dual emit as `join`, minus the `on … equals` key. Two flavors by whether `B` references `c`: uncorrelated
// (`B` independent) is the cross product, emitting `_cross_join` (pushes down to a SQL CROSS JOIN);
// correlated (`from d in c.items`) is the flatten, emitting `_select_many` (in-memory; rejected over SQL —
// the inner collection is a per-row value, not a table). join and both crosses share the post-source tail
// (emit_two_source). One extra `from` only; `from … from` + `join` in one query is rejected.

def private is_ident_byte(c : int) : bool {
    return is_alnum(c) || c == '_'
}

// Replace `//` line and `/* */` block comments with spaces (newlines preserved, so the spliced text's
// line/column stay accurate), skipping comment markers inside string literals.
def private strip_query_comments(query : string) : string {
    return modify_data(query) $(var arr) {
        let n = length(arr)
        var i = 0
        var inStr = false
        while (i < n) {
            let c = int(arr[i])
            if (inStr) {
                if (c == '\\') { i += 2; continue }
                if (c == '"') { inStr = false }
                i++
                continue
            }
            if (c == '"') { inStr = true; i++; continue }
            if (c == '/' && i + 1 < n && int(arr[i + 1]) == '/') {
                while (i < n && int(arr[i]) != '\n' && int(arr[i]) != '\r') { arr[i] = uint8(' '); i++ }
                continue
            }
            if (c == '/' && i + 1 < n && int(arr[i + 1]) == '*') {
                arr[i] = uint8(' '); arr[i + 1] = uint8(' '); i += 2
                while (i < n) {
                    if (int(arr[i]) == '*' && i + 1 < n && int(arr[i + 1]) == '/') {
                        arr[i] = uint8(' '); arr[i + 1] = uint8(' '); i += 2
                        break
                    }
                    if (int(arr[i]) != '\n' && int(arr[i]) != '\r') { arr[i] = uint8(' ') }
                    i++
                }
                continue
            }
            i++
        }
    }
}

// Byte offset of the next whole-word `kw` at paren/bracket/brace depth 0 in q[from..], or -1.
// String literals are skipped so a keyword inside `"..."` never matches, and a word immediately
// following `.` (a member access like `c.where`) is not treated as a clause keyword.
def private find_kw_depth0(q : string; kw : string; from : int) : int {
    var found = -1
    let m = length(kw)
    peek_data(q) $(arr) {
        let n = length(arr)
        var depth = 0
        var inStr = false
        var prevSig = 0    // last significant (non-whitespace) byte, 0 = none
        var prevSig2 = 0   // significant byte before prevSig — disambiguates a `>` tail
        var i = from
        while (i < n && found < 0) {
            let c = int(arr[i])
            if (inStr) {
                if (c == '\\') { i += 2; continue }
                if (c == '"') { inStr = false; prevSig2 = prevSig; prevSig = c }
                i++
                continue
            }
            if (c == '"') { inStr = true; prevSig2 = prevSig; prevSig = c; i++; continue }
            if (c == '(' || c == '[' || c == '{') { depth++; prevSig2 = prevSig; prevSig = c; i++; continue }
            if (c == ')' || c == ']' || c == '}') { depth--; prevSig2 = prevSig; prevSig = c; i++; continue }
            // A clause keyword is a depth-0 whole word NOT preceded (ignoring whitespace) by `.` (member access)
            // or by a `>` that is the tail of a pipe `|>` / lambda arrow `=>` / `->` — so an aggregate like
            // `g |> select(...) |> sum` inside an orderby/where body is not mistaken for a `select` clause. A `>`
            // from a type/generic bracket (`default<int>`, `type<Foo>`) or comparison is NOT excluded, so a
            // clause keyword that legitimately follows one (e.g. `let d = default<int> select …`) still matches.
            let afterPipeTail = prevSig == '>' && (prevSig2 == '|' || prevSig2 == '=' || prevSig2 == '-')
            let atWordStart = is_ident_byte(c) && ((i == 0) || !is_ident_byte(int(arr[i - 1]))) && prevSig != '.' && !afterPipeTail
            if (depth == 0 && atWordStart && i + m <= n && slice(q, i, i + m) == kw) {
                if (i + m == n || !is_ident_byte(int(arr[i + m]))) {
                    found = i
                }
            }
            if (!is_white_space(c)) { prevSig2 = prevSig; prevSig = c }
            i++
        }
    }
    return found
}

// Split `text` on top-level `,` (paren/bracket/brace depth 0, outside string literals) and return the
// stripped segments. Used to parse a multi-key `orderby k1, k2 desc, k3` without splitting inside f(x,y)/(a,b).
def private split_depth0(text : string) : array<string> {
    var segs : array<string>
    peek_data(text) $(arr) {
        let n = length(arr)
        segs |> reserve(4)   // orderby clauses are short (≤4 keys); avoids realloc on the common path
        var depth = 0
        var inStr = false
        var segStart = 0
        var i = 0
        while (i < n) {
            let c = int(arr[i])
            if (inStr) {
                if (c == '\\') { i += 2; continue }
                if (c == '"') { inStr = false }
                i++
                continue
            }
            if (c == '"') { inStr = true; i++; continue }
            if (c == '(' || c == '[' || c == '{') { depth++; i++; continue }
            if (c == ')' || c == ']' || c == '}') { depth--; i++; continue }
            if (depth == 0 && c == ',') {
                segs |> push(strip(slice(text, segStart, i)))
                segStart = i + 1
            }
            i++
        }
        segs |> push(strip(slice(text, segStart, n)))
    }
    return <- segs
}

struct private OrderKeys {
    keys     : array<string>
    descMask : uint           // bit i → key i descending (LSB = first key)
    ok       : bool           // false on an empty / direction-only segment
}

// Parse an orderby clause body — `k1 [asc|desc], k2 [asc|desc], …` — into keys + per-key descending mask.
def private parse_orderby(rawOrder : string) : OrderKeys {
    var r : OrderKeys
    r.ok = true
    let segs <- split_depth0(rawOrder)
    r.keys |> reserve(length(segs))
    var idx = 0u
    for (seg in segs) {
        let dseg = strip_trailing_keyword(seg, "descending")
        let keyText = dseg._1 ? dseg._0 : strip_trailing_keyword(seg, "ascending")._0
        if (keyText == "" || keyText == "descending" || keyText == "ascending") {
            r.ok = false
            return <- r
        }
        r.keys |> push(keyText)
        // idx < 32u guards the shift on absurd key counts; the caller rejects > 4 keys with a clean message.
        if (dseg._1 && idx < 32u) { r.descMask = r.descMask | (1u << idx) }
        idx++
    }
    if (empty(r.keys)) { r.ok = false }
    return <- r
}

// Build the order pipe fragment. `ti` is the lambda parameter name; `keys` are already rewritten for the
// emit context. Single key → `_order_by` / `_order_by_descending`; multi-key → `_order_by_keys((…), mask)`.
def private emit_order_op(keys : array<string>; descMask : uint; ti : string) : string {
    if (length(keys) == 1) {
        let op = (descMask & 1u) != 0u ? "_order_by_descending" : "_order_by"
        return " |> {op}($({ti}) => {keys[0]})"
    }
    return " |> _order_by_keys($({ti}) => ({join(keys, ", ")}), {descMask}u)"
}

// Read the identifier at/after `from` (skipping leading whitespace); returns (name, endOffset).
// name is "" when no identifier is present.
def private read_ident(q : string; from : int) : tuple<string; int> {
    var s = -1
    var e = -1
    peek_data(q) $(arr) {
        let n = length(arr)
        var i = from
        while (i < n && !is_ident_byte(int(arr[i]))) { i++ }
        s = i
        while (i < n && is_ident_byte(int(arr[i]))) { i++ }
        e = i
    }
    if (s < 0 || e <= s) return ("", -1)
    return (slice(q, s, e), e)
}

// If `text` ends with a standalone `kw` keyword (preceded by whitespace, so a member access like
// `c.iterator` / `c.descending` is not matched), strip it and return (remainder, true).
def private strip_trailing_keyword(text : string; kw : string) : tuple<string; bool> {
    let p = strip(text)
    let m = length(kw)
    let n = length(p)
    if (n <= m || !ends_with(p, kw)) return (p, false)
    var wsBefore = false
    peek_data(p) $(arr) {
        wsBefore = is_white_space(int(arr[n - m - 1]))
    }
    if (wsBefore) return (strip(slice(p, 0, n - m)), true)
    return (p, false)
}

// Source builder for a clause. `fused` distinguishes a source that will be a fused `_fold` chain head /
// fused-op source (single-`from`, `join` — keep `each` so the fusion picks the array up as a loop source)
// from an UNFUSED operator argument (the uncorrelated multiple-`from`, whose `_cross_join` runs at tier-3
// passthrough — pass the bare array so `each`'s `[unsafe_outside_of_for]` does not trip and the array×array
// cross_join overload is selected). array (untyped) → each(...) / bare; decs (keyword marker) →
// from_decs_template; any other typed value source → from_in (dispatches by the source value's type).
def private build_src(rowType : string; srcText : string; fused : bool) : string {
    if (rowType == "") return fused ? "each({srcText})" : "({srcText})"
    if (srcText == "decs") return "from_decs_template(type<{rowType}>)"
    return "from_in({srcText}, type<{rowType}>)"
}

// Whole-word identifier substitution over query-clause text: a word found as a key in `subs` is replaced
// with its mapped string, anything else is copied verbatim. A word immediately following `.` (a member
// access) is left alone, and depth is irrelevant — refs at any paren depth are substituted. Plain string
// content is copied verbatim, but a string INTERPOLATION (`"…{expr}…"`) is daslang code, so its `{…}` body
// is substituted too (with one level of nested string literal inside the interpolation copied verbatim).
// Backs both the transparent-identifier rewrite (rewrite_idents) and `let`-binding inlining (extract_lets).
def private substitute_idents(text : string; subs : table<string; string>) : string {
    return build_string() $(var w) {
        peek_data(text) $(arr) {
            let n = length(arr)
            var i = 0
            var inStr = false   // inside a "…" literal (plain content portion)
            var interp = 0      // brace depth inside a string interpolation (>0 ⇒ code mode within the string)
            var prevSig = 0     // last significant (non-whitespace) byte, 0 = none
            while (i < n) {
                let c = int(arr[i])
                // Plain string content (not in an interpolation): copy verbatim; `{` opens interpolation, `"` closes the string.
                if (inStr && interp == 0) {
                    if (c == '\\' && i + 1 < n) { w |> write(slice(text, i, i + 2)); i += 2; continue }
                    w |> write(slice(text, i, i + 1))
                    if (c == '"') { inStr = false; prevSig = c }
                    elif (c == '{') { interp = 1; prevSig = 0 }
                    i++
                    continue
                }
                // Code mode — top-level, or inside a string interpolation.
                if (c == '"') {
                    if (interp > 0) {   // nested string inside interpolation — copy verbatim, no rewrite
                        w |> write(slice(text, i, i + 1)); i++
                        while (i < n) {
                            let cc = int(arr[i])
                            if (cc == '\\' && i + 1 < n) { w |> write(slice(text, i, i + 2)); i += 2; continue }
                            w |> write(slice(text, i, i + 1)); i++
                            if (cc == '"') break
                        }
                        prevSig = '"'
                        continue
                    }
                    inStr = true; prevSig = c; w |> write(slice(text, i, i + 1)); i++
                    continue
                }
                if (interp > 0 && c == '{') { interp ++; w |> write(slice(text, i, i + 1)); prevSig = c; i++; continue }
                if (interp > 0 && c == '}') {
                    interp --
                    w |> write(slice(text, i, i + 1))
                    prevSig = interp == 0 ? '"' : c   // back to plain string content when the interpolation closes
                    i++
                    continue
                }
                let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1])))
                if (atWordStart) {
                    var j = i
                    while (j < n && is_ident_byte(int(arr[j]))) { j++ }
                    let word = slice(text, i, j)
                    w |> write(prevSig != '.' ? (subs?[word] ?? word) : word)
                    prevSig = int(arr[j - 1])
                    i = j
                    continue
                }
                w |> write(slice(text, i, i + 1))
                if (!is_white_space(c)) { prevSig = c }
                i++
            }
        }
    }
}

// Rewrite whole-word identifiers named in `names` to `prefix.<name>` (the transparent-identifier
// substitution after a join). Thin wrapper over substitute_idents — see it for the string/interp rules.
def private rewrite_idents(text : string; names : array<string>; prefix : string) : string {
    let subs <- {for (nm in names); nm => "{prefix}.{nm}"}
    return substitute_idents(text, subs)
}

// Does `text` reference the whole-word identifier `name` in **code** position? A word right after `.`
// (member access) is ignored, plain ``"…"`` string content is skipped, but a string INTERPOLATION
// (``"…{expr}…"``) is code, so ``{…}`` bodies ARE scanned (same string/interp model as rewrite_idents).
// Used to detect a CORRELATED second `from` (``from x in o.items``) and to decide whether a projection
// uses a range variable (so the generated lambda can underscore the ones it doesn't — no spurious LINT013).
def private mentions_ident(text : string; name : string) : bool {
    var found = false
    let m = length(name)
    peek_data(text) $(arr) {
        let n = length(arr)
        var i = 0
        var inStr = false   // inside a "…" literal (plain content portion)
        var interp = 0      // brace depth inside a string interpolation (>0 ⇒ code mode within the string)
        var prevSig = 0
        while (i < n && !found) {
            let c = int(arr[i])
            // Plain string content (not in an interpolation): skip; `{` opens interpolation, `"` closes.
            if (inStr && interp == 0) {
                if (c == '\\' && i + 1 < n) { i += 2; continue }
                if (c == '"') { inStr = false; prevSig = c }
                elif (c == '{') { interp = 1; prevSig = 0 }
                i++
                continue
            }
            // Code mode — top-level, or inside an interpolation.
            if (c == '"') {
                if (interp > 0) {   // nested string inside interpolation — skip its content
                    i++
                    while (i < n) {
                        let cc = int(arr[i])
                        if (cc == '\\' && i + 1 < n) { i += 2; continue }
                        i++
                        if (cc == '"') break
                    }
                    prevSig = '"'
                    continue
                }
                inStr = true; prevSig = c; i++
                continue
            }
            if (interp > 0 && c == '{') { interp++; prevSig = c; i++; continue }
            if (interp > 0 && c == '}') {
                interp--
                prevSig = interp == 0 ? '"' : c
                i++
                continue
            }
            let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1]))) && prevSig != '.'
            if (atWordStart && i + m <= n && slice(text, i, i + m) == name
                    && (i + m == n || !is_ident_byte(int(arr[i + m])))) {
                found = true
            }
            if (!is_white_space(c)) { prevSig = c }
            i++
        }
    }
    return found
}

// Synthetic single parameter that carries the joined `(c, d)` pair past a two-source op (transparent
// identifier). Post-source clauses are rewritten to address the carried fields `linq_join_ti.c` /
// `linq_join_ti.d`. Reserved — a range variable may not use this name (`__`-prefixed names are
// reserved by the compiler, so this stays single-token).
let JOIN_TI = "linq_join_ti"

// Shared tail for the two-range-variable forms — `join … on … equals …` (transpile_join) and the
// uncorrelated multiple-`from` / SelectMany (transpile_cross). Given the first source (already built),
// the second range variable, and `tailStart` (the offset where the variable-length "tail" begins — the
// join key B for `join`, the second source expression for the second `from`), it parses the post-source
// clauses (a pre- or post-source `where`, one `orderby`, the `select`|`group` terminal, a trailing
// `iterator`) and emits the `_fold(...)` chain. Two emit shapes, decided textually (the reader runs
// before type inference and cannot know the source):
//   - select-terminal (no post-source where/orderby, terminal = select): the projection IS the op's
//     result selector — both range vars in scope, spliced verbatim. Works on all 4 sources (SQL pushdown).
//   - transparent-identifier (post-source where/orderby, or a group terminal): the op carries
//     `(c = c, d = d)`; post-source clauses rewrite `c`/`d` to the carried fields and stream/sort/group
//     the pairs directly. In-memory only (over SQL the carried whole-row tuple trips `_sql`'s column-ref
//     check — the documented boundary). group yields `(key, array<pair>)` per bucket.
// `isJoin` selects `_join` (with the `keyA equals <tail>` predicate lambda) vs `_cross_join` (no key);
// for cross the second source is built from `tail` here, after a correlation check.
def private emit_two_source(q : string; nq : int; varName : string; var2Name : string; srcA : string;
                            isJoin : bool; keyA : string; srcBForJoin : string; rowType2 : string;
                            secondIntroPos : int; tailStart : int; firstWhere : int;
                            prog : ProgramPtr; at : LineInfo) : string {
    if (varName == JOIN_TI) {
        macro_error(prog, at, "linq: range variable name '{JOIN_TI}' is reserved")
        return "(0)"
    }
    if (var2Name == JOIN_TI || var2Name == varName) {
        macro_error(prog, at, "linq: second range variable '{var2Name}' must differ from '{varName}' and the reserved '{JOIN_TI}'")
        return "(0)"
    }
    // `into` after a `join … on … equals …` is a group-join (C# GroupJoin) — bound below once the terminal is
    // located. Over a second `from` (cross / SelectMany) it is not supported; single-source query-continuation
    // `into` lives in transpile_query.
    let intoPos = find_kw_depth0(q, "into", tailStart)
    if (intoPos >= 0 && !isJoin) {
        macro_error(prog, at, "linq: 'into' over a second 'from' is not supported — group-join 'into' works after a 'join … on … equals …' clause; query-continuation 'into' works on single-source queries")
        return "(0)"
    }
    let orderByPos = find_kw_depth0(q, "orderby", tailStart)
    let selectPos = find_kw_depth0(q, "select", tailStart)
    let groupPos = find_kw_depth0(q, "group", tailStart)
    if (selectPos < 0 && groupPos < 0) {
        macro_error(prog, at, "linq: query must end with a 'select' or 'group … by …' clause")
        return "(0)"
    }
    if (selectPos >= 0 && groupPos >= 0) {
        macro_error(prog, at, "linq: query cannot have both 'select' and 'group' — they are alternative terminals")
        return "(0)"
    }
    let isGroup = groupPos >= 0
    let termPos = isGroup ? groupPos : selectPos
    // A query-continuation `into` AFTER the terminal (`select … into x …`) is single-source only — a join /
    // multiple-`from` carry can't rebind the (outer, inner) pair. Reject it clearly instead of letting the
    // `into …` text fall into the projection (it otherwise mis-parses as a low-level syntax error). This also
    // catches a trailing continuation `into` after a valid pre-terminal group-join `into g`.
    let postTermInto = find_kw_depth0(q, "into", isGroup ? groupPos + 5 : selectPos + 6)
    if (postTermInto >= 0) {
        macro_error(prog, at, "linq: query-continuation 'into' after the 'select'/'group' terminal is not supported on a 'join' or multiple-'from' query — it works on single-source queries; a group-join 'into' goes before the terminal (`join … equals … into g select …`)")
        return "(0)"
    }
    if (isGroup && orderByPos >= 0) {
        macro_error(prog, at, "linq: 'orderby' before 'group' is not supported — order the groups after a 'select', or drop the 'orderby'")
        return "(0)"
    }
    // `join … into <g>`: group-join. `g` binds the array of matches, in scope with the outer range variable in
    // the terminal `select`. This release is SELECT-TERMINAL only (a pre-join `where` still filters the left
    // source); a post-`into` where/orderby/group over the (outer, g) pair needs the non-copyable match-array
    // carry — not yet supported (use the pipe-form `_group_join`). An `into` past the terminal is plain text in
    // the `select` projection, not a group-join.
    var groupIntoVar = ""
    let groupInto = intoPos >= 0 && intoPos < termPos
    if (groupInto) {
        if (isGroup) {
            macro_error(prog, at, "linq: 'group' after 'join … into' is not yet supported — 'select' the group (e.g. `select (key, g |> length)`), or use the pipe form")
            return "(0)"
        }
        if (orderByPos >= 0) {
            macro_error(prog, at, "linq: 'orderby' after 'join … into g' is not yet supported — the (outer, g) pair can't be carried (g is a non-copyable array); materialize then sort, or use the pipe form")
            return "(0)"
        }
        let iv = read_ident(q, intoPos + 4)
        if (iv._0 == "") {
            macro_error(prog, at, "linq: 'join … into' requires a group variable (e.g. `join b in B on … equals … into g select …`)")
            return "(0)"
        }
        if (iv._0 == JOIN_TI || iv._0 == varName || iv._0 == var2Name) {
            macro_error(prog, at, "linq: group variable '{iv._0}' must differ from '{varName}', '{var2Name}', and the reserved '{JOIN_TI}'")
            return "(0)"
        }
        if (strip(slice(q, iv._1, termPos)) != "") {
            macro_error(prog, at, "linq: a 'where' after 'join … into g' is not yet supported — the (outer, g) pair can't be carried; filter with a pre-'join' 'where', or use the pipe form")
            return "(0)"
        }
        groupIntoVar = iv._0
    }
    // Single equi-key only (join) — a second depth-0 `equals` before the terminal is a composite key.
    if (isJoin) {
        let equals2 = find_kw_depth0(q, "equals", tailStart)
        if (equals2 >= 0 && equals2 < termPos) {
            macro_error(prog, at, "linq: join 'on' supports a single equi-key only — composite keys (`a equals b && c equals d`) need raw SQL")
            return "(0)"
        }
    }
    // Collect every `where` and split by position: a `where` BEFORE the second source filters the left
    // source (single var `c`, no rewrite — pushes to SQL); a `where` AFTER the tail start filters the
    // carried pair (transparent-id rewrite). A `where` inside the join / second-`from` clause (between the
    // intro and the tail) is a grammar error. C# allows multiple in each slot; each runs to the next clause.
    var preWheres : array<string>
    var postWheres : array<string>
    var firstPostWhere = -1
    var wp = firstWhere
    while (wp >= 0 && wp < termPos) {
        let wEnd = next_clause_kw(q, wp + 5)
        let predText = strip(slice(q, wp + 5, wEnd))
        if (predText == "") {
            macro_error(prog, at, "linq: empty 'where' predicate")
            return "(0)"
        }
        if (wp < secondIntroPos) {
            preWheres |> push(predText)
        } elif (wp < tailStart) {
            if (isJoin) {
                macro_error(prog, at, "linq: 'where' cannot appear inside the join clause (between 'join' and 'equals')")
            } else {
                macro_error(prog, at, "linq: 'where' cannot appear between the second range variable and 'in'")
            }
            return "(0)"
        } else {
            if (firstPostWhere < 0) { firstPostWhere = wp }
            postWheres |> push(predText)
        }
        wp = find_kw_depth0(q, "where", wEnd)
    }
    let hasPostWhere = !empty(postWheres)
    if (groupInto && hasPostWhere) {
        macro_error(prog, at, "linq: a 'where' after 'join … into g' is not yet supported — the (outer, g) pair can't be carried; filter with a pre-'join' 'where', or use the pipe form")
        return "(0)"
    }
    // The tail (key B / second source expr) ends at the first post clause: orderby, a post-where, or term.
    var tailEnd = termPos
    if (orderByPos >= 0 && orderByPos < tailEnd) { tailEnd = orderByPos }
    if (firstPostWhere >= 0 && firstPostWhere < tailEnd) { tailEnd = firstPostWhere }
    if (groupInto && intoPos < tailEnd) { tailEnd = intoPos }   // join key B ends at `into`, before the group var
    let tail = strip(slice(q, tailStart, tailEnd))
    var srcB = srcBForJoin
    var onLambda = ""
    var correlated = false
    if (isJoin) {
        if (keyA == "" || tail == "") {
            macro_error(prog, at, "linq: empty join key — expected `on <keyA> equals <keyB>`")
            return "(0)"
        }
        onLambda = "$({varName}, {var2Name}) => {keyA} == {tail}"
    } else {
        if (tail == "") {
            macro_error(prog, at, "linq: empty source expression after the second 'in'")
            return "(0)"
        }
        // A CORRELATED second source (references the first range variable) is SelectMany flatten, not a
        // cross product: the second source is a per-row collection (`from {var2Name} in {varName}.items`).
        // Its collection-selector `srcB` is built below — after the `where` is known, so an inner-only
        // post-`from` `where` can be pushed into the collection (filter before flattening). The inner
        // element type is inferred (a `: RowB` annotation on a correlated second from is ignored).
        correlated = mentions_ident(tail, varName)
        if (!correlated) {
            srcB = build_src(rowType2, tail, false)
        }
    }
    var orderKeys : array<string>
    var orderDescMask = 0u
    if (orderByPos >= 0) {
        // End at the next clause keyword (a `where` may follow the `orderby`), not the terminal.
        let rawOrder = strip(slice(q, orderByPos + 7, next_clause_kw(q, orderByPos + 7)))
        var op <- parse_orderby(rawOrder)
        if (!op.ok) {
            macro_error(prog, at, "linq: empty 'orderby' expression")
            return "(0)"
        }
        if (length(op.keys) > 4) {
            macro_error(prog, at, "linq: 'orderby' supports at most 4 keys (got {length(op.keys)})")
            return "(0)"
        }
        orderKeys <- op.keys
        orderDescMask = op.descMask
    }
    var projText = ""
    var groupKey = ""
    var wantIterator = false
    if (isGroup) {
        let groupBody = strip(slice(q, termPos + 5, nq))
        let byPos = find_kw_depth0(groupBody, "by", 0)
        if (byPos < 0) {
            macro_error(prog, at, "linq: 'group' requires a 'by' clause (e.g. `group {varName} by {varName}.field`)")
            return "(0)"
        }
        let elem = strip(slice(groupBody, 0, byPos))
        if (elem != varName) {
            macro_error(prog, at, "linq: 'group' element must be the range variable '{varName}' — element selectors are not yet supported (write `group {varName} by …`)")
            return "(0)"
        }
        let gk = strip_trailing_keyword(strip(slice(groupBody, byPos + 2, length(groupBody))), "iterator")
        groupKey = gk._0
        wantIterator = gk._1
        if (groupKey == "") {
            macro_error(prog, at, "linq: empty 'group … by' key expression")
            return "(0)"
        }
    } else {
        let pj = strip_trailing_keyword(strip(slice(q, termPos + 6, nq)), "iterator")
        projText = pj._0
        wantIterator = pj._1
        if (projText == "") {
            macro_error(prog, at, "linq: empty 'select' projection")
            return "(0)"
        }
    }
    // Correlated SelectMany: the outer row is non-copyable (it holds the very collection being flattened),
    // so the transparent-identifier carry can't hold it. Build the collection selector here (deferred from
    // the source-shape branch so an inner-only post-`where` can be pushed into it). `orderby` / `group` over
    // the flattened pairs need the carry → rejected; an inner-only post-`where` is pushed into the collection
    // (filter before flattening — identical semantics); an outer-referencing post-`where` would need the carry.
    if (correlated) {
        if (orderByPos >= 0) {
            macro_error(prog, at, "linq: 'orderby' over a correlated 'from … from' (flatten) is not supported — the outer row '{varName}' is non-copyable, so the ordered pairs can't be carried; materialize the result then sort, or restructure as a 'join'")
            return "(0)"
        }
        if (isGroup) {
            macro_error(prog, at, "linq: 'group' over a correlated 'from … from' (flatten) is not supported — the outer row '{varName}' is non-copyable; 'select' the fields you need instead")
            return "(0)"
        }
        for (pred in postWheres) {
            if (mentions_ident(pred, varName)) {
                macro_error(prog, at, "linq: a post-'from' 'where' over a correlated 'from … from' may reference only the inner range variable '{var2Name}' (it is pushed into the collection before flattening) — referencing the outer '{varName}' would require carrying the non-copyable outer row; filter the outer with a pre-'from' 'where', or restructure")
                return "(0)"
            }
        }
        // Zero-copy collection selector: `each` borrows the outer's collection — no per-row clone (unlike
        // `to_sequence`, whose const-array overload clones) and no per-row materialize (unlike `where_`'s
        // array overload). Each inner-only post-`where` fuses onto the borrowed iterator (the iterator overload
        // of `where_` stays lazy). `each` outside a for-loop needs the `unsafe` wrap. Borrowing a TEMPORARY
        // tail (e.g. `from x in [c.id]` or a func result) is still safe: daslang heap-allocates arrays and
        // does NOT auto-finalize on scope exit, so the backing storage outlives the borrow (it is reclaimed
        // by GC, not at the end of the enclosing expression) — no use-after-free.
        srcB = build_string() $(var sw) {
            sw |> write("$({varName}) => unsafe(each({tail}))")
            for (pred in postWheres) {
                sw |> write(" |> _where($({var2Name}) => {pred})")
            }
        }
    }
    let carryLambda = "$({varName}, {var2Name}) => ({varName} = {varName}, {var2Name} = {var2Name})"
    let names <- [varName, var2Name]
    let hasOrderBy = orderByPos >= 0
    // Pre-source `where`s (single range var `c`) filter the left source — emitted before the op, no
    // rewrite; push down to SQL (AND-folded). Only a POST-source where/orderby/group forces the
    // transparent-id carry. Correlated never uses the carry (rejections + push-down above) → select-terminal.
    let useTI = !correlated && (hasPostWhere || hasOrderBy || isGroup)
    // group-join emits `_group_join` (result's 2nd param is the match ARRAY `g`, not a single row); the on-key
    // lambda is identical to `_join`. In-memory only — `_group_join` is not a SQL-pushable op (over a SQL source
    // `_sql` rejects it, the documented boundary).
    let opName = groupInto ? "_group_join" : (isJoin ? "_join" : (correlated ? "_select_many" : "_cross_join"))
    let onLambdaPart = isJoin ? "{onLambda}, " : ""
    return build_string() $(var w) {
        w |> write("( _fold( {srcA}")
        for (pred in preWheres) {
            w |> write(" |> _where($({varName}) => {pred})")
        }
        if (!useTI) {
            // Select-terminal: the `select` projection IS the op's result selector (both range vars in scope —
            // for a group-join the 2nd is the match array `g`). Underscore-prefix a range var the projection
            // doesn't reference (e.g. a scalar `select c.name` over a cross leaves `b` unused) so the generated
            // lambda is LINT013-clean for the user.
            let rv2 = groupInto ? groupIntoVar : var2Name
            let p1 = mentions_ident(projText, varName) ? varName : "_{varName}"
            let p2 = mentions_ident(projText, rv2) ? rv2 : "_{rv2}"
            w |> write(" |> {opName}({srcB}, {onLambdaPart}$({p1}, {p2}) => {projText})")
        } else {
            // Transparent-id: carry (c, d); post-source clauses rewrite c/d to the carried fields and
            // stream/sort/group the pairs directly (single stage — no pre-materialize).
            w |> write(" |> {opName}({srcB}, {onLambdaPart}{carryLambda})")
            for (pred in postWheres) {
                w |> write(" |> _where($({JOIN_TI}) => {rewrite_idents(pred, names, JOIN_TI)})")
            }
            if (hasOrderBy) {
                let rkeys <- [for (k in orderKeys); rewrite_idents(k, names, JOIN_TI)]
                w |> write(emit_order_op(rkeys, orderDescMask, JOIN_TI))
            }
            if (isGroup) {
                w |> write(" |> _group_by_lazy($({JOIN_TI}) => {rewrite_idents(groupKey, names, JOIN_TI)})")
            } else {
                w |> write(" |> _select($({JOIN_TI}) => {rewrite_idents(projText, names, JOIN_TI)})")
            }
        }
        w |> write(" |> to_array() )")
        if (wantIterator) { w |> write(" |> to_sequence()") }
        w |> write(" )")
    }
}

// `join <d> [: RowB] in <B> on <keyA> equals <keyB>` — single inner equi-join. Parses the join intro and
// the left source (a pre-join `where` filters it, single var, pushes to SQL), then delegates the
// post-source clauses + emit to emit_two_source. See it for the two emit shapes.
def private transpile_join(q : string; nq : int; varName : string; rowType : string;
                           srcStart : int; joinPos : int; prog : ProgramPtr; at : LineInfo) : string {
    if (find_kw_depth0(q, "join", joinPos + 4) >= 0) {
        macro_error(prog, at, "linq: only one 'join' per query is supported (nested joins are not yet supported)")
        return "(0)"
    }
    let v2 = read_ident(q, joinPos + 4)
    let var2Name = v2._0
    if (var2Name == "") {
        macro_error(prog, at, "linq: expected a range variable after 'join'")
        return "(0)"
    }
    let in2 = find_kw_depth0(q, "in", v2._1)
    if (in2 < 0) {
        macro_error(prog, at, "linq: expected 'in' after the join range variable '{var2Name}'")
        return "(0)"
    }
    let between2 = strip(slice(q, v2._1, in2))
    var rowType2 = ""
    if (starts_with(between2, ":")) {
        rowType2 = strip(slice(between2, 1, length(between2)))
        if (rowType2 == "") {
            macro_error(prog, at, "linq: expected a row type after ':' in the join clause (e.g. `join d : Dealer in db`)")
            return "(0)"
        }
    } elif (between2 != "") {
        macro_error(prog, at, "linq: unexpected '{between2}' between the join range variable and 'in'")
        return "(0)"
    }
    let onPos = find_kw_depth0(q, "on", in2 + 2)
    if (onPos < 0) {
        macro_error(prog, at, "linq: join requires an 'on <keyA> equals <keyB>' clause")
        return "(0)"
    }
    let src2Text = strip(slice(q, in2 + 2, onPos))
    if (src2Text == "") {
        macro_error(prog, at, "linq: empty join source after 'in'")
        return "(0)"
    }
    let equalsPos = find_kw_depth0(q, "equals", onPos + 2)
    if (equalsPos < 0) {
        macro_error(prog, at, "linq: join 'on' requires 'equals' (e.g. `on c.id equals d.cid`)")
        return "(0)"
    }
    let keyA = strip(slice(q, onPos + 2, equalsPos))
    let firstWhere = find_kw_depth0(q, "where", srcStart)
    let preWhere = firstWhere >= 0 && firstWhere < joinPos
    let srcAEnd = preWhere ? firstWhere : joinPos
    let srcAText = strip(slice(q, srcStart, srcAEnd))
    if (srcAText == "") {
        macro_error(prog, at, "linq: empty source expression after 'in'")
        return "(0)"
    }
    let srcA = build_src(rowType, srcAText, true)
    let srcB = build_src(rowType2, src2Text, true)
    return emit_two_source(q, nq, varName, var2Name, srcA, true, keyA, srcB, rowType2,
                           joinPos, equalsPos + 6, firstWhere, prog, at)
}

// Multiple-`from` (SelectMany):
//   from <c> [: RowA] in <A> [where <pred>] from <d> [: RowB] in <B>
//      [where <pred>] [orderby <key> [descending]] ( select <proj> | group <c> by <key> ) [iterator]
// Two flavors, decided in emit_two_source by whether `<B>` references `<c>`:
//   - uncorrelated (`<B>` independent of `<c>`): cross product → `_cross_join` (pushes down to SQL CROSS JOIN);
//   - correlated (`from <d> in <c>.items`): flatten → `_select_many` (in-memory; rejected over SQL).
// Parses the second-from intro and the left source (a pre-`from` `where` filters it), then delegates the
// post-source clauses + dual emit to emit_two_source.
def private transpile_cross(q : string; nq : int; varName : string; rowType : string;
                            srcStart : int; secondFromPos : int; prog : ProgramPtr; at : LineInfo) : string {
    if (find_kw_depth0(q, "from", secondFromPos + 4) >= 0) {
        macro_error(prog, at, "linq: only two 'from' clauses are supported (N-ary SelectMany is not yet supported)")
        return "(0)"
    }
    let v2 = read_ident(q, secondFromPos + 4)
    let var2Name = v2._0
    if (var2Name == "") {
        macro_error(prog, at, "linq: expected a range variable after the second 'from'")
        return "(0)"
    }
    let in2 = find_kw_depth0(q, "in", v2._1)
    if (in2 < 0) {
        macro_error(prog, at, "linq: expected 'in' after the second range variable '{var2Name}'")
        return "(0)"
    }
    let between2 = strip(slice(q, v2._1, in2))
    var rowType2 = ""
    if (starts_with(between2, ":")) {
        rowType2 = strip(slice(between2, 1, length(between2)))
        if (rowType2 == "") {
            macro_error(prog, at, "linq: expected a row type after ':' in the second from clause (e.g. `from d : Dealer in db`)")
            return "(0)"
        }
    } elif (between2 != "") {
        macro_error(prog, at, "linq: unexpected '{between2}' between the second range variable and 'in'")
        return "(0)"
    }
    let firstWhere = find_kw_depth0(q, "where", srcStart)
    let preWhere = firstWhere >= 0 && firstWhere < secondFromPos
    let srcAEnd = preWhere ? firstWhere : secondFromPos
    let srcAText = strip(slice(q, srcStart, srcAEnd))
    if (srcAText == "") {
        macro_error(prog, at, "linq: empty source expression after the first 'in'")
        return "(0)"
    }
    let srcA = build_src(rowType, srcAText, false)
    // srcB is built inside emit_two_source from the tail (after the correlation check).
    return emit_two_source(q, nq, varName, var2Name, srcA, false, "", "", rowType2,
                           secondFromPos, in2 + 2, firstWhere, prog, at)
}

// Offset of the next depth-0 query-body clause keyword at/after `from`, or `length(q)` if none. The set is
// every clause that can follow a `let` (and so bounds a `let` RHS) or begin the body after the source.
def private next_clause_kw(q : string; from : int) : int {
    var best = length(q)
    for (kw in ["let", "where", "orderby", "select", "group", "join", "from", "into"]) {
        let p = find_kw_depth0(q, kw, from)
        if (p >= 0 && p < best) { best = p }
    }
    return best
}

// Extract `let` bindings (Approach A — textual inlining). `let n = <expr>` introduces a computed range
// variable; every downstream reference is replaced with `(<expr>)`, yielding a `let`-free query the rest of
// the reader handles unchanged — in every context (single / join / second-`from`), and inlined computed
// columns/keys push down to SQL. Chained lets resolve in order (a later RHS inlines the earlier bindings).
// `bodyStart` is the offset just past the source's `in` (lets only follow the source). A binding inlines
// only into the clauses that FOLLOW it (C# forward-only `let` scoping): the source expression and any text
// before a binding are left untouched, so an identifier that legitimately precedes a same-named binding
// (an outer/global) is not clobbered. Returns (ok, rewritten-query); a malformed binding reports a
// macro_error and returns ok=false.
def private extract_lets(q : string; bodyStart : int; rangeVar : string;
                         prog : ProgramPtr; at : LineInfo) : tuple<bool; string> {
    var subs : table<string; string>        // binding name → fully-resolved "(<expr>)"
    var cuts : array<tuple<int; int; string>>   // (start, end, name) of each `let` clause, in order
    let srcEnd = next_clause_kw(q, bodyStart)   // first clause keyword = end of the source expression
    // A second range variable (after `join` or a second `from`) is also off-limits for a `let` name —
    // reusing it would shadow it (a `let` after the clause) or rewrite its declaration (a `let` before it).
    let joinPos = find_kw_depth0(q, "join", bodyStart)
    let secondFromPos = find_kw_depth0(q, "from", bodyStart)
    let var2j = joinPos >= 0 ? read_ident(q, joinPos + 4)._0 : ""
    let var2f = secondFromPos >= 0 ? read_ident(q, secondFromPos + 4)._0 : ""
    var searchFrom = bodyStart
    while (true) {
        let lp = find_kw_depth0(q, "let", searchFrom)
        if (lp < 0) break
        let np = read_ident(q, lp + 3)
        let name = np._0
        if (name == "") {
            macro_error(prog, at, "linq: 'let' requires a binding name (e.g. `let n = <expr>`)")
            return (false, "")
        }
        if (name == rangeVar || name == var2j || name == var2f) {
            macro_error(prog, at, "linq: 'let {name}' shadows the range variable '{name}'")
            return (false, "")
        }
        if (key_exists(subs, name)) {
            macro_error(prog, at, "linq: 'let {name}' is already bound earlier in the query")
            return (false, "")
        }
        // Expect a single '=' (not '==') after the binding name.
        var eq = -1
        peek_data(q) $(arr) {
            let n2 = length(arr)
            var i = np._1
            while (i < n2 && is_white_space(int(arr[i]))) { i++ }
            if (i < n2 && int(arr[i]) == '=' && (i + 1 >= n2 || int(arr[i + 1]) != '=')) { eq = i }
        }
        if (eq < 0) {
            macro_error(prog, at, "linq: expected '=' after 'let {name}'")
            return (false, "")
        }
        let rhsStart = eq + 1
        let rhsEnd = next_clause_kw(q, rhsStart)   // RHS runs to the next clause keyword (or end)
        var rhs = strip(slice(q, rhsStart, rhsEnd))
        if (rhs == "") {
            macro_error(prog, at, "linq: empty 'let {name}' expression")
            return (false, "")
        }
        // A `let` with no following clause has nothing to inline into — it would be silently dropped,
        // turning an invalid query (e.g. `… select … let n = …`) into a valid one. Reject it: a `let`
        // must precede the `select`/`group` terminal.
        if (rhsEnd >= length(q)) {
            macro_error(prog, at, "linq: 'let {name}' has no following clause — a 'let' must precede the 'select'/'group' terminal")
            return (false, "")
        }
        if (!empty(subs)) { rhs = substitute_idents(rhs, subs) }   // chain earlier lets in
        subs[name] = "({rhs})"
        cuts |> push((lp, rhsEnd, name))
        searchFrom = rhsEnd
    }
    if (empty(subs)) return (true, q)
    // Rebuild: source region (< srcEnd) verbatim; each `let` span dropped (replaced by a space); each clause
    // segment inlined with only the bindings introduced BEFORE it, so a reference preceding a binding stays.
    let body = build_string() $(var w) {
        var active : table<string; string>
        var pos = srcEnd
        for (c in cuts) {
            w |> write(substitute_idents(slice(q, pos, c._0), active))
            w |> write(" ")
            active[c._2] = subs[c._2]
            pos = c._1
        }
        w |> write(substitute_idents(slice(q, pos, length(q)), active))
    }
    return (true, "{slice(q, 0, srcEnd)}{body}")
}

// A2 rewrite for a `group … into g` continuation: the group is a `tuple<key; array<members>>`, so `g.key`
// addresses the key (`g._0`) and a bare `g` is the member collection (`g._1`). Scans like substitute_idents
// (string/interpolation aware, member access after `.` skipped), but specialised: a standalone `g` followed
// by `.key` (a full word) becomes `g._0` (consuming the `.key`); any other standalone `g` becomes `g._1`.
def private rewrite_group_var(text : string; gname : string) : string {
    return build_string() $(var w) {
        peek_data(text) $(arr) {
            let n = length(arr)
            var i = 0
            var inStr = false   // inside a "…" literal (plain content portion)
            var interp = 0      // brace depth inside a string interpolation (>0 ⇒ code mode within the string)
            var prevSig = 0     // last significant (non-whitespace) byte, 0 = none
            while (i < n) {
                let c = int(arr[i])
                if (inStr && interp == 0) {
                    if (c == '\\' && i + 1 < n) { w |> write(slice(text, i, i + 2)); i += 2; continue }
                    w |> write(slice(text, i, i + 1))
                    if (c == '"') { inStr = false; prevSig = c }
                    elif (c == '{') { interp = 1; prevSig = 0 }
                    i++
                    continue
                }
                if (c == '"') {
                    if (interp > 0) {
                        w |> write(slice(text, i, i + 1)); i++
                        while (i < n) {
                            let cc = int(arr[i])
                            if (cc == '\\' && i + 1 < n) { w |> write(slice(text, i, i + 2)); i += 2; continue }
                            w |> write(slice(text, i, i + 1)); i++
                            if (cc == '"') break
                        }
                        prevSig = '"'
                        continue
                    }
                    inStr = true; prevSig = c; w |> write(slice(text, i, i + 1)); i++
                    continue
                }
                if (interp > 0 && c == '{') { interp ++; w |> write(slice(text, i, i + 1)); prevSig = c; i++; continue }
                if (interp > 0 && c == '}') {
                    interp --
                    w |> write(slice(text, i, i + 1))
                    prevSig = interp == 0 ? '"' : c
                    i++
                    continue
                }
                let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1])))
                if (atWordStart) {
                    var j = i
                    while (j < n && is_ident_byte(int(arr[j]))) { j++ }
                    let word = slice(text, i, j)
                    if (prevSig != '.' && word == gname) {
                        // `g.key` → `g._0`; truly bare `g` → `g._1` (the members array). Any other `g.<member>`
                        // (explicit `g._0` / `g._1`, or an invalid `g.foo`) is left untouched so raw tuple access
                        // keeps working and invalid members error naturally — never rewrite `g._0` into `g._1._0`.
                        let dotFollows = j < n && int(arr[j]) == '.'
                        var keyEnd = -1
                        if (dotFollows) {
                            var k = j + 1
                            while (k < n && is_ident_byte(int(arr[k]))) { k++ }
                            if (slice(text, j + 1, k) == "key") { keyEnd = k }
                        }
                        if (keyEnd >= 0) {
                            w |> write("{gname}._0")
                            i = keyEnd
                            prevSig = '_'   // last emitted token is identifier-like, not a `.`
                            continue
                        } elif (!dotFollows) {
                            w |> write("{gname}._1")
                            i = j
                            prevSig = '_'
                            continue
                        }
                        // g.<member> other than .key: fall through, leave the explicit access intact.
                    }
                    w |> write(word)
                    prevSig = int(arr[j - 1])
                    i = j
                    continue
                }
                w |> write(slice(text, i, i + 1))
                if (!is_white_space(c)) { prevSig = c }
                i++
            }
        }
    }
}

// One query stage: `[where] [orderby] (select <proj> | group <var> by <key>) [iterator]`, optionally followed
// by `into <var>` which begins the next stage (C# query continuation). `rangeVar` binds this stage's rows;
// `isGroupVar` is true when `rangeVar` came from a prior `group … into` (so its clause text gets the A2 rewrite).
struct private QueryStage {
    rangeVar : string = ""
    isGroupVar : bool = false
    predicates : array<string>   // every `where` before the terminal, in source order (C# allows multiple)
    hasOrderBy : bool = false
    orderKeys : array<string>
    orderDescMask : uint = 0u
    isGroup : bool = false
    groupKey : string = ""
    projText : string = ""
    identitySelect : bool = false
    wantIterator : bool = false
    intoVar : string = ""       // continuation range variable, "" if this is the terminal stage
    nextStart : int = 0         // offset just past `into <var>` (start of the next stage)
}

// Parse one stage of a single-source query starting at `start`, binding `rangeVar`. Fills `stage` (incl. the
// optional `into` continuation). Returns false (after a macro_error) on a malformed stage.
def private parse_one_stage(q : string; nq : int; start : int; rangeVar : string; isGroupVar : bool;
                            prog : ProgramPtr; at : LineInfo; var stage : QueryStage) : bool {
    stage.rangeVar = rangeVar
    stage.isGroupVar = isGroupVar
    // The terminal is the earliest select/group from `start` — found FIRST, independent of where/orderby, so
    // a LATER stage's `where` (after this stage's `into`) cannot hijack this stage's terminal lookup.
    let selectPos = find_kw_depth0(q, "select", start)
    let groupPos = find_kw_depth0(q, "group", start)
    if (selectPos < 0 && groupPos < 0) {
        macro_error(prog, at, "linq: query must end with a 'select' or 'group … by …' clause")
        return false
    }
    let isGroup = groupPos >= 0 && (selectPos < 0 || groupPos < selectPos)
    let termPos = isGroup ? groupPos : selectPos
    let otherPos = isGroup ? selectPos : groupPos
    let termKwEnd = isGroup ? termPos + 5 : termPos + 6
    // An `into` after the terminal ends this stage and begins the next.
    let intoPos = find_kw_depth0(q, "into", termKwEnd)
    let termEnd = intoPos >= 0 ? intoPos : nq
    if (otherPos >= 0 && otherPos < termEnd) {
        macro_error(prog, at, "linq: query cannot have both 'select' and 'group' — they are alternative terminals")
        return false
    }
    stage.isGroup = isGroup
    // where / orderby belong to THIS stage only when they precede its terminal (in C# clause order).
    // Collect every depth-0 `where` before the terminal — C# allows multiple, in any body position. A
    // `where` after the `orderby` filters the sorted sequence, which (for a total order) yields the same
    // result as filtering first, so all predicates emit as `_where` ahead of the order op; source order
    // among the wheres is preserved. Each predicate runs to the next clause keyword.
    var wherePos = find_kw_depth0(q, "where", start)
    while (wherePos >= 0 && wherePos < termPos) {
        let predEnd = next_clause_kw(q, wherePos + 5)
        let predText = strip(slice(q, wherePos + 5, predEnd))
        if (predText == "") {
            macro_error(prog, at, "linq: empty 'where' predicate")
            return false
        }
        stage.predicates |> push(isGroupVar ? rewrite_group_var(predText, rangeVar) : predText)
        wherePos = find_kw_depth0(q, "where", predEnd)
    }
    let orderByPos = find_kw_depth0(q, "orderby", start)
    stage.hasOrderBy = orderByPos >= 0 && orderByPos < termPos
    if (isGroup && stage.hasOrderBy) {
        macro_error(prog, at, "linq: 'orderby' before 'group' is not supported — order the groups after a 'select', or drop the 'orderby'")
        return false
    }
    if (stage.hasOrderBy) {
        // End at the next clause keyword (a `where` may follow the `orderby`), not the terminal.
        let rawOrder = strip(slice(q, orderByPos + 7, next_clause_kw(q, orderByPos + 7)))
        var op <- parse_orderby(rawOrder)
        if (!op.ok) {
            macro_error(prog, at, "linq: empty 'orderby' expression")
            return false
        }
        if (length(op.keys) > 4) {
            macro_error(prog, at, "linq: 'orderby' supports at most 4 keys (got {length(op.keys)})")
            return false
        }
        if (isGroupVar) {
            stage.orderKeys <- [for (k in op.keys); rewrite_group_var(k, rangeVar)]
        } else {
            stage.orderKeys <- op.keys
        }
        stage.orderDescMask = op.descMask
    }
    if (isGroup) {
        let groupBody = strip(slice(q, termPos + 5, termEnd))
        let byPos = find_kw_depth0(groupBody, "by", 0)
        if (byPos < 0) {
            macro_error(prog, at, "linq: 'group' requires a 'by' clause (e.g. `group {rangeVar} by {rangeVar}.field`)")
            return false
        }
        let elem = strip(slice(groupBody, 0, byPos))
        if (elem != rangeVar) {
            macro_error(prog, at, "linq: 'group' element must be the range variable '{rangeVar}' — element selectors are not yet supported (write `group {rangeVar} by …`)")
            return false
        }
        let gk = strip_trailing_keyword(strip(slice(groupBody, byPos + 2, length(groupBody))), "iterator")
        let groupKey = gk._0
        stage.wantIterator = gk._1
        if (groupKey == "") {
            macro_error(prog, at, "linq: empty 'group … by' key expression")
            return false
        }
        stage.groupKey = isGroupVar ? rewrite_group_var(groupKey, rangeVar) : groupKey
    } else {
        let pj = strip_trailing_keyword(strip(slice(q, termPos + 6, termEnd)), "iterator")
        let projText = pj._0
        stage.wantIterator = pj._1
        if (projText == "") {
            macro_error(prog, at, "linq: empty 'select' projection")
            return false
        }
        stage.identitySelect = (projText == rangeVar)
        stage.projText = (isGroupVar && !stage.identitySelect) ? rewrite_group_var(projText, rangeVar) : projText
    }
    if (intoPos >= 0) {
        if (stage.wantIterator) {
            macro_error(prog, at, "linq: 'iterator' must be the final clause — it cannot precede an 'into' continuation")
            return false
        }
        let iv = read_ident(q, intoPos + 4)
        if (iv._0 == "") {
            macro_error(prog, at, "linq: 'into' requires a continuation range variable (e.g. `… into g select …`)")
            return false
        }
        if (iv._0 == JOIN_TI) {
            macro_error(prog, at, "linq: continuation range variable '{JOIN_TI}' is reserved")
            return false
        }
        if (strip(slice(q, iv._1, nq)) == "") {
            macro_error(prog, at, "linq: 'into {iv._0}' has no continuation — add a 'where'/'orderby'/'select'/'group' after it")
            return false
        }
        stage.intoVar = iv._0
        stage.nextStart = iv._1
    } else {
        stage.intoVar = ""
        stage.nextStart = nq
    }
    return true
}

// Parse the query and emit the `_fold(...)` rewrite text. On a malformed query, register a macro error
// and emit a benign `(0)` so the surrounding statement still parses (the error is the real signal).
def private transpile_query(query : string; prog : ProgramPtr; at : LineInfo) : string {
    var q = strip(strip_query_comments(query))
    let fromWord = read_ident(q, 0)
    if (fromWord._0 != "from") {
        macro_error(prog, at, "linq: query must start with the 'from' keyword (got: {q})")
        return "(0)"
    }
    let vp = read_ident(q, fromWord._1)
    let varName = vp._0
    let afterVar = vp._1
    if (varName == "") {
        macro_error(prog, at, "linq: expected a range variable after 'from'")
        return "(0)"
    }
    let inPos = find_kw_depth0(q, "in", afterVar)
    if (inPos < 0) {
        macro_error(prog, at, "linq: expected 'in' after range variable '{varName}'")
        return "(0)"
    }
    // Optional row-type annotation `: Row` between the range variable and `in` (selects a typed source).
    let between = strip(slice(q, afterVar, inPos))
    var rowType = ""
    if (starts_with(between, ":")) {
        rowType = strip(slice(between, 1, length(between)))
        if (rowType == "") {
            macro_error(prog, at, "linq: expected a row type after ':' (e.g. `from c : Car in db`)")
            return "(0)"
        }
    } elif (between != "") {
        macro_error(prog, at, "linq: unexpected '{between}' between range variable and 'in'")
        return "(0)"
    }
    // Clauses in C# order, each searched after the previous so positions stay ordered:
    //   from … [let <n> = <expr>]* [where <pred>] [orderby <expr> [descending]] ( select <proj> | group <var> by <key> ) [iterator]
    let srcStart = inPos + 2
    // `let` bindings are inlined away first, so the rest of the reader sees a `let`-free query in every
    // context (the substitution rewrites the whole body, including a join/second-`from` tail).
    let lets = extract_lets(q, srcStart, varName, prog, at)
    if (!lets._0) return "(0)"
    q = lets._1
    let nq = length(q)
    // A `join` or a second `from` introduces a second range variable — each has its own grammar and emit
    // (transpile_join / transpile_cross). Combining the two in one query is not yet supported.
    let joinPos = find_kw_depth0(q, "join", srcStart)
    let secondFromPos = find_kw_depth0(q, "from", srcStart)
    if (joinPos >= 0 && secondFromPos >= 0) {
        macro_error(prog, at, "linq: combining 'join' with a second 'from' (SelectMany) in one query is not yet supported")
        return "(0)"
    }
    if (secondFromPos >= 0)
        return transpile_cross(q, nq, varName, rowType, srcStart, secondFromPos, prog, at)
    if (joinPos >= 0)
        return transpile_join(q, nq, varName, rowType, srcStart, joinPos, prog, at)
    let srcEnd = next_clause_kw(q, srcStart)
    let srcText = strip(slice(q, srcStart, srcEnd))
    if (srcText == "") {
        macro_error(prog, at, "linq: empty source expression after 'in'")
        return "(0)"
    }
    let srcBuild = build_src(rowType, srcText, true)
    // Parse the stage chain. Each stage ends in select/group; an `into <var>` continuation rebinds a new
    // range variable to that stage's output and appends more clauses on the SAME `_fold` chain (no
    // inter-stage materialization — fusion and the SQL single-statement pushdown are preserved).
    var stages : array<QueryStage>
    var curVar = varName
    var curIsGroup = false
    var pstart = srcEnd
    while (true) {
        var stage <- QueryStage()
        if (!parse_one_stage(q, nq, pstart, curVar, curIsGroup, prog, at, stage)) return "(0)"
        let contVar = stage.intoVar
        let contStart = stage.nextStart
        let contIsGroup = stage.isGroup
        stages |> emplace(stage)
        if (contVar == "") break
        curVar = contVar
        curIsGroup = contIsGroup
        pstart = contStart
    }
    let wantIterator = stages[length(stages) - 1].wantIterator
    return build_string() $(var writer) {
        writer |> write("( _fold( {srcBuild}")
        for (s in stages) {
            for (pred in s.predicates) {
                writer |> write(" |> _where($({s.rangeVar}) => {pred})")
            }
            if (s.isGroup) {
                writer |> write(" |> _group_by_lazy($({s.rangeVar}) => {s.groupKey})")
            } else {
                if (s.hasOrderBy) {
                    writer |> write(emit_order_op(s.orderKeys, s.orderDescMask, s.rangeVar))
                }
                if (!s.identitySelect) {
                    writer |> write(" |> _select($({s.rangeVar}) => {s.projText})")
                }
            }
        }
        writer |> write(" |> to_array() )")
        if (wantIterator) {
            writer |> write(" |> to_sequence()")
        }
        writer |> write(" )")
    }
}

[call_macro(name="from_in")]
class FromInMacro : AstCallMacro {
    //! Typed-source dispatcher for the C# query form `from c : Row in <src>`. Rewrites
    //! `from_in(src, type<Row>)` to the concrete `_fold` source builder by `src`'s type — a SQL runner
    //! → `select_from`, an XML node → `from_xml_node`, a JSON value → `from_json`. Must be a call macro:
    //! a plain function would leave `from_in(...)` at the chain head, which `_fold`'s name-based source
    //! detection cannot route.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        if (length(call.arguments) != 2) {
            macro_error(prog, call.at, "from_in(src, type<Row>): expected 2 arguments")
            return call
        }
        let srcT = call.arguments[0]._type
        if (srcT == null) {
            macro_error(prog, call.at, "from_in: source type is not inferred")
            return call
        }
        // SQL: db is a sqlite_boost::SqlRunner → select_from(db, type<Row>)
        if (srcT.structType != null && srcT.structType.name == "SqlRunner"
                && srcT.structType._module != null && srcT.structType._module.name == "sqlite_boost")
            return qmacro($c("select_from")($e(call.arguments[0]), $e(call.arguments[1])))
        // XML: node is a pugixml `xml_node` (handled type) → unsafe(from_xml_node(node, type<Row>)).
        // from_xml_node is [unsafe_outside_of_for]; the unsafe wrap is required in a `_fold` chain.
        if (srcT.isHandle && srcT.annotation != null && srcT.annotation.name == "xml_node")
            return qmacro(unsafe($c("from_xml_node")($e(call.arguments[0]), $e(call.arguments[1]))))
        // JSON: src is a json `JsonValue?` (pointer to json::JsonValue, a JSON array) → unsafe(from_json(jv, type<Row>)).
        // from_json is [unsafe_outside_of_for]; the unsafe wrap is required in a `_fold` chain. String-matched
        // on the type so linq_das pulls in no json dependency.
        if (srcT.isPointer && srcT.firstType != null && srcT.firstType.structType != null
                && srcT.firstType.structType.name == "JsonValue"
                && srcT.firstType.structType._module != null && srcT.firstType.structType._module.name == "json")
            return qmacro(unsafe($c("from_json")($e(call.arguments[0]), $e(call.arguments[1]))))
        macro_error(prog, call.at, "linq: unsupported source for `from c : Row in <src>` — expected a SQL runner, an XML node, or a JSON value (use `in decs` for decs, or an array for the untyped `from c in <arr>` form)")
        return call
    }
}

[reader_macro(name=linq)]
class LinqDasReader : AstReaderMacro {
    //! C#-style LINQ query reader macro.
    //! ``%linq! from c [: Row] in src [where <pred>] [ join d [: RowB] in B on <kA> equals <kB> | from d [: RowB] in B ] [orderby <expr> [descending]] ( select <proj> | group c by <key> ) [iterator] %%``
    //! rewrites to a ``_fold(...)`` chain. Sources: array (untyped `from c in arr`), or a typed
    //! ``from c : Row in src`` over decs / SQL / XML / JSON. ``orderby`` is a single sort key; ``group c by <key>``
    //! is a terminal yielding ``tuple<key; array<elem>>`` buckets (in-memory sources only). A second range
    //! variable comes from either a ``join`` (single inner equi-join) or a second ``from`` (SelectMany):
    //! uncorrelated (``from d in B``) is the cross product, emitting ``_cross_join`` (pushes down to a SQL
    //! CROSS JOIN); correlated (``from d in c.items``) is the flatten, emitting ``_select_many`` (in-memory;
    //! rejected over SQL). A select-terminal projects the joined/crossed result row
    //! (scalar or named-tuple pushes down to SQL; whole-row ``select c`` is in-memory only), while a
    //! post-source ``where``/``orderby`` or a ``group`` terminal carries the ``(c, d)`` pair (in-memory). A
    //! trailing ``iterator`` keyword yields an ``iterator<T>`` over the materialized result instead of the
    //! ``array<T>``.
    def override accept(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; ch : int; info : LineInfo) : bool {
        append(expr.sequence, ch)
        if (ends_with(expr.sequence, "%%")) {
            resize(expr.sequence, length(expr.sequence) - 2)
            return false
        }
        return true
    }
    def override suffix(prog : ProgramPtr; mod : Module?; var expr : ExprReader?; info : LineInfo;
                        var outLine : int&; var outFile : FileInfo?&) : string {
        return transpile_query(string(expr.sequence), prog, info)
    }
}
