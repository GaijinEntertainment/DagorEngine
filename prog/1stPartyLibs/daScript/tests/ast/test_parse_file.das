options gen2
options rtti = true
options strict_smart_pointers = false
options no_aot

require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost
require daslib/rtti
require daslib/fio
require daslib/strings_boost
require strings

//! `parse_file` hands over the AST as parsed, before infer. Source-rewriting tools need that
//! tree: infer lowers make-syntax, folds constants and inserts generated nodes, none of which
//! map back to a source span. The fixture is v1 syntax, so this also pins that a gen1 parse is
//! reachable from das (version_2_syntax = false).
//!
//! It further pins WHERE the spans are, because a rewriter's anchors depend on it: a make-node's
//! own `at` covers only its 2-char opener (`[[` / `\{\{`), while the enclosing `ExprLet.atInit`
//! reaches past the closer. A change in either is a deliberate decision, not a detail — this
//! test is the tripwire.
//!
//! The AST dies with the program, so every assertion runs INSIDE the parse_file block: an
//! ExpressionPtr kept past it is a dangling gc_node pointer.

let FIXTURE = "{get_das_root()}/tests/ast/_fixture_parse_file_gen1.das"


def private read_lines(file : string) : array<string> {
    var lines <- fread(file) |> split("\n")
    for (ln in lines) {
        if (ln |> ends_with("\r")) {
            ln = slice(ln, 0, length(ln) - 1)
        }
    }
    return <- lines
}

def private text_at(lines : array<string>; line, column, count : int) : string {
    if (line < 1 || line > length(lines)) return ""
    let ln = lines[line - 1]
    if (column < 0 || column + count > length(ln)) return ""
    return slice(ln, column, column + count)
}

def private span_head(lines : array<string>; li : LineInfo; count : int) : string {
    return text_at(lines, int(li.line), int(li.column), count)
}

def private span_tail(lines : array<string>; li : LineInfo; count : int) : string {
    return text_at(lines, int(li.last_line), int(li.last_column) - count, count)
}


def private collect_inits(var blk : ExprBlock?; var inits : array<ExpressionPtr>;
                          var initEndLine, initEndColumn : array<int>) {
    for (stmt in blk.list) {
        var letExpr = stmt ?as ExprLet
        if (letExpr == null) continue
        for (v in letExpr.variables) {
            if (v.init == null) continue
            inits |> push(v.init)
            initEndLine |> push(int(letExpr.atInit.last_line))
            initEndColumn |> push(int(letExpr.atInit.last_column))
        }
    }
}


def private check_make_nodes(t : T?; var fn : FunctionPtr; lines : array<string>) {
    var blk = fn.body ?as ExprBlock
    if (blk == null) {
        t |> failure("make_them has no block body")
        return
    }
    var inits : array<ExpressionPtr>
    var initEndLine : array<int>
    var initEndColumn : array<int>
    collect_inits(blk, inits, initEndLine, initEndColumn)
    t |> equal(length(inits), 4, "four initializers in make_them")
    if (length(inits) != 4) return

    // [[Foo a=1, b=2.0]] stays a make-struct; `useInitializer` records that no () was written
    var msd = inits[0] ?as ExprMakeStruct
    t |> success(msd != null, "first init is ExprMakeStruct")
    if (msd != null) {
        t |> success(!msd.makeStructFlags.useInitializer, "no () in [[Foo ...]] means useInitializer == false")
        t |> equal(length(msd.structs), 1, "one instance")
    }

    // [[auto 1; 2; 3]] — the v1 spelling is recorded in the tree, so a rewriter does not have
    // to consult the source to know which syntax was written
    var mka = inits[1] ?as ExprMakeArray
    t |> success(mka != null, "second init is ExprMakeArray")
    if (mka != null) {
        t |> success(!mka.gen2, "v1 make-array carries gen2 == false")
        t |> equal(length(mka.values), 3, "three values")
    }

    // [[auto 4, 5]] is a one-element make-array holding a make-tuple; infer is what collapses
    // that into a bare tuple, so a pre-infer walk has to expect the wrapper
    var mkt = inits[2] ?as ExprMakeArray
    t |> success(mkt != null, "[[auto 4, 5]] is ExprMakeArray")
    if (mkt != null) {
        t |> equal(length(mkt.values), 1, "one value")
        if (length(mkt.values) == 1) {
            t |> success(mkt.values[0] ?as ExprMakeTuple != null, "the value is ExprMakeTuple")
        }
    }

    // {{ k => v }} is lowered to a to_table_move call over a make-array AT PARSE TIME, so
    // make-table detection is call-shaped, not node-shaped
    var tabCall = inits[3] ?as ExprCall
    t |> success(tabCall != null, "fourth init is ExprCall")
    if (tabCall != null) {
        t |> success(find(string(tabCall.name), "to_table_move") >= 0, "call is to_table_move, got {tabCall.name}")
        t |> equal(length(tabCall.arguments), 1, "one argument")
        if (length(tabCall.arguments) == 1) {
            t |> success(tabCall.arguments[0] ?as ExprMakeArray != null, "argument is ExprMakeArray")
        }
    }

    check_make_spans(t, inits, initEndLine, initEndColumn, lines)
}


def private check_make_spans(t : T?; inits : array<ExpressionPtr>;
                             initEndLine, initEndColumn : array<int>; lines : array<string>) {
    // a make-node's own span is JUST the opener — the closer is nowhere in the tree, which is
    // why a rewriter has to scan forward for `]]` / `\}\}`
    t |> equal(span_head(lines, inits[0].at, 2), "[[", "make-struct span starts at [[")
    t |> equal(span_tail(lines, inits[0].at, 2), "[[", "make-struct span ENDS at [[ too")
    t |> equal(span_head(lines, inits[1].at, 2), "[[", "make-array span starts at [[")
    t |> equal(span_tail(lines, inits[1].at, 2), "[[", "make-array span ENDS at [[ too")
    t |> equal(span_head(lines, inits[3].at, 2), "\{\{", "make-table span starts at \{\{")

    // the enclosing let's atInit is the anchor that does include the closer
    t |> equal(text_at(lines, initEndLine[0], initEndColumn[0] - 2, 2), "]]", "atInit ends at ]]")
    t |> equal(text_at(lines, initEndLine[1], initEndColumn[1] - 2, 2), "]]", "atInit ends at ]]")
    t |> equal(text_at(lines, initEndLine[3], initEndColumn[3] - 2, 2), "\}\}", "atInit ends at \}\}")
}


[test]
def test_parse_file_gen1_pre_infer(t : T?) {
    t |> run("v1 make-syntax and its spans survive the parse") @(t : T?) {
        let lines <- read_lines(FIXTURE)
        if (empty(lines)) {
            t |> success(false, "could not read {FIXTURE}")
            return
        }
        var seen = false
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.ignore_shared_modules = true
                cop.export_all = true
                cop.version_2_syntax = false
                cop.no_optimizations = true
                parse_file(FIXTURE, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    if (!ok) {
                        t |> success(false, "fixture failed to parse: {issues}")
                        return
                    }
                    for_each_function(get_this_module(program), "") $(var fn) {
                        // required modules contribute generic instances (builtin's
                        // to_table_move, which the \{\{...\}\} lowering emits) — only walk
                        // the fixture's own code
                        if (fn.at.fileInfo == null || fn.name != "make_them"
                                || !(fn.at.fileInfo.name |> ends_with("_fixture_parse_file_gen1.das"))) return
                        seen = true
                        check_make_nodes(t, fn, lines)
                    }
                }
            }
        }
        t |> success(seen, "make_them was found in the parsed module")
    }
}
