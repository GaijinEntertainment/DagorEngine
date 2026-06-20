options gen2

require strings
require daslib/strings_boost
require canonical


// PatternHit is the verdict of `classify`. A non-empty `name` flags
// the function as matching a known "boring" shape (dispatcher, repeated
// init, etc.) — the caller drops it from clustering unless the user
// explicitly asked to keep this pattern via `--keep <name>`.
struct public PatternHit {
    name : string
    note : string
}


// Single entry point for "is this function carrying real signal?"
// Returns `name == ""` when the function is interesting; non-empty
// otherwise. Patterns are tried in priority order — the first match
// wins, which means more specific shapes should be checked before
// more general ones.
def public classify(name : string; canonical : string) : PatternHit {
    var hit = PatternHit()
    // Visitor methods are recognized by name (`<Class>`<hook>`) — the
    // body of a real visitor override is structurally constrained by
    // the dispatch contract, not by the algorithm. Check first so that
    // a visitor whose body happens to match `dispatch`/`emit` is still
    // classified under the more semantic `visitor` bucket.
    // Single-statement dastest test wrappers:
    //   def test_xxx(t : T?) { t |> run("desc") @(t : T?) { ... } }
    // The canonicalizer collapses the lambda body to ADDR, so every
    // such test canonicalizes identically. `dispatch` only fires at
    // N >= 2 statements, so a one-run-per-def test slips through —
    // hence this name-tagged single-statement variant. Check before
    // `emit` because `emit` is the more general "small N trivial calls"
    // shape; `test_wrapper` is the more specific name+shape match.
    if (try_visitor(name, hit) || try_dispatch(canonical, hit)
        || try_test_wrapper(name, canonical, hit) || try_emit(canonical, hit)) return hit
    return hit
}


// Visitor methods follow a fixed naming convention: a class-qualified
// method name (`<Class>`<hook>`) where the hook starts with one of
// `visit`, `preVisit`, `postVisit`, `before`, or `after`. These are
// `AstVisitor` overrides whose body shape is dictated by the dispatch
// contract (one method per AST node type), so cross-class duplication
// at the canonical level is structural — the function name carries
// the dispatch, not the body. Common in `daslib/aot_cpp.das`,
// `daslib/ast_print.das`, `daslib/templates_boost.das`,
// `daslib/rst_comment.das`, `daslib/perf_lint.das`, etc. Use
// `--keep visitor` to include them.
def try_visitor(name : string; var hit : PatternHit) : bool {
    let bt = find(name, "`")
    if (bt < 0) return false
    let method = slice(name, bt + 1)
    if (method |> starts_with("preVisit")
            || method |> starts_with("postVisit")
            || method |> starts_with("visit")
            || method |> starts_with("before")
            || method |> starts_with("after")) {
        hit.name := "visitor"
        hit.note := "{method}"
        return true
    }
    return false
}


// Dispatch shape: the function body is N >= 2 byte-identical top-level
// statement chunks. Catches dastest's `t |> run("X") @(t) { … }` lists
// and any other "uniform call list" outer function. Lambda bodies are
// collapsed to a single `ADDR` token upstream by the canonicalizer, so
// two `run` statements look identical regardless of what the lambdas
// actually do — meaning the outer function carries zero unique
// structure beyond "this many calls".
def try_dispatch(canonical : string; var hit : PatternHit) : bool {
    let stmts <- split_top_level_stmts(canonical)
    if (length(stmts) < 2) return false
    for (i in range(1, length(stmts))) {
        if (stmts[i] != stmts[0]) return false
    }
    hit.name := "dispatch"
    hit.note := "{length(stmts)}x repeated"
    return true
}


// Single-statement dastest test wrapper: a function whose name starts
// with `test_` and whose body is exactly one top-level `t |> run(...)`
// call carrying a lambda body (one or more `ADDR` tokens — captures +
// the lambda function pointer). The canonicalizer collapses the lambda
// body to `ADDR`, so every dastest with a single named subtest produces
// the same canonical regardless of what the lambda actually does. This
// pattern catches them; users opt back in via `--keep test_wrapper`.
def try_test_wrapper(name : string; canonical : string; var hit : PatternHit) : bool {
    if (!(name |> starts_with("test_"))) return false
    let stmts <- split_top_level_stmts(canonical)
    if (length(stmts) != 1) return false
    let toks <- tokenize_canonical(stmts[0])
    if (length(toks) < 2
        || (toks[0] != "STMT" && toks[0] != "FIN_STMT")
        || toks[1] != "CALL:run") return false
    var addr_count = 0
    for (i in range(2, length(toks))) {
        if (toks[i] == "ADDR") {
            addr_count++
        }
    }
    if (addr_count == 0) return false
    hit.name := "test_wrapper"
    hit.note := "single t |> run(...) body"
    return true
}


// Emit shape: the function body is a small number (1..6) of top-level
// statements, each being either a single trivial `CALL:foo(...)` (only
// literal/var/field args — no control flow, no nested calls) or a
// single trivial `RET ...`. Catches the visitor-pattern boilerplate
// in `daslib/aot_cpp.das`, `daslib/ast_print.das`, etc., where ~50
// `def override visitExprXxx(...) { write(*ss, ")") ; return that }`
// methods all collapse to byte-identical canonical streams that just
// emit a string and return. The function name is what carries the
// dispatch (one override per AST node type), so the body duplication
// is structurally required and not actionable.
def try_emit(canonical : string; var hit : PatternHit) : bool {
    let stmts <- split_top_level_stmts(canonical)
    if (empty(stmts) || length(stmts) > 6) return false
    var emit_count = 0
    for (stmt in stmts) {
        let toks <- tokenize_canonical(stmt)
        if (length(toks) < 2 || (toks[0] != "STMT" && toks[0] != "FIN_STMT")) return false
        let head = toks[1]
        if (head == "RET") {
            // Plain return of a var/null/literal — accept.
            if (!emit_args_trivial(toks, 2, length(toks))) return false
            continue
        }
        if (head |> starts_with("CALL:")) {
            // A single top-level call with only trivial args.
            if (!emit_args_trivial(toks, 2, length(toks))) return false
            emit_count++
            continue
        }
        return false
    }
    if (emit_count == 0) return false
    hit.name := "emit"
    hit.note := "{emit_count} trivial-call stmt(s)"
    return true
}


// Args of a trivial-emit statement may only contain literals, var/sym
// refs, field reads, pointer dereferences (P2R), unary operators, and
// string-builder pieces (MK_STR, SBLD). Anything else (a nested call,
// a control-flow keyword, OP2 arithmetic, lambda) means the statement
// is doing real work — not boilerplate.
def emit_args_trivial(toks : array<string>; lo, hi : int) : bool {
    for (i in range(lo, hi)) {
        let tok = toks[i]
        if (tok |> starts_with("<var_") || tok == "<sym>"
                || tok == ".FLD" || tok == "P2R" || tok == "LIT" || tok == "MK_STR"
                || tok == "SBLD" || tok == "ENDSBLD"
                || tok |> starts_with("OP1:")) continue
        return false
    }
    return true
}


// Split a canonical token stream into its top-level statement chunks.
// "Top-level" means within the outer `BODY BLK … ENDBLK` block, NOT
// nested inside any inner BLK/ENDBLK pair (loop bodies, if/then/else
// bodies, lambda bodies via MK_BLK, with/assume blocks).
//
// Tracks BLK depth on a flat token walk. `STMT` and `FIN_STMT` are
// both treated as boundary markers so a function body that ends in
// a final-expression block doesn't merge its last two statements.
//
// Returns an empty array for forward-declarations and other
// body-less canonicals (no `BODY BLK` pair found).
def public split_top_level_stmts(canonical : string) : array<string> {
    var out : array<string>
    let toks <- tokenize_canonical(canonical)
    let n = length(toks)
    var i = 0
    while (i + 1 < n && !(toks[i] == "BODY" && toks[i + 1] == "BLK")) {
        i++
    }
    if (i + 1 >= n) return <- out
    i += 2
    var depth = 0
    var cur_start = -1
    while (i < n) {
        let tok = toks[i]
        if (depth == 0 && (tok == "STMT" || tok == "FIN_STMT")) {
            if (cur_start >= 0) {
                out |> push(join_tokens(toks, cur_start, i))
            }
            cur_start = i
        } elif (tok == "BLK") {
            depth++
        } elif (tok == "ENDBLK") {
            if (depth == 0) {
                if (cur_start >= 0) {
                    out |> push(join_tokens(toks, cur_start, i))
                }
                return <- out
            }
            depth--
        }
        i++
    }
    return <- out
}


def join_tokens(toks : array<string>; lo : int; hi : int) : string {
    var sb : array<string>
    sb |> reserve(hi - lo)
    for (k in range(lo, hi)) {
        sb |> push(toks[k])
    }
    return sb |> join(" ")
}
