options gen2
options rtti = true
options strict_smart_pointers = false
options no_aot

require dastest/testing_boost public
require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/fio
require daslib/strings_boost
require strings

//! Regression for three LineInfo range bugs (see utils/internal/lineinfo-audit):
//! 1. ExprLet.atInit used to include the lexer-synthesized statement-terminating
//!    semicolon, overshooting one phantom column past the end of line.
//! 2. Closure/block-arg bodies started one column early — the hidden mid-rule
//!    action before '{' in expression_block leaked its zero-width between-token
//!    location into the block span via ast_makeBlock.
//! 3. Class-method bodies are wrapped in a synthesized ExprBlock/ExprWith that
//!    was stamped with the method-NAME token range instead of the body's span.

let FIXTURE = "{get_das_root()}/tests/ast/_lineinfo_ranges_fixture.das"


class RangeChecker : AstVisitor {
    src_lines : array<string>
    problems : array<string>

    def line_of(l : int) : string {
        return (l >= 1 && l <= length(src_lines)) ? src_lines[l - 1] : ""
    }

    def char_at(l, c : int) : string {
        let ln = line_of(l)
        return (c >= 0 && c < length(ln)) ? slice(ln, c, c + 1) : ""
    }

    def override preVisitExprLet(expr : ExprLet?) {
        if (expr.genFlags.generated) return
        let il = int(expr.atInit.last_line)
        let ilc = int(expr.atInit.last_column)
        if (il < 1) return
        if (ilc > length(line_of(il))) {
            problems |> push("atInit overshoots line end at {int(expr.at.line)}:{int(expr.at.column)} (..{il}:{ilc}, line is {length(line_of(il))})")
            return
        }
        // end-exclusive: last char of the span must be a real token char
        let last_ch = char_at(il, ilc - 1)
        if (empty(last_ch) || last_ch == " " || last_ch == ";") {
            problems |> push("atInit ends on '{last_ch}' at {il}:{ilc}")
        }
    }

    def override preVisitExprBlock(blk : ExprBlock?) {
        if (blk.genFlags.generated) return
        let l = int(blk.at.line)
        let ll = int(blk.at.last_line)
        let lc = int(blk.at.last_column)
        if (l < 1) return
        let opener = char_at(l, int(blk.at.column))
        let closer = char_at(ll, lc - 1)
        if (opener != "\{" || closer != "\}") {
            problems |> push("block at {l}:{int(blk.at.column)}..{ll}:{lc} spans '{opener}'..'{closer}', expected braces")
        }
    }
}


def check_fixture(t : T?; var checker : RangeChecker?) : bool {
    let text = fread(FIXTURE)
    if (empty(text)) {
        t |> success(false, "could not read {FIXTURE}")
        return false
    }
    var src_lines <- text |> split("\n")
    for (ln in src_lines) {
        if (ln |> ends_with("\r")) {
            ln = slice(ln, 0, length(ln) - 1)
        }
    }
    checker.src_lines <- src_lines
    var compiled = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = true
            cop.no_optimizations = true
            compile_file(FIXTURE, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    t |> success(false, "fixture failed to compile: {issues}")
                    return
                }
                compiled = true
                let thisMod = get_this_module(program)
                for_each_function(thisMod, "") $(func) {
                    // skip reified generics from required modules — only walk
                    // functions written in the fixture
                    if (func.flags.builtIn || func.flags.generated || func.at.fileInfo == null
                            || !(func.at.fileInfo.name |> ends_with("_lineinfo_ranges_fixture.das"))) return
                    make_visitor(*checker) $(adapter) {
                        visit(func, adapter)
                    }
                }
            }
        }
    }
    return compiled
}


[test]
def test_lineinfo_ranges(t : T?) {
    t |> run("let atInit and block spans match source") @(t : T?) {
        var checker = new RangeChecker()
        if (check_fixture(t, checker)) {
            for (p in checker.problems) {
                to_log(LOG_ERROR, "lineinfo: {p}\n")
            }
            t |> success(empty(checker.problems), "no lineinfo range violations (found {length(checker.problems)})")
        }
        unsafe {
            delete checker
        }
    }
}


//! The last line of a file may carry no newline. The lexer records where a line of code ends when
//! it consumes that newline, so a block closed by end of file used to end at the line before it.


class private SpanCollector : AstVisitor {
    spans : array<string>

    def override preVisitExprBlock(blk : ExprBlock?) {
        if (blk.genFlags.generated) return
        spans |> push("{int(blk.at.line)}:{int(blk.at.column)}..{int(blk.at.last_line)}:{int(blk.at.last_column)}")
    }
}


def private gen1_block_spans(path : string; var spans : array<string>) : bool {
    var parsed = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = false
            cop.no_optimizations = true
            parse_file(path, access, unsafe(addr(mg)), cop) $(ok; program; _issues) {
                if (!ok) return
                parsed = true
                var collector = new SpanCollector()
                for_each_function(get_this_module(program), "") $(var fn) {
                    if (fn.at.fileInfo == null || fn.flags.generated) return
                    make_visitor(*collector) $(adapter) {
                        visit(fn, adapter)
                    }
                }
                spans |> push_from(collector.spans)
                unsafe {
                    delete collector
                }
            }
        }
    }
    return parsed
}


[test]
def test_lineinfo_ranges_gen1_eof(t : T?) {
    t |> run("a v1 block closed by end of file ends at its last statement") @(t : T?) {
        let body = "def eof_block(n : int)\n    var acc = 0\n    for i in range(n)\n        acc += i\n    return acc"
        let nlFile = create_temp_file_result("lineinfo_gen1_nl", ".das")
        let eofFile = create_temp_file_result("lineinfo_gen1_eof", ".das")
        t |> success(nlFile is value && eofFile is value, "temporary fixtures created")
        if (!(nlFile is value && eofFile is value)) return
        let nlPath = unsafe(nlFile.value)
        let eofPath = unsafe(eofFile.value)
        t |> success(fwrite(nlPath, "{body}\n") && fwrite(eofPath, body), "temporary fixtures written")
        var nlSpans : array<string>
        var eofSpans : array<string>
        let nlOk = gen1_block_spans(nlPath, nlSpans)
        let eofOk = gen1_block_spans(eofPath, eofSpans)
        remove(nlPath)
        remove(eofPath)
        t |> success(nlOk && eofOk, "both fixtures parsed")
        if (!(nlOk && eofOk)) return
        t |> equal(join(eofSpans, ", "), join(nlSpans, ", "))
        t |> success(!empty(eofSpans) && eofSpans[0] |> ends_with("..5:14"),
            "outer block ends at the last statement ({empty(eofSpans) ? "none" : eofSpans[0]})")
    }
}
