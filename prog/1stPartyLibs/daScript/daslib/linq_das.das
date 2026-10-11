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
//        →   ( _fold( from_in(cars) |> _where($(c) => c.price > 100) |> _order_by($(c) => c.price) |> _select($(c) => c.name) |> to_array() ) )
//     %linq! from c in cars group c by c.brand %%
//        →   ( _fold( from_in(cars) |> _group_by_lazy($(c) => c.brand) |> to_array() ) )
//     %linq! from c in cars join d in dealers on c.brand equals d.brand select (N = c.name, City = d.city) %%
//        →   ( _fold( from_in(cars) |> _join(from_in(dealers), $(c, d) => c.brand == d.brand, $(c, d) => (N = c.name, City = d.city)) |> to_array() ) )
//     %linq! from c in cars from d in dealers select (N = c.name, City = d.city) %%
//        →   ( _fold( (cars) |> _cross_join((dealers), $(c, d) => (N = c.name, City = d.city)) |> to_array() ) )
//
// (`from_in(arr)` resolves during inference to `each(arr)` — see FromInMacro at the bottom.)
//
// An untyped `from c in <src>` is an array or table source — the 1-arg `from_in` call macro dispatches
// by the value type (array → `each`, `table<K;V>` → `each_kv` with `kv.key`/`kv.value`, `table<K>` set →
// `keys`). A typed `from c : Row in <src>` selects a row-typed source: `decs` (a keyword marker →
// `from_decs_template`), or any value whose type the 2-arg `from_in` dispatches (a SQL runner →
// `select_from`, an XML node → `from_xml_node`). The range variable is
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

def private find_kw_depth0(q : string; kw : string; from : int) : int {
    var found = -1
    let m = length(kw)
    peek_data(q) $(arr) {
        let n = length(arr)
        var depth = 0
        var inStr = false
        var lastNonWs = 0
        var nonWsBeforeLast = 0
        var i = from
        while (i < n && found < 0) {
            let c = int(arr[i])
            if (inStr) {
                if (c == '\\') { i += 2; continue }
                if (c == '"') { inStr = false; nonWsBeforeLast = lastNonWs; lastNonWs = c }
                i++
                continue
            }
            if (c == '"') { inStr = true; nonWsBeforeLast = lastNonWs; lastNonWs = c; i++; continue }
            if (c == '(' || c == '[' || c == '{') { depth++; nonWsBeforeLast = lastNonWs; lastNonWs = c; i++; continue }
            if (c == ')' || c == ']' || c == '}') { depth--; nonWsBeforeLast = lastNonWs; lastNonWs = c; i++; continue }
            let afterPipeTail = lastNonWs == '>' && (nonWsBeforeLast == '|' || nonWsBeforeLast == '=' || nonWsBeforeLast == '-')
            let atWordStart = is_ident_byte(c) && ((i == 0) || !is_ident_byte(int(arr[i - 1]))) && lastNonWs != '.' && !afterPipeTail
            if (depth == 0 && atWordStart && i + m <= n && starts_with(arr, i, kw)) {
                if (i + m == n || !is_ident_byte(int(arr[i + m]))) {
                    found = i
                }
            }
            if (!is_white_space(c)) { nonWsBeforeLast = lastNonWs; lastNonWs = c }
            i++
        }
    }
    return found
}

def private split_depth0(text : string) : array<string> {
    var segs : array<string>
    peek_data(text) $(arr) {
        let n = length(arr)
        segs |> reserve(4)
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
                segs |> push(strip(slice(arr, segStart, i)))
                segStart = i + 1
            }
            i++
        }
        segs |> push(strip(slice(arr, segStart, n)))
    }
    return <- segs
}

struct private OrderKeys {
    keys     : array<string>
    descMask : uint
    ok       : bool
}

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
        if (dseg._1 && idx < 32u) { r.descMask = r.descMask | (1u << idx) }
        idx++
    }
    if (empty(r.keys)) { r.ok = false }
    return <- r
}

def private emit_order_op(keys : array<string>; descMask : uint; ti : string) : string {
    if (length(keys) == 1) {
        let op = (descMask & 1u) != 0u ? "_order_by_descending" : "_order_by"
        return " |> {op}($({ti}) => {keys[0]})"
    }
    return " |> _order_by_keys($({ti}) => ({join(keys, ", ")}), {descMask}u)"
}

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

def private build_src(rowType : string; srcText : string; fused : bool) : string {
    if (rowType == "") return fused ? "from_in({srcText})" : "({srcText})"
    if (srcText == "decs") return "from_decs_template(type<{rowType}>)"
    return "from_in({srcText}, type<{rowType}>)"
}

def private strip_redundant_parens(s : string) : string {
    let trimmed = strip(s)
    var out = trimmed
    peek_data(trimmed) $(arr) {
        var lo = 0
        var hi = length(arr)
        var again = true
        while (again) {
            again = false
            if (hi - lo >= 2 && int(arr[lo]) == '(' && int(arr[hi - 1]) == ')') {
                var depth = 0
                var matched = true
                for (i in lo .. hi) {
                    let c = int(arr[i])
                    if (c == '(') {
                        depth ++
                    } elif (c == ')') {
                        depth --
                        if (depth == 0 && i != hi - 1) {
                            matched = false
                            break
                        }
                    }
                }
                if (matched) {
                    lo ++
                    hi --
                    while (lo < hi && is_white_space(int(arr[lo]))) {
                        lo ++
                    }
                    while (hi > lo && is_white_space(int(arr[hi - 1]))) {
                        hi --
                    }
                    again = true
                }
            }
        }
        out = slice(arr, lo, hi)
    }
    return out
}

def private is_label_position(arr : auto; j, n : int) : bool {
    var k = j
    while (k < n && is_white_space(int(arr[k]))) {
        k++
    }
    return k < n && int(arr[k]) == '=' && (k + 1 >= n || (int(arr[k + 1]) != '=' && int(arr[k + 1]) != '>'))
}

def private substitute_idents(text : string; subs : table<string; string>) : string {
    return build_string() $(var w) {
        peek_data(text) $(arr) {
            let n = length(arr)
            var i = 0
            var inStr = false
            var interpDepth = 0
            var lastNonWs = 0
            while (i < n) {
                let c = int(arr[i])
                if (inStr && interpDepth == 0) {
                    if (c == '\\' && i + 1 < n) { w |> write_string(arr, i, i + 2); i += 2; continue }
                    w |> write_string(arr, i, i + 1)
                    if (c == '"') { inStr = false; lastNonWs = c }
                    elif (c == '{') { interpDepth = 1; lastNonWs = 0 }
                    i++
                    continue
                }
                if (c == '"') {
                    if (interpDepth > 0) {
                        w |> write_string(arr, i, i + 1); i++
                        while (i < n) {
                            let cc = int(arr[i])
                            if (cc == '\\' && i + 1 < n) { w |> write_string(arr, i, i + 2); i += 2; continue }
                            w |> write_string(arr, i, i + 1); i++
                            if (cc == '"') break
                        }
                        lastNonWs = '"'
                        continue
                    }
                    inStr = true; lastNonWs = c; w |> write_string(arr, i, i + 1); i++
                    continue
                }
                if (interpDepth > 0 && c == '{') { interpDepth ++; w |> write_string(arr, i, i + 1); lastNonWs = c; i++; continue }
                if (interpDepth > 0 && c == '}') {
                    interpDepth --
                    w |> write_string(arr, i, i + 1)
                    lastNonWs = interpDepth == 0 ? '"' : c
                    i++
                    continue
                }
                let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1])))
                if (atWordStart) {
                    var j = i
                    while (j < n && is_ident_byte(int(arr[j]))) { j++ }
                    let word = slice(arr, i, j)
                    let isLabel = is_label_position(arr, j, n)
                    w |> write((lastNonWs != '.' && !isLabel) ? (subs?[word] ?? word) : word)
                    lastNonWs = int(arr[j - 1])
                    i = j
                    continue
                }
                w |> write_string(arr, i, i + 1)
                if (!is_white_space(c)) { lastNonWs = c }
                i++
            }
        }
    }
}

def private rewrite_idents(text : string; names : array<string>; prefix : string) : string {
    let subs <- {for (nm in names); nm => "{prefix}.{nm}"}
    return substitute_idents(text, subs)
}

def private mentions_ident(text : string; name : string) : bool {
    var found = false
    let m = length(name)
    peek_data(text) $(arr) {
        let n = length(arr)
        var i = 0
        var inStr = false
        var interpDepth = 0
        var lastNonWs = 0
        while (i < n && !found) {
            let c = int(arr[i])
            if (inStr && interpDepth == 0) {
                if (c == '\\' && i + 1 < n) { i += 2; continue }
                if (c == '"') { inStr = false; lastNonWs = c }
                elif (c == '{') { interpDepth = 1; lastNonWs = 0 }
                i++
                continue
            }
            if (c == '"') {
                if (interpDepth > 0) {
                    i++
                    while (i < n) {
                        let cc = int(arr[i])
                        if (cc == '\\' && i + 1 < n) { i += 2; continue }
                        i++
                        if (cc == '"') break
                    }
                    lastNonWs = '"'
                    continue
                }
                inStr = true; lastNonWs = c; i++
                continue
            }
            if (interpDepth > 0 && c == '{') { interpDepth++; lastNonWs = c; i++; continue }
            if (interpDepth > 0 && c == '}') {
                interpDepth--
                lastNonWs = interpDepth == 0 ? '"' : c
                i++
                continue
            }
            let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1]))) && lastNonWs != '.'
            if (atWordStart && i + m <= n && starts_with(arr, i, name)
                    && (i + m == n || !is_ident_byte(int(arr[i + m])))
                    && !is_label_position(arr, i + m, n)) {
                found = true
            }
            if (!is_white_space(c)) { lastNonWs = c }
            i++
        }
    }
    return found
}

let JOIN_TI = "linq_join_ti"

def private emit_two_source(q : string; nq : int; varName : string; var2Name : string; srcA : string;   // nolint:STYLE037,STYLE038 - one arm per emitted `_fold` shape; the clause offsets are shared state
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
    let intoPos = find_kw_depth0(q, "into", tailStart)
    if (intoPos >= 0 && !isJoin) {
        macro_error(prog, at, "linq: 'into' over a second 'from' is not supported - group-join 'into' works after a 'join ... on ... equals ...' clause; query-continuation 'into' works on single-source queries")
        return "(0)"
    }
    let rawOrderByPos = find_kw_depth0(q, "orderby", tailStart)
    let selectPos = find_kw_depth0(q, "select", tailStart)
    let groupPos = find_kw_depth0(q, "group", tailStart)
    if (selectPos < 0 && groupPos < 0) {
        macro_error(prog, at, "linq: query must end with a 'select' or 'group ... by ...' clause")
        return "(0)"
    }
    if (selectPos >= 0 && groupPos >= 0) {
        macro_error(prog, at, "linq: query cannot have both 'select' and 'group' - they are alternative terminals")
        return "(0)"
    }
    let isGroup = groupPos >= 0
    let termPos = isGroup ? groupPos : selectPos
    let orderByPos = (rawOrderByPos >= 0 && rawOrderByPos < termPos) ? rawOrderByPos : -1
    let postTermInto = find_kw_depth0(q, "into", isGroup ? groupPos + 5 : selectPos + 6)
    if (postTermInto >= 0) {
        macro_error(prog, at, "linq: query-continuation 'into' after the 'select'/'group' terminal is not supported on a 'join' or multiple-'from' query - it works on single-source queries; a group-join 'into' goes before the terminal (`join ... equals ... into g select ...`)")
        return "(0)"
    }
    if (isGroup && orderByPos >= 0) {
        macro_error(prog, at, "linq: 'orderby' before 'group' is not supported - order the groups after a 'select', or drop the 'orderby'")
        return "(0)"
    }
    var groupIntoVar = ""
    let groupInto = intoPos >= 0 && intoPos < termPos
    if (groupInto) {
        if (isGroup) {
            macro_error(prog, at, "linq: 'group' after 'join ... into' is not yet supported - 'select' the group (e.g. `select (key, g |> length)`), or use the pipe form")
            return "(0)"
        }
        if (orderByPos >= 0) {
            macro_error(prog, at, "linq: 'orderby' after 'join ... into g' is not yet supported - the (outer, g) pair can't be carried (g is a non-copyable array); materialize then sort, or use the pipe form")
            return "(0)"
        }
        let iv = read_ident(q, intoPos + 4)
        if (iv._0 == "") {
            macro_error(prog, at, "linq: 'join ... into' requires a group variable (e.g. `join b in B on ... equals ... into g select ...`)")
            return "(0)"
        }
        if (iv._0 == JOIN_TI || iv._0 == varName || iv._0 == var2Name) {
            macro_error(prog, at, "linq: group variable '{iv._0}' must differ from '{varName}', '{var2Name}', and the reserved '{JOIN_TI}'")
            return "(0)"
        }
        if (strip(slice(q, iv._1, termPos)) != "") {
            macro_error(prog, at, "linq: a 'where' after 'join ... into g' is not yet supported - the (outer, g) pair can't be carried; filter with a pre-'join' 'where', or use the pipe form")
            return "(0)"
        }
        groupIntoVar = iv._0
    }
    if (isJoin) {
        let equals2 = find_kw_depth0(q, "equals", tailStart)
        if (equals2 >= 0 && equals2 < termPos) {
            macro_error(prog, at, "linq: join 'on' supports a single equi-key only - composite keys (`a equals b && c equals d`) need raw SQL")
            return "(0)"
        }
    }
    var preWheres : array<string>
    var postWheres : array<string>
    var firstPostWhere = -1
    var wp = firstWhere
    var whereErr = ""
    peek_data(q) $(qd) {
        while (wp >= 0 && wp < termPos) {
            let wEnd = next_clause_kw(q, wp + 5)
            let predText = strip(slice(qd, wp + 5, wEnd))
            if (predText == "") {
                whereErr = "linq: empty 'where' predicate"
                return
            }
            if (wp < secondIntroPos) {
                preWheres |> push(predText)
            } elif (wp < tailStart) {
                whereErr = (isJoin
                    ? "linq: 'where' cannot appear inside the join clause (between 'join' and 'equals')"
                    : "linq: 'where' cannot appear between the second range variable and 'in'")
                return
            } else {
                if (firstPostWhere < 0) { firstPostWhere = wp }
                postWheres |> push(predText)
            }
            wp = find_kw_depth0(q, "where", wEnd)
        }
    }
    if (whereErr != "") {
        macro_error(prog, at, whereErr)
        return "(0)"
    }
    let hasPostWhere = !empty(postWheres)
    if (groupInto && hasPostWhere) {
        macro_error(prog, at, "linq: a 'where' after 'join ... into g' is not yet supported - the (outer, g) pair can't be carried; filter with a pre-'join' 'where', or use the pipe form")
        return "(0)"
    }
    var tailEnd = termPos
    if (orderByPos >= 0 && orderByPos < tailEnd) { tailEnd = orderByPos }
    if (firstPostWhere >= 0 && firstPostWhere < tailEnd) { tailEnd = firstPostWhere }
    if (groupInto && intoPos < tailEnd) { tailEnd = intoPos }
    let tail = strip(slice(q, tailStart, tailEnd))
    var srcB = srcBForJoin
    var onLambda = ""
    var correlated = false
    if (isJoin) {
        if (keyA == "" || tail == "") {
            macro_error(prog, at, "linq: empty join key - expected `on <keyA> equals <keyB>`")
            return "(0)"
        }
        onLambda = "$({varName}, {var2Name}) => {keyA} == {tail}"
    } else {
        if (tail == "") {
            macro_error(prog, at, "linq: empty source expression after the second 'in'")
            return "(0)"
        }
        correlated = mentions_ident(tail, varName)
        if (!correlated) {
            srcB = build_src(rowType2, tail, true)
        }
    }
    var orderKeys : array<string>
    var orderDescMask = 0u
    if (orderByPos >= 0) {
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
        orderKeys <- op.keys   // nolint:PERF030 - target is the fresh empty decl above; the fill is conditional, so it cannot fold into the declaration
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
            macro_error(prog, at, "linq: 'group' element must be the range variable '{varName}' - element selectors are not yet supported (write `group {varName} by ...`)")
            return "(0)"
        }
        let gk = strip_trailing_keyword(strip(slice(groupBody, byPos + 2, length(groupBody))), "iterator")
        groupKey = gk._0
        wantIterator = gk._1
        if (groupKey == "") {
            macro_error(prog, at, "linq: empty 'group ... by' key expression")
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
    if (correlated) {
        if (orderByPos >= 0) {
            macro_error(prog, at, "linq: 'orderby' over a correlated 'from ... from' (flatten) is not supported - the outer row '{varName}' is non-copyable, so the ordered pairs can't be carried; materialize the result then sort, or restructure as a 'join'")
            return "(0)"
        }
        if (isGroup) {
            macro_error(prog, at, "linq: 'group' over a correlated 'from ... from' (flatten) is not supported - the outer row '{varName}' is non-copyable; 'select' the fields you need instead")
            return "(0)"
        }
        for (pred in postWheres) {
            if (mentions_ident(pred, varName)) {
                macro_error(prog, at, "linq: a post-'from' 'where' over a correlated 'from ... from' may reference only the inner range variable '{var2Name}' (it is pushed into the collection before flattening) - referencing the outer '{varName}' would require carrying the non-copyable outer row; filter the outer with a pre-'from' 'where', or restructure")
                return "(0)"
            }
        }
        srcB = build_string() $(var sw) {
            if (rowType2 != "") {
                sw |> write("$({varName}) => from_in({tail}, type<{rowType2}>)")
            } else {
                sw |> write("$({varName}) => unsafe(each({tail}))")
            }
            for (pred in postWheres) {
                sw |> write(" |> _where($({var2Name}) => {pred})")
            }
        }
    }
    let carryLambda = "$({varName}, {var2Name}) => ({varName} = {varName}, {var2Name} = {var2Name})"
    let names <- [varName, var2Name]
    let hasOrderBy = orderByPos >= 0
    let useTI = !correlated && (hasPostWhere || hasOrderBy || isGroup)
    let opName = groupInto ? "_group_join" : (isJoin ? "_join" : (correlated ? "_select_many" : "_cross_join"))
    let onLambdaPart = isJoin ? "{onLambda}, " : ""
    return build_string() $(var w) {
        w |> write("( _fold( {srcA}")
        for (pred in preWheres) {
            w |> write(" |> _where($({varName}) => {pred})")
        }
        if (!useTI) {
            let rv2 = groupInto ? groupIntoVar : var2Name
            let p1 = mentions_ident(projText, varName) ? varName : "_{varName}"
            let p2 = mentions_ident(projText, rv2) ? rv2 : "_{rv2}"
            w |> write(" |> {opName}({srcB}, {onLambdaPart}$({p1}, {p2}) => {projText})")
        } else {
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
    return emit_two_source(q, nq, varName, var2Name, srcA, false, "", "", rowType2,
                           secondFromPos, in2 + 2, firstWhere, prog, at)
}

def private next_clause_kw(q : string; from : int) : int {
    var best = length(q)
    for (kw in ["let", "where", "orderby", "select", "group", "join", "from", "into"]) {
        let p = find_kw_depth0(q, kw, from)
        if (p >= 0 && p < best) { best = p }
    }
    return best
}

def private extract_lets(q : string; bodyStart : int; rangeVar : string;
                         prog : ProgramPtr; at : LineInfo) : tuple<bool; string> {
    var subs : table<string; string>
    var cuts : array<tuple<int; int; string>>
    let srcEnd = next_clause_kw(q, bodyStart)
    let joinPos = find_kw_depth0(q, "join", bodyStart)
    let secondFromPos = find_kw_depth0(q, "from", bodyStart)
    let var2j = joinPos >= 0 ? read_ident(q, joinPos + 4)._0 : ""
    let var2f = secondFromPos >= 0 ? read_ident(q, secondFromPos + 4)._0 : ""
    var searchFrom = bodyStart
    var letErr = ""
    peek_data(q) $(qd) {
        let nq = length(qd)
        while (true) {
            let lp = find_kw_depth0(q, "let", searchFrom)
            if (lp < 0) return
            let np = read_ident(q, lp + 3)
            let name = np._0
            if (name == "") {
                letErr = "linq: 'let' requires a binding name (e.g. `let n = <expr>`)"
                return
            }
            if (name == rangeVar || name == var2j || name == var2f) {
                letErr = "linq: 'let {name}' shadows the range variable '{name}'"
                return
            }
            if (key_exists(subs, name)) {
                letErr = "linq: 'let {name}' is already bound earlier in the query"
                return
            }
            var eq = -1
            let i = skip_white_space(qd, np._1)
            if (i < nq && int(qd[i]) == '=' && (i + 1 >= nq || int(qd[i + 1]) != '=')) { eq = i }
            if (eq < 0) {
                letErr = "linq: expected '=' after 'let {name}'"
                return
            }
            let rhsStart = eq + 1
            let rhsEnd = next_clause_kw(q, rhsStart)
            var rhs = strip(slice(qd, rhsStart, rhsEnd))
            if (rhs == "") {
                letErr = "linq: empty 'let {name}' expression"
                return
            }
            if (rhsEnd >= nq) {
                letErr = "linq: 'let {name}' has no following clause - a 'let' must precede the 'select'/'group' terminal"
                return
            }
            if (!empty(subs)) { rhs = substitute_idents(rhs, subs) }
            subs[name] = "({rhs})"
            cuts |> push((lp, rhsEnd, name))
            searchFrom = rhsEnd
        }
    }
    if (letErr != "") {
        macro_error(prog, at, letErr)
        return (false, "")
    }
    if (empty(subs)) return (true, q)
    let body = build_string() $(var w) {
        peek_data(q) $(qd) {
            var active : table<string; string>
            var pos = srcEnd
            for (c in cuts) {
                w |> write(substitute_idents(slice(qd, pos, c._0), active))
                w |> write(" ")
                active[c._2] = subs[c._2]
                pos = c._1
            }
            w |> write(substitute_idents(slice(qd, pos, length(qd)), active))
        }
    }
    return (true, "{slice(q, 0, srcEnd)}{body}")
}

def private rewrite_group_var(text : string; gname : string) : string {
    return build_string() $(var w) {
        peek_data(text) $(arr) {   // nolint:STYLE037 - one byte-level scanner state machine
            let n = length(arr)
            var i = 0
            var inStr = false
            var interpDepth = 0
            var lastNonWs = 0
            while (i < n) {
                let c = int(arr[i])
                if (inStr && interpDepth == 0) {
                    if (c == '\\' && i + 1 < n) { w |> write_string(arr, i, i + 2); i += 2; continue }
                    w |> write_string(arr, i, i + 1)
                    if (c == '"') { inStr = false; lastNonWs = c }
                    elif (c == '{') { interpDepth = 1; lastNonWs = 0 }
                    i++
                    continue
                }
                if (c == '"') {
                    if (interpDepth > 0) {
                        w |> write_string(arr, i, i + 1); i++
                        while (i < n) {
                            let cc = int(arr[i])
                            if (cc == '\\' && i + 1 < n) { w |> write_string(arr, i, i + 2); i += 2; continue }
                            w |> write_string(arr, i, i + 1); i++
                            if (cc == '"') break
                        }
                        lastNonWs = '"'
                        continue
                    }
                    inStr = true; lastNonWs = c; w |> write_string(arr, i, i + 1); i++
                    continue
                }
                if (interpDepth > 0 && c == '{') { interpDepth ++; w |> write_string(arr, i, i + 1); lastNonWs = c; i++; continue }
                if (interpDepth > 0 && c == '}') {
                    interpDepth --
                    w |> write_string(arr, i, i + 1)
                    lastNonWs = interpDepth == 0 ? '"' : c
                    i++
                    continue
                }
                let atWordStart = is_ident_byte(c) && (i == 0 || !is_ident_byte(int(arr[i - 1])))
                if (atWordStart) {
                    var j = i
                    while (j < n && is_ident_byte(int(arr[j]))) { j++ }
                    let word = slice(arr, i, j)
                    if (lastNonWs != '.' && word == gname && !is_label_position(arr, j, n)) {
                        let dotFollows = j < n && int(arr[j]) == '.'
                        var keyEnd = -1
                        if (dotFollows) {
                            var k = j + 1
                            while (k < n && is_ident_byte(int(arr[k]))) { k++ }
                            if (slice(arr, j + 1, k) == "key") { keyEnd = k }
                        }
                        if (keyEnd >= 0) {
                            w |> write("{gname}._0")
                            i = keyEnd
                            lastNonWs = '_'
                            continue
                        } elif (!dotFollows) {
                            w |> write("{gname}._1")
                            i = j
                            lastNonWs = '_'
                            continue
                        }
                    }
                    w |> write(word)
                    lastNonWs = int(arr[j - 1])
                    i = j
                    continue
                }
                w |> write_string(arr, i, i + 1)
                if (!is_white_space(c)) { lastNonWs = c }
                i++
            }
        }
    }
}

struct private QueryStage {
    rangeVar : string = ""
    isGroupVar : bool = false
    predicates : array<string>
    hasOrderBy : bool = false
    orderKeys : array<string>
    orderDescMask : uint = 0u
    isGroup : bool = false
    groupKey : string = ""
    projText : string = ""
    identitySelect : bool = false
    wantIterator : bool = false
    intoVar : string = ""
    nextStart : int = 0
}

def private parse_one_stage(q : string; nq : int; start : int; rangeVar : string; isGroupVar : bool;   // nolint:STYLE037,STYLE038 - one arm per query clause; the arms share the scan cursor
                            prog : ProgramPtr; at : LineInfo; var stage : QueryStage) : bool {
    stage.rangeVar = rangeVar
    stage.isGroupVar = isGroupVar
    let selectPos = find_kw_depth0(q, "select", start)
    let groupPos = find_kw_depth0(q, "group", start)
    if (selectPos < 0 && groupPos < 0) {
        macro_error(prog, at, "linq: query must end with a 'select' or 'group ... by ...' clause")
        return false
    }
    let isGroup = groupPos >= 0 && (selectPos < 0 || groupPos < selectPos)
    let termPos = isGroup ? groupPos : selectPos
    let otherPos = isGroup ? selectPos : groupPos
    let termKwEnd = isGroup ? termPos + 5 : termPos + 6
    let intoPos = find_kw_depth0(q, "into", termKwEnd)
    let termEnd = intoPos >= 0 ? intoPos : nq
    if (otherPos >= 0 && otherPos < termEnd) {
        macro_error(prog, at, "linq: query cannot have both 'select' and 'group' - they are alternative terminals")
        return false
    }
    stage.isGroup = isGroup
    var wherePos = find_kw_depth0(q, "where", start)
    var emptyWhere = false
    peek_data(q) $(qd) {
        while (wherePos >= 0 && wherePos < termPos) {
            let predEnd = next_clause_kw(q, wherePos + 5)
            let predText = strip(slice(qd, wherePos + 5, predEnd))
            if (predText == "") {
                emptyWhere = true
                return
            }
            stage.predicates |> push(isGroupVar ? rewrite_group_var(predText, rangeVar) : predText)
            wherePos = find_kw_depth0(q, "where", predEnd)
        }
    }
    if (emptyWhere) {
        macro_error(prog, at, "linq: empty 'where' predicate")
        return false
    }
    let orderByPos = find_kw_depth0(q, "orderby", start)
    stage.hasOrderBy = orderByPos >= 0 && orderByPos < termPos
    if (isGroup && stage.hasOrderBy) {
        macro_error(prog, at, "linq: 'orderby' before 'group' is not supported - order the groups after a 'select', or drop the 'orderby'")
        return false
    }
    if (stage.hasOrderBy) {
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
            macro_error(prog, at, "linq: 'group' element must be the range variable '{rangeVar}' - element selectors are not yet supported (write `group {rangeVar} by ...`)")
            return false
        }
        let gk = strip_trailing_keyword(strip(slice(groupBody, byPos + 2, length(groupBody))), "iterator")
        let groupKey = gk._0
        stage.wantIterator = gk._1
        if (groupKey == "") {
            macro_error(prog, at, "linq: empty 'group ... by' key expression")
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
        stage.identitySelect = (strip_redundant_parens(projText) == rangeVar)
        stage.projText = (isGroupVar && !stage.identitySelect) ? rewrite_group_var(projText, rangeVar) : projText
    }
    if (intoPos >= 0) {
        if (stage.wantIterator) {
            macro_error(prog, at, "linq: 'iterator' must be the final clause - it cannot precede an 'into' continuation")
            return false
        }
        let iv = read_ident(q, intoPos + 4)
        if (iv._0 == "") {
            macro_error(prog, at, "linq: 'into' requires a continuation range variable (e.g. `... into g select ...`)")
            return false
        }
        if (iv._0 == JOIN_TI) {
            macro_error(prog, at, "linq: continuation range variable '{JOIN_TI}' is reserved")
            return false
        }
        if (strip(slice(q, iv._1, nq)) == "") {
            macro_error(prog, at, "linq: 'into {iv._0}' has no continuation - add a 'where'/'orderby'/'select'/'group' after it")
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

def private transpile_query(query : string; prog : ProgramPtr; at : LineInfo) : string {   // nolint:STYLE038 - one arm per query form; the dispatch IS the transpiler
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
    let srcStart = inPos + 2
    let lets = extract_lets(q, srcStart, varName, prog, at)
    if (!lets._0) return "(0)"
    q = lets._1
    let nq = length(q)
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
    //! Source dispatcher for the C# query `from` clause. The typed form `from c : Row in <src>` arrives as `from_in(src, type<Row>)` and rewrites to the concrete `_fold` source builder by `src`'s type — a SQL runner → `select_from`, an XML node → `from_xml_node`, a JSON value → `from_json`. The UNTYPED form `from c in <src>` arrives as `from_in(src)` — a table → `each_kv` (`keys` for the `table<K>` set form), anything else → `each` (the array path). Must be a call macro: a plain function would leave `from_in(...)` at the chain head, which `_fold`'s name-based source detection cannot route.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        if (length(call.arguments) != 1 && length(call.arguments) != 2) {
            macro_error(prog, call.at, "from_in(src[, type<Row>]): expected 1 or 2 arguments")
            return null
        }
        let srcT = call.arguments[0]._type
        if (srcT == null || srcT.isAutoOrAlias) {
            macro_error(prog, call.at, "from_in: source type is not inferred")
            return null
        }
        if (length(call.arguments) == 1) {
            if (srcT.isGoodTableType) {
                if (srcT.secondType == null || srcT.secondType.baseType == Type.tVoid)
                    return qmacro(unsafe($c("keys")($e(call.arguments[0]))))
                return qmacro(unsafe($c("each_kv")($e(call.arguments[0]))))
            }
            return qmacro(unsafe($c("each")($e(call.arguments[0]))))
        }
        if (srcT.isGoodTableType) {
            macro_error(prog, call.at, "linq: a table source takes no row-type annotation - write `from kv in <tab>` (kv.key / kv.value)")
            return null
        }
        if (srcT.structType != null && srcT.structType.name == "SqlRunner"
                && srcT.structType._module != null && srcT.structType._module.name == "sqlite_boost")
            return qmacro($c("select_from")($e(call.arguments[0]), $e(call.arguments[1])))
        if (srcT.isHandle && srcT.annotation != null && srcT.annotation.name == "xml_node")
            return qmacro(unsafe($c("from_xml_node")($e(call.arguments[0]), $e(call.arguments[1]))))
        if (srcT.isPointer && srcT.firstType != null && srcT.firstType.structType != null
                && srcT.firstType.structType.name == "JsonValue"
                && srcT.firstType.structType._module != null && srcT.firstType.structType._module.name == "json")
            return qmacro(unsafe($c("from_json")($e(call.arguments[0]), $e(call.arguments[1]))))
        macro_error(prog, call.at, "linq: unsupported source for `from c : Row in <src>` - expected a SQL runner, an XML node, or a JSON value (use `in decs` for decs; arrays and tables use the untyped `from c in <src>` form)")
        return null
    }
}

[reader_macro(name=linq)]
class LinqDasReader : AstReaderMacro {
    //! C#-style LINQ query reader macro.
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
        outLine = int(info.line)
        return transpile_query(string(expr.sequence), prog, info)
    }
}
