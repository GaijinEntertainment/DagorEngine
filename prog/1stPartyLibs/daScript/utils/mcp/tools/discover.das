options gen2

module discover

require common public
require daslib/fio
require strings
require daslib/strings_boost
require daslib/strings_convert
require daslib/utf8_utils
require daslib/fts5_query
require daslib/result
require daslib/logger
require math

//! `discover` - the question to ask before writing a helper: does the standard library already
//! do this? The corpus is the generated module digest, `skills/daslang/references/everything.md`.

struct private Row {
    _module : string
    group : string
    name : string
    name_lower : string
    description : string
    haystack : string
    name_tokens : array<string>
    group_tokens : array<string>
    desc_tokens : array<string>
}

struct private Hit {
    idx : int
    exact : bool
    matched_terms : int
    score : float
}

let private SYNONYM_WEIGHT = 0.4
let private ORDER_BONUS = 1.5
let private CONNECTOR_SHARE = 1.0 / 3.0

// `to` and `from` stay: they are the direction word in `uri_to_file_name` / `uri_from_file_name`.
let private STOPWORDS <- {
    "a", "an", "the", "of", "in", "for", "and", "or", "not", "near", "with",
    "into", "is", "it", "this", "that", "on", "as", "by", "be", "are", "at", "do", "does",
    "i", "how", "can", "any", "which", "what", "one", "given"
}

// Direction and accessor words sit in hundreds of names; they carry order (see `order_bonus`),
// not weight, so a hit on one counts a third of a real word.
let private CONNECTORS <- { "to", "from", "get", "set", "has" }

let private SYNONYMS <- {
    "path" => ["file", "filename", "dir"],
    "file" => ["path", "filename"],
    "filename" => ["file", "path", "name"],
    "url" => ["uri"],
    "uri" => ["url"],
    "dir" => ["directory", "folder"],
    "directory" => ["dir", "folder"],
    "folder" => ["dir", "directory"],
    "str" => ["string"],
    "string" => ["str", "text"],
    "text" => ["string"],
    "arr" => ["array"],
    "remove" => ["delete", "erase"],
    "delete" => ["remove", "erase"],
    "erase" => ["remove", "delete"],
    "find" => ["search", "lookup", "index"],
    "search" => ["find", "lookup"],
    "create" => ["make", "new"],
    "make" => ["create", "new"],
    "load" => ["read", "open"],
    "read" => ["load"],
    "save" => ["write"],
    "write" => ["save"],
    "length" => ["len", "size", "count"],
    "size" => ["length", "len"],
    "count" => ["length", "size"],
    "join" => ["concat", "concatenate"],
    "concat" => ["join", "append"],
    "escape" => ["encode", "quote"],
    "unescape" => ["decode", "unquote"],
    "encode" => ["escape"],
    "decode" => ["unescape", "parse"],
    "parse" => ["decode", "scan", "read"],
    "print" => ["format", "sprint", "write"],
    "format" => ["print", "sprint"],
    "convert" => ["cast"],
    "hash" => ["digest", "sha"],
    "random" => ["rand", "rng"],
    "time" => ["clock", "date", "ticks"],
    "current" => ["now"],
    "now" => ["current"],
    "whole" => ["entire", "all", "full"],
    "entire" => ["whole", "all"],
    "milliseconds" => ["ms", "msec", "usec", "microseconds", "ticks"],
    "microseconds" => ["usec", "us", "milliseconds", "ticks"],
    "thread" => ["job", "async"],
    "job" => ["thread", "async"],
    "sort" => ["order"],
    "lower" => ["lowercase"],
    "upper" => ["uppercase"]
}

// An SDK installs the bundle twice: under `DAS_INSTALL_DOCDIR/skills/daslang` and, pinned to
// the install root, under `.claude/skills/daslang` - the second copy answers when the docs
// were relocated.
let private DIGEST_LOCATIONS <- ["skills/daslang/references/everything.md", ".claude/skills/daslang/references/everything.md"]

def private digest_path() : string {
    let root = get_das_root()
    for (rel in DIGEST_LOCATIONS) {
        let path = path_join(root, rel)
        let st = stat(path)
        return path if (st.is_valid && st.is_reg)
    }
    return path_join(root, DIGEST_LOCATIONS[0])
}

def private tokens_of(text : string) : array<string> {
    var out : array<string>
    each_word(text |> replace("_", " "), out)
    return <- out
}

def private parse_digest(text : string) : array<Row> {
    var rows : array<Row>
    var mod = ""
    var group = ""
    for (raw in text |> replace("\r\n", "\n") |> split("\n")) {
        if (raw |> starts_with("## ")) {
            mod = trim(slice(raw, 3))
            group = ""
            continue
        }
        if (raw |> starts_with("### ")) {
            group = trim(slice(raw, 4))
            continue
        }
        continue if (!(raw |> starts_with("- `")))
        let close = find(raw, "`", 3)
        continue if (close < 0)
        let name = slice(raw, 3, close)
        let rest = slice(raw, close + 1)
        let description = rest |> starts_with(" - ") ? slice(rest, 3) : ""
        rows |> emplace(Row(_module = mod, group = group, name = name, name_lower = to_lower(name),
            description = description,
            haystack = "{name |> replace("_", " ")} {name} {group} {description}",
            name_tokens <- tokens_of(name), group_tokens <- tokens_of(group), desc_tokens <- tokens_of(description)))
    }
    return <- rows
}

def private has_fts5_syntax(query : string) : bool {
    for (op in ["\"", "*", "(", ")", " AND ", " OR ", " NOT "]) {
        return true if (find(query, op) >= 0)
    }
    return false
}

def private term_score(row : Row; term : string) : float {
    let raw = raw_term_score(row, term)
    return CONNECTORS |> key_exists(term) ? raw * CONNECTOR_SHARE : raw
}

def private best_hit(tokens : array<string>; term : string; exact_weight, prefix_weight : float) : float {
    var s = 0.0
    for (t in tokens) {
        if (t == term) {
            s = max(s, exact_weight)
        } elif (t |> starts_with(term)) {
            s = max(s, prefix_weight)
        }
    }
    return s
}

def private raw_term_score(row : Row; term : string) : float {
    var s = best_hit(row.name_tokens, term, 3.0, 2.0)
    if (s == 0.0 && find(row.name_lower, term) >= 0) {
        s = 1.5
    }
    s = max(s, best_hit(row.group_tokens, term, 1.5, 1.5))
    return max(s, best_hit(row.desc_tokens, term, 1.0, 0.7))
}

def private name_position(row : Row; term : string) : int {
    for (t, i in row.name_tokens, count()) {
        return i if (t == term || t |> starts_with(term))
    }
    return -1
}

// `uri_to_file_name` and `file_name_to_uri` share every token; the query's word order is
// what tells them apart, so a name whose hits sit in query order outranks its mirror.
def private order_bonus(row : Row; terms : array<string>) : float {
    var last = -1
    var ordered = 0
    for (term in terms) {
        let pos = name_position(row, term)
        continue if (pos < 0)
        return 0.0 if (pos < last)
        last = pos
        ordered++
    }
    return ordered >= 2 ? ORDER_BONUS : 0.0
}

def private score_row(row : Row; terms : array<string>; query_lower : string; idx : int) : Hit {
    var total = order_bonus(row, terms)
    var matched = 0
    for (term in terms) {
        var best = term_score(row, term)
        SYNONYMS |> get(term) $(alts) {
            for (alt in alts) {
                best = max(best, SYNONYM_WEIGHT * term_score(row, alt))
            }
        }
        if (best > 0.0) {
            matched++
        }
        total += best
    }
    return Hit(idx = idx, exact = row.name_lower == query_lower, matched_terms = matched, score = total)
}

def private query_terms(query : string) : array<string> {
    return <- [for (w in tokens_of(query)); w; where !(STOPWORDS |> key_exists(w))]
}

def private parse_limit(limit : string) : int {
    return 12 if (empty(limit))
    let r <- try_to_int(limit)
    return is_ok(r) && unwrap(r) > 0 ? unwrap(r) : 12
}

def do_discover(query, limit, module_filter : string) : string {
    return make_tool_result("'query' argument is required", true) if (empty(trim(query)))
    let path = digest_path()
    let text = fread(path)
    return make_tool_result("module digest not found: {path} - regenerate it with `daslang -documentation doc/reflections/das2rst.das`", true) if (empty(text))
    let rows <- parse_digest(text)
    let terms <- query_terms(query)
    let query_lower = to_lower(trim(query))
    let use_fts = has_fts5_syntax(query)
    var compiled <- try_compile_text_match(use_fts ? query : "")
    return make_tool_result("invalid FTS5 query: {unwrap_err(compiled)}", true) if (is_err(compiled))
    let fts <- move_unwrap(compiled)
    var hits : array<Hit>
    for (row, i in rows, count()) {
        continue if ((!empty(module_filter) && row._module != module_filter) || (use_fts && !text_match(row.haystack, fts)))
        let h = score_row(row, terms, query_lower, i)
        continue if (h.score <= 0.0 && !use_fts && !h.exact)
        hits |> push(h)
    }
    // An exact name wins outright; then the row covering more of the query's words outranks any
    // score a fewer-word row piles up on one strong name token; the score orders equal coverage.
    hits |> sort($(a, b) => a.exact != b.exact ? a.exact : (a.matched_terms != b.matched_terms ? a.matched_terms > b.matched_terms : (a.score != b.score ? a.score > b.score : rows[a.idx].name < rows[b.idx].name)))
    let shown = min(length(hits), parse_limit(limit))
    let top = empty(hits) ? "-" : "{rows[hits[0].idx]._module}::{rows[hits[0].idx].name}"
    let logged_query = query |> replace("\r", " ") |> replace("\n", " ")
    logger_info("mcp.discover", "\"{logged_query}\" -> {length(hits)} hits, top {top}")
    return make_tool_result(build_string() $(var w) {
        if (empty(hits)) {
            w |> write("no symbol matches \"{query}\" among {length(rows)}. Try fewer or different words; FTS5 syntax works too: uri*, \"file name\", path NOT windows.\n")
            return
        }
        w |> write("{length(hits)} match(es) for \"{query}\" among {length(rows)} symbols; showing {shown}\n")
        for (i in range(shown)) {
            let ri = hits[i].idx
            let tail = empty(rows[ri].group) ? "" : "  [{rows[ri].group}]"
            let desc = empty(rows[ri].description) ? "" : " - {rows[ri].description}"
            w |> write("  {rows[ri]._module}::{rows[ri].name}{desc}{tail}\n")
        }
    })
}
