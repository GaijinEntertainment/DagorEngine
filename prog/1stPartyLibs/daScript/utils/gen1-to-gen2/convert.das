options gen2
options indenting = 4

require daslib/ast
require daslib/rtti
require daslib/fio
require strings
require daslib/strings_boost
require offsets
require comments
require scan
require patch
require rules_make
require rules_stmt
require rules_decl
require ../common/own_functions.das


struct public ConvertOptions {
    //! What the caller asked for.
    v2 : bool = true            //!< gen2 syntax (braces, parens); false stops at gen1.5
    semicolon : bool = false    //!< keep the statement-terminating `;`
    verify_compile : bool = false   //!< compile the result as gen2 before handing it back
    project : string            //!< .das_project whose mount points the file's `require`s need
}


//! Offset of the file's `options gen2` statement, or -1.
//!
//! The compiler's own detector (`detectGen2Syntax`) scans the WHOLE file for the first one
//! outside a comment or a string, so a marker on line 12 counts exactly as much as one on
//! line 1 - and a rewriter that only looks at line 1 will contradict it.
def public gen2_marker(src : Source; lex : Lexical) : int {
    return find_token(src, lex, 0, "options gen2")
}


def private rule_gen2_header(src : Source; lex : Lexical; file : string; opt : ConvertOptions;
                             var set : EditSet) {
    if (file |> ends_with(".inc") || !opt.v2) return
    let header = opt.semicolon ? "options gen2;" : "options gen2"
    let marker = gen2_marker(src, lex)
    if (marker >= 0) {
        let line = line_at(src, marker)
        let text = text_of(src, line_begin(src, line), line_end(src, line)) |> strip
        if (text |> starts_with("options gen2 = false")) {
            set |> replace(line_begin(src, line), line_end(src, line), header, "gen2-header")
        }
        return
    }
    set |> insert(0, "{header}{src.eol}", "gen2-header")
}


class ConvertVisitor : AstVisitor {
    @do_not_delete src : Source const?
    @do_not_delete lex : Lexical const?
    edits : EditSet
    file : string
    opt = ConvertOptions()
    wrapped : table<int; bool>
    braced : table<int; bool>
    dotted : table<int; bool>

    def override preVisitExprCall(expr : ExprCall?) {
        if (opt.v2) {
            rule_piped_make((*src), (*lex), expr, edits)
        }
        let open = rule_move_wrapper((*src), (*lex), expr, edits)
        if (open >= 0) {
            wrapped |> insert(open, true)
        }
        if (opt.v2) {
            rule_type_function_call((*src), (*lex), expr, edits)
        }
    }

    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        rule_make_struct((*src), (*lex), expr, is_wrapped(expr.at), edits)
    }

    def override preVisitExprMakeArray(expr : ExprMakeArray?) {
        rule_make_array((*src), (*lex), expr, is_wrapped(expr.at), edits)
    }

    def override preVisitExprArrayComprehension(expr : ExprArrayComprehension?) {
        rule_comprehension((*src), (*lex), expr, edits)
    }

    def override preVisitExprCallMacro(expr : ExprCallMacro?) {
        if (opt.v2) {
            rule_call_macro_parens((*src), (*lex), expr, edits)
        }
        for (arg in expr.arguments) {
            visit_owned(arg)
        }
    }

    def visit_owned(expr : ExpressionPtr) {
        var sub = new ConvertVisitor(src = src, lex = lex, file = file, opt = opt)
        make_visitor(*sub) $(adapter) {
            visit(expr, adapter)
        }
        edits.edits |> push_from(sub.edits.edits)
        edits.refused |> push_from(sub.edits.refused)
        unsafe {
            delete sub
        }
    }

    def override preVisitExprBlock(expr : ExprBlock?) {
        if (!opt.v2 || expr.genFlags.generated) return
        let key = span_begin((*src), expr.at)
        if (key_exists(braced, key)) return
        braced |> insert(key, true)
        rule_block_braces((*src), (*lex), expr, edits)
        for (arg in expr.arguments) {
            rule_argument_annotation((*src), (*lex), arg.at, edits)
        }
    }

    def override preVisitExprCast(expr : ExprCast?) {
        if (!opt.v2) return
        rule_cast_parens((*src), (*lex), expr, edits)
    }

    def override preVisitExprReturn(expr : ExprReturn?) {
        if (!opt.v2) return
        rule_return_tuple((*src), (*lex), expr, edits)
    }

    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) {
        dot_constant(expr.at, expr.genFlags.generated)
    }

    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) {
        dot_constant(expr.at, expr.genFlags.generated)
    }

    def dot_constant(at : LineInfo; generated : bool) {
        if (!opt.v2) return
        let key = span_begin((*src), at)
        if (key_exists(dotted, key)) return
        dotted |> insert(key, true)
        rule_enum_dot((*src), (*lex), at, generated, edits)
    }

    def override preVisitExprLet(expr : ExprLet?) {
        if (!opt.v2) return
        rule_let_tuple((*src), (*lex), expr, edits)
        rule_multiline_init((*src), (*lex), expr, edits)
    }

    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
        if (!opt.v2) return
        rule_typeinfo((*src), (*lex), expr, edits)
    }

    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) {
        if (!opt.v2) return
        rule_if_parens((*src), (*lex), expr, edits)
        glue_else(expr)
    }

    def override preVisitExprWhile(expr : ExprWhile?) {
        if (!opt.v2) return
        rule_while_parens((*src), (*lex), expr, edits)
    }

    def override preVisitExprWith(expr : ExprWith?) {
        if (!opt.v2) return
        rule_with_parens((*src), (*lex), expr, edits)
    }

    def override preVisitExprFor(expr : ExprFor?) {
        if (!opt.v2) return
        rule_for_parens((*src), (*lex), expr, edits)
    }

    def override preVisitExprTryCatch(expr : ExprTryCatch?) {
        if (!opt.v2 || expr.try_block == null || expr.catch_block == null) return
        rule_glue_keyword((*src), block_close_at((*src), (*lex), expr.try_block.at),
            span_begin((*src), expr.catch_block.at), "recover", is_braced((*src), expr.catch_block.at), edits)
    }

    def glue_else(expr : ExprIfThenElse?) {
        if (expr.if_true == null || expr.if_false == null) return
        rule_glue_keyword((*src), block_close_at((*src), (*lex), expr.if_true.at),
            span_begin((*src), expr.if_false.at), "else", is_braced((*src), expr.if_false.at), edits)
    }

    def is_wrapped(at : LineInfo) : bool {
        return wrapped ?[span_begin((*src), at)] ?? false
    }
}


def private same_file(at : LineInfo; file : string) : bool {
    if (at.fileInfo == null) return false
    return normalize(string(at.fileInfo.name)) |> replace("\\", "/") == normalize(file) |> replace("\\", "/")
}


def private collect_declaration_edits(src : Source; lex : Lexical; var program : ProgramPtr;
                                      file : string; var set : EditSet) {
    for_each_structure(get_this_module(program)) $(var st) {
        if (!same_file(st.at, file)) return
        rule_decl_braces(src, lex, span_begin(src, st.at), set, true)
    }
    for_each_enumeration(get_this_module(program)) $(var en) {
        if (!same_file(en.at, file)) return
        rule_decl_braces(src, lex, span_begin(src, en.at), set, true)
    }
    var globalBlocks : table<int; bool>
    for_each_global(get_this_module(program)) $(var gvar) {
        if (!same_file(gvar.at, file)) return
        let hdr = global_block_header(src, lex, span_begin(src, gvar.at))
        if (hdr >= 0 && !key_exists(globalBlocks, hdr)) {
            globalBlocks |> insert(hdr, true)
            rule_decl_braces(src, lex, hdr, set)
        }
    }
    var typedefBlocks : table<int; bool>
    for_each_typedef(get_this_module(program)) $(_name; var td) {
        if (!same_file(td.at, file)) return
        let at = span_begin(src, td.at)
        rule_decl_braces(src, lex, at, set)
        let hdr = typedef_block_header(src, lex, at)
        if (hdr >= 0 && !key_exists(typedefBlocks, hdr)) {
            typedefBlocks |> insert(hdr, true)
            rule_typedef_block(src, lex, hdr, set)
        }
    }
}


def public collect_edits(src : Source; lex : Lexical; var program : ProgramPtr; file : string;
                         opt : ConvertOptions; var set : EditSet) {
    //! Records every conversion for one file. Order of calls does not matter: edits are sorted
    //! by offset when applied, and two rules that would fight over one range are refused there.
    rule_gen2_header(src, lex, file, opt, set)

    var v = new ConvertVisitor()
    unsafe {
        v.src = addr(src)
        v.lex = addr(lex)
    }
    v.file = file
    v.opt = opt
    for_each_own_function(program, file) $(fn) {
        make_visitor(*v) $(adapter) {
            visit(fn, adapter)
        }
    }
    for_each_global(get_this_module(program)) $(var gvar) {
        if (gvar.init == null || !same_file(gvar.at, file)) return
        make_visitor(*v) $(adapter) {
            visit(gvar.init, adapter)
        }
    }
    if (opt.v2) {
        collect_declaration_edits(src, lex, program, file, set)
    }
    set.edits |> push_from(v.edits.edits)
    set.refused |> push_from(v.edits.refused)
    unsafe {
        delete v
    }
}


struct public FileResult {
    //! What one file's conversion produced: the text, plus why it was left alone.
    file : string
    changed : bool
    skipped : string
    failed : string
    refused : array<string>
    output : string
}


def public compiles_as_gen2(near : string; text : string; var issues_out : string&; project : string = "") : bool {
    //! Compiles converted text as gen2 — the check that the rewrite produced real daslang.
    //!
    //! It goes through a file beside the original rather than through `compile`, because the
    //! text-only entry gets no module group and would report daslib's own generics as missing.
    //! The scratch file is removed either way.
    let probe = "{dir_name(near)}/.gen2_verify_{base_name(near)}"
    if (!fwrite(probe, text)) {
        issues_out = "could not write {probe}"
        return false
    }
    var ok_out = false
    var inscope access <- make_file_access(project)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = true
            cop.no_optimizations = true
            compile_file(probe, access, unsafe(addr(mg)), cop) $(ok; _program; issues) {
                ok_out = ok
                issues_out = string(issues)
            }
        }
    }
    remove(probe)
    return ok_out
}


def public convert_file(file : string; opt : ConvertOptions) : FileResult {
    //! Converts one file and returns the result; writes nothing.
    var res = FileResult(file = file)
    let text = fread(file)
    if (empty(text)) {
        res.skipped = "empty or unreadable"
        return <- res
    }
    let src = make_source(text)
    let lex <- scan_lexical(src)
    let marker = gen2_marker(src, lex)
    if (marker >= 0) {
        let mline = line_at(src, marker)
        if (!(text_of(src, line_begin(src, mline), line_end(src, mline)) |> strip
                |> starts_with("options gen2 = false"))) {
            res.skipped = "already gen2"
            res.output = text
            return <- res
        }
    }
    if (text |> find("%spoof_template~") >= 0) {
        res.skipped = "contains spoof templates"
        res.output = text
        return <- res
    }
    var set : EditSet
    var collected = false
    var inscope access <- make_file_access(opt.project)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = false
            cop.no_optimizations = true
            parse_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    res.failed = empty(string(issues)) ? "parse failed" : string(issues)
                    return
                }
                collect_edits(src, lex, program, file, opt, set)
                collected = true
            }
        }
    }
    if (!collected) {
        if (empty(res.failed)) {
            res.failed = "parse failed"
        }
        return <- res
    }
    res.output = apply(src, lex, set)
    res.refused <- set.refused
    let conserved = check_comments_preserved(text, res.output)
    if (!conserved.ok) {
        res.failed = "comment conservation: {conserved.detail}"
        res.output = text
        return <- res
    }
    res.changed = res.output != text
    if (opt.verify_compile && res.changed) {
        var issues = ""
        if (!compiles_as_gen2(file, res.output, issues, opt.project)) {
            if (issues |> find("missing prerequisite") < 0 && issues |> find("not found") < 0) {
                res.failed = "converted output does not compile: {issues}"
                res.output = text
            }
        }
    }
    return <- res
}
