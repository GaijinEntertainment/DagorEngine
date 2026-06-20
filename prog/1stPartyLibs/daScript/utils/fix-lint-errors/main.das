options gen2

require daslib/ast_boost
require daslib/fio
require daslib/strings_boost
require daslib/clargs
require strings
require math

// utils/fix-lint-errors --
//
// Auto-fixes mechanical lint rules. Walks the AST of each input file via
// `compile_file`, identifies fixable patterns, and rewrites the source.
//
// Rules supported:
//   STYLE005  if (cond) { <terminator> }   ->  if (cond) <terminator>
//   STYLE017  if (c) return true; else return false  ->  return c / !c
//   STYLE018  b == true / b != false       ->  b ; b == false / b != true -> !b
//   PERF013   a += 1 / a -= 1              ->  a++ / a--
//   PERF015   a > b ? a : b                ->  max(a, b) (and min variants)
//   PERF016   x < 0 ? -x : x               ->  abs(x) (and other ternary abs)
//   PERF017   empty(x) / != 0        ->  empty(x) / !empty(x)
//
// Future rules (PERF014) layer on the same Edit pipeline — each visitor
// override pushes an Edit; apply_edits writes them back in reverse-position
// order, with an overlap filter to drop edits subsumed by a larger one.
//
// Out of scope: rules that need cross-statement restructuring (STYLE016
// guard combining, STYLE011/012/013 init patterns, PERF018 for-each).

[CommandLineArgs]
struct Config {
    @clarg_short = "q"
    @clarg_doc = "Suppress per-file PASS/PROCESS lines"
    quiet : bool

    @clarg_doc = "Print edits but don't write files"
    dry_run : bool

    @clarg_doc = "Skip post-rewrite compile-verify auto-revert (for debugging broken rewrites)"
    no_verify : bool

    @clarg_short = "?"
    @clarg_doc = "Show this help and exit"
    help : bool
}

struct SourceEdit {
    start_line : int    // 1-based, inclusive
    start_col : int     // 1-based, inclusive
    end_line : int      // 1-based, inclusive
    end_col : int       // 1-based, exclusive (one past last char)
    rule : string       // e.g. "STYLE005"
}

class FixerVisitor : AstVisitor {
    //! Walks a compiled program collecting SourceEdit records. Each edit names
    //! a span to replace; the replacement text is computed at apply time from
    //! the inner expression's `at`. Per-file edits are sorted descending in
    //! `apply_edits` and applied in that order so earlier offsets stay valid.
    edits : array<SourceEdit>
    replacements : array<string>
    @do_not_delete current_function : Function?
    in_template : bool = false
    in_closure : int = 0
    has_math_require : bool = false  // gates PERF015/PERF016 (rewrites use math::min/max/abs)
    // The file we're currently editing. `visit_with_generics` walks generic
    // instantiations whose `.at` may point to a DIFFERENT file (e.g. builtin
    // instantiated from us). Gate every edit on filename equality to avoid
    // emitting edits for cross-file source positions.
    target_filename : string

    def FixerVisitor() {
        pass
    }

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = fn
        // Skip `flags.generated` — synthesized functions from struct
        // annotations like `[CommandLineArgs]` carry the struct's LineInfo
        // throughout their body, so a `empty(args)` check inside the
        // generated parse_args would rewrite to a span starting at the struct
        // definition (massive corruption observed on utils/daspkg/commands.das).
        // `isTemplate`/`macroFunction` bodies are user-written daslang code —
        // their AST `.at` points at real source; post-rewrite `try_compile`
        // verification catches any edge case that slips through.
        in_template = fn.flags.generated
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        current_function = null
        in_template = false
        return <- fn
    }

    def override preVisitExprBlock(blk : ExprBlock?) : void {
        if (blk.blockFlags.isClosure) {
            in_closure++
        }
        // STYLE017(b): adjacent `if (cond) return b1` / `return b2` (b1 != b2).
        check_style017b(blk)
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            in_closure--
        }
        return <- blk
    }

    // --- helpers ---

    def extract_block_inner(blk_at : LineInfo) : string {
        //! Returns the source bytes between an ExprBlock's `{` and `}` (exclusive
        //! on both ends), trimmed. For a single-statement block this yields the
        //! statement source — keyword + subexpr + any trailing `;` — which we use
        //! verbatim as the STYLE005 replacement.
        //!
        //! Why not `extract_text(inner_stmt.at)`? Because terminator LineInfos
        //! cover only the keyword (e.g. ExprReturn.at spans "return", not the
        //! subexpr). Going via the block bounds captures the full statement
        //! whatever its shape.
        //!
        //! Coordinate convention: daslang LineInfo columns are 0-based. `column`
        //! is the 0-indexed start; `last_column` is one past the last char
        //! (exclusive 0-indexed end). So `{` lives at index `column`; the byte
        //! immediately after `{` is at `column + 1`. The `}` is at
        //! `last_column - 1`; the slice we want ends at `last_column - 1`.
        return "" if (blk_at.fileInfo == null || blk_at.line == 0u)
        let start_line = int(blk_at.line)
        let end_line = int(blk_at.last_line)
        let inner_start = int(blk_at.column) + 1
        let inner_end = int(blk_at.last_column) - 1
        var result = ""
        if (start_line == end_line) {
            get_file_source_line(blk_at.fileInfo, blk_at.line) $(line) {
                let lo = max(0, inner_start)
                let hi = min(length(line), inner_end)
                if (hi > lo) {
                    result := slice(line, lo, hi)
                }
            }
            return strip(result)
        }
        for (ln in range(start_line, end_line + 1)) {
            get_file_source_line(blk_at.fileInfo, uint(ln)) $(line) {
                if (ln == start_line) {
                    let lo = max(0, inner_start)
                    if (lo < length(line)) {
                        result := slice(line, lo)
                    }
                } elif (ln == end_line) {
                    let hi = min(length(line), inner_end)
                    if (hi > 0) {
                        result := "{result}\n{slice(line, 0, hi)}"
                    }
                } else {
                    result := "{result}\n{line}"
                }
            }
        }
        return strip(result)
    }

    def extract_source(at : LineInfo) : string {
        //! Extracts source bytes covered by `at`. Multi-line aware. Columns
        //! 0-based, last_column exclusive.
        return "" if (at.fileInfo == null || at.line == 0u)
        let start_line = int(at.line)
        let end_line = int(at.last_line)
        let s_col = int(at.column)
        let e_col = int(at.last_column)
        var result = ""
        if (start_line == end_line) {
            get_file_source_line(at.fileInfo, at.line) $(line) {
                let lo = max(0, s_col)
                let hi = min(length(line), e_col)
                if (hi > lo) {
                    result := slice(line, lo, hi)
                }
            }
            return result
        }
        for (ln in range(start_line, end_line + 1)) {
            get_file_source_line(at.fileInfo, uint(ln)) $(line) {
                if (ln == start_line) {
                    let lo = max(0, s_col)
                    if (lo < length(line)) {
                        result := slice(line, lo)
                    }
                } elif (ln == end_line) {
                    let hi = min(length(line), e_col)
                    if (hi > 0) {
                        result := "{result}\n{slice(line, 0, hi)}"
                    }
                } else {
                    result := "{result}\n{line}"
                }
            }
        }
        return result
    }

    def peel_ref2value(e : Expression? const) : Expression? const {
        //! Peels a single ExprRef2Value wrapper (typer artifact). Safe — does not
        //! peel ExprPtr2Ref (user-visible `*foo` deref).
        if (e != null && e is ExprRef2Value) return (e as ExprRef2Value).subexpr
        return e
    }

    def is_length_call(e : Expression? const) : bool {
        //! True for length(string) / length(das_string) / length(array) /
        //! length(table). Excludes math::length(float2/3/4).
        return false if (e == null || !(e is ExprCall))
        let call = e as ExprCall
        return false if (call.func == null || call.func.name != "length" || call.func._module == null)
        let modname = string(call.func._module.name)
        return modname == "strings" || modname == "$" || modname == "builtin"
    }

    def source_span_of(e : Expression? const;
                       var sl : int&; var sc : int&;
                       var el : int&; var ec : int&) : void {
        //! Returns the source bounds for an expression. For ExprCall, this is
        //! `atEnclosure` (covers `name(args)`); for everything else, `at`. Note
        //! that the typer may have canonicalized operand order so the AST left
        //! is not necessarily the source left — callers must take min/max
        //! across operands.
        if (e == null) {
            sl = 0; sc = 0; el = 0; ec = 0
            return
        }
        if (e is ExprCall) {
            let c = e as ExprCall
            sl = int(c.atEnclosure.line)
            sc = int(c.atEnclosure.column)
            el = int(c.atEnclosure.last_line)
            ec = int(c.atEnclosure.last_column)
            return
        }
        sl = int(e.at.line)
        sc = int(e.at.column)
        el = int(e.at.last_line)
        ec = int(e.at.last_column)
    }

    def rightmost_pos_of(e : Expression? const; var hi_line : int&; var hi_col : int&) : void {
        //! Recursive rightmost source position. For explicit field syntax
        //! `obj.name`, ExprField.at covers just `.` and we add length(name).
        //! For IMPLICIT self-field access `name` (inside a class method),
        //! the parser synthesizes ExprField(value=ExprVar(self), name=...) but
        //! the AST node's .at already covers the field name's source span.
        //! Distinguish by comparing field.at to value.at — if they're the same,
        //! it's the implicit form.
        if (e == null) {
            hi_line = 0; hi_col = 0
            return
        }
        if (e is ExprField) {
            let fld = e as ExprField
            if (is_implicit_self_field(fld.at, fld.value)) {
                hi_line = int(fld.at.last_line)
                hi_col = int(fld.at.last_column)
                return
            }
            hi_line = int(fld.at.last_line)
            hi_col = int(fld.at.last_column) + length(fld.name)
            return
        }
        if (e is ExprSafeField) {
            let fld = e as ExprSafeField
            if (is_implicit_self_field(fld.at, fld.value)) {
                hi_line = int(fld.at.last_line)
                hi_col = int(fld.at.last_column)
                return
            }
            hi_line = int(fld.at.last_line)
            hi_col = int(fld.at.last_column) + length(fld.name)
            return
        }
        if (e is ExprAt) {
            let ea = e as ExprAt
            if (ea.index != null) {
                hi_line = int(ea.index.at.last_line)
                hi_col = int(ea.index.at.last_column) + 1
                return
            }
        }
        if (e is ExprSafeAt) {
            let ea = e as ExprSafeAt
            if (ea.index != null) {
                hi_line = int(ea.index.at.last_line)
                hi_col = int(ea.index.at.last_column) + 1
                return
            }
        }
        if (e is ExprCall) {
            let c = e as ExprCall
            // For paren-form calls, atEnclosure is the source span. For pipe
            // form, atEnclosure may be unset or at the call name; fall back to
            // the maximum of (atEnclosure, call.at, args' rightmost).
            var line_out = int(c.atEnclosure.last_line)
            var col_out = int(c.atEnclosure.last_column)
            let cname = string(c.name)
            // Property-method form `obj.foo` parses to ExprCall with name=`.foo`.
            // call.at covers just `.foo` and args[0] precedes it. The rightmost
            // is end-of-name = at.last_column.
            if (line_out == 0
                    || (line_out == int(c.at.line) && col_out == int(c.at.last_column))) {
                line_out = int(c.at.last_line)
                col_out = int(c.at.last_column)
            }
            for (a in c.arguments) {
                var a_l = 0; var a_c = 0
                rightmost_pos_of(a, a_l, a_c)
                if (a_l != 0 && (a_l > line_out || (a_l == line_out && a_c > col_out))) {
                    line_out = a_l
                    col_out = a_c
                }
            }
            // Property calls have name starting with backtick (`.foo) in some
            // forms; conservative: ensure we cover the full name length.
            if (!empty(cname) && cname |> starts_with(".")) {
                let name_end_col = int(c.at.column) + length(cname)
                if (name_end_col > col_out) {
                    col_out = name_end_col
                }
            }
            hi_line = line_out
            hi_col = col_out
            return
        }
        if (e is ExprOp1) {
            // `-x` — the right end lives in subexpr (the operand follows `-`).
            let op1 = e as ExprOp1
            if (op1.subexpr != null) {
                rightmost_pos_of(op1.subexpr, hi_line, hi_col)
                return
            }
        }
        if (e is ExprRef2Value) {
            rightmost_pos_of((e as ExprRef2Value).subexpr, hi_line, hi_col)
            return
        }
        if (e is ExprPtr2Ref) {
            rightmost_pos_of((e as ExprPtr2Ref).subexpr, hi_line, hi_col)
            return
        }
        if (e is ExprOp2) {
            rightmost_pos_of((e as ExprOp2).right, hi_line, hi_col)
            return
        }
        if (e is ExprOp3) {
            rightmost_pos_of((e as ExprOp3).right, hi_line, hi_col)
            return
        }
        hi_line = int(e.at.last_line)
        hi_col = int(e.at.last_column)
    }

    def lo_min(var sl : int&; var sc : int&; o_sl : int; o_sc : int) : void {
        if (o_sl == 0) return
        if (sl == 0 || o_sl < sl || (o_sl == sl && o_sc < sc)) {
            sl = o_sl
            sc = o_sc
        }
    }

    def hi_max(var el : int&; var ec : int&; o_el : int; o_ec : int) : void {
        if (o_el == 0) return
        if (el == 0 || o_el > el || (o_el == el && o_ec > ec)) {
            el = o_el
            ec = o_ec
        }
    }

    def extract_expr_source(e : Expression? const; fi : FileInfo? const) : string {
        //! Convenience wrapper: leftmost_pos + rightmost_pos + extract_source_at.
        //! Falls back to `e.at` span when chain helpers return 0 (unknown shape).
        //! Self-validates the extracted text — if it starts with a punctuation
        //! character (`.`, `[`, `)`, etc.) the chain-helper got an inner-node
        //! position and the extraction is wrong. Returns "" in that case so the
        //! caller bails rather than emitting a corrupted edit.
        return "" if (e == null || fi == null)
        var lo_l = 0; var lo_c = 0
        var hi_l = 0; var hi_c = 0
        leftmost_pos_of(e, lo_l, lo_c)
        rightmost_pos_of(e, hi_l, hi_c)
        if (lo_l == 0) {
            lo_l = int(e.at.line)
            lo_c = int(e.at.column)
        }
        if (hi_l == 0) {
            hi_l = int(e.at.last_line)
            hi_c = int(e.at.last_column)
        }
        let text = extract_source_at(lo_l, lo_c, hi_l, hi_c, fi)
        return "" if (empty(text))
        let c0 = first_character(text)
        // Identifier start, digit, `!`, `-`, `(`, `*`, `"`, `'` are all legitimate
        // expression openers. Anything else (especially `.`, `]`, `,`) means we
        // landed mid-token.
        if (c0 == '.' || c0 == ']' || c0 == ',' || c0 == ';' || c0 == ')' || c0 == '}') return ""
        return text
    }

    def is_const_zero(e : Expression? const) : bool {
        let inner = peel_ref2value(e)
        return false if (inner == null)
        if (inner is ExprConstInt) return (inner as ExprConstInt).value == 0
        if (inner is ExprConstUInt) return (inner as ExprConstUInt).value == 0u
        if (inner is ExprConstInt64) return (inner as ExprConstInt64).value == 0l
        if (inner is ExprConstUInt64) return (inner as ExprConstUInt64).value == 0ul
        if (inner is ExprConstFloat) return (inner as ExprConstFloat).value == 0.0f
        if (inner is ExprConstDouble) return (inner as ExprConstDouble).value == 0.0lf
        return false
    }

    def is_neg_of(maybe_neg : Expression? const; ref : Expression? const) : bool {
        //! True when `maybe_neg` is `-ref` structurally — peel ExprRef2Value, then
        //! the outer must be ExprOp1 with op `-` whose subexpr describes the same
        //! as `ref`.
        let mn = peel_ref2value(maybe_neg)
        let rf = peel_ref2value(ref)
        return false if (mn == null || rf == null || !(mn is ExprOp1))
        let op1 = mn as ExprOp1
        return false if (op1.op != "-" || op1.subexpr == null)
        return describe(peel_ref2value(op1.subexpr)) == describe(rf)
    }

    def expr_describe_equal(a : Expression? const; b : Expression? const) : bool {
        let aa = peel_ref2value(a)
        let bb = peel_ref2value(b)
        return false if (aa == null || bb == null)
        return describe(aa) == describe(bb)
    }

    def is_simple_subject(e : Expression? const) : bool {
        //! True if `e` has a clean, known source span — both leftmost_pos_of
        //! and rightmost_pos_of can derive exact bounds from the AST without
        //! needing to read invisible-in-AST tokens (parens, deref `*`).
        //! Chain shapes walk recursively (ExprField → value, ExprAt → subexpr).
        //! ExprRef2Value is a typer artifact (invisible in source) and is
        //! peeled transparently. ExprPtr2Ref is NOT — parens around `(*pv)`
        //! aren't in the AST so leftmost lands on the `*`, missing `(`.
        //! ExprCall with a backticked property name (`.foo` form) is also
        //! NOT safe — the call's source extent depends on hidden arg ordering.
        return false if (e == null)
        if (e is ExprRef2Value) return is_simple_subject((e as ExprRef2Value).subexpr)
        if (e is ExprConstString || e is ExprConstInt || e is ExprConstUInt
                || e is ExprConstBool || e is ExprConstFloat || e is ExprConstDouble
                || e is ExprVar) return true
        if (e is ExprCall) {
            let c = e as ExprCall
            // Property-method form: name starts with backtick + dot.
            let nm = string(c.name)
            return empty(nm) || !(nm |> starts_with(".") || nm |> starts_with("`"))
        }
        if (e is ExprField) return is_simple_subject((e as ExprField).value)
        if (e is ExprSafeField) return is_simple_subject((e as ExprSafeField).value)
        if (e is ExprAt) return is_simple_subject((e as ExprAt).subexpr)
        if (e is ExprSafeAt) return is_simple_subject((e as ExprSafeAt).subexpr)
        return false
    }

    def cond_uses_property_call(e : Expression? const) : bool {
        //! Walks an expression looking for a property-method ExprCall. The
        //! property `.foo` form has hidden arg positions that make span
        //! computation unreliable — STYLE017 bails on conds containing one.
        return false if (e == null)
        if (e is ExprCall) {
            let c = e as ExprCall
            let nm = string(c.name)
            return true if (!empty(nm) && (nm |> starts_with(".") || nm |> starts_with("`")))
        }
        if (e is ExprOp1) return cond_uses_property_call((e as ExprOp1).subexpr)
        if (e is ExprOp2) return (cond_uses_property_call((e as ExprOp2).left)
                || cond_uses_property_call((e as ExprOp2).right))
        if (e is ExprField) return cond_uses_property_call((e as ExprField).value)
        if (e is ExprSafeField) return cond_uses_property_call((e as ExprSafeField).value)
        if (e is ExprPtr2Ref) return cond_uses_property_call((e as ExprPtr2Ref).subexpr)
        if (e is ExprRef2Value) return cond_uses_property_call((e as ExprRef2Value).subexpr)
        return false
    }

    def is_implicit_self_field(field_at : LineInfo; value : Expression? const) : bool {
        //! True if the ExprField is a synthesized implicit-self access whose
        //! `.at` already covers the field name source span. Signal: the inner
        //! ExprVar(self) shares the field's `.at`.
        return false if (value == null || !(value is ExprVar))
        let v = value as ExprVar
        return (field_at.line == v.at.line
                && field_at.column == v.at.column
                && field_at.last_line == v.at.last_line
                && field_at.last_column == v.at.last_column)
    }

    def leftmost_pos_of(e : Expression? const; var lo_line : int&; var lo_col : int&) : void {
        //! Recursive leftmost source position. ExprField.at points at the `.`
        //! separator and ExprAt.at at the `[`, so the user-source start lives
        //! in the chain root. Auto-deref wrappers (ExprPtr2Ref, ExprRef2Value)
        //! introduced by the typer have unreliable `.at` — recurse past them.
        if (e == null) {
            lo_line = 0; lo_col = 0
            return
        }
        if (e is ExprField) {
            let fld = e as ExprField
            // Implicit-self: synthesized `self.name`, no real `.` in source —
            // the field's own .at covers the name source.
            if (is_implicit_self_field(fld.at, fld.value)) {
                lo_line = int(fld.at.line)
                lo_col = int(fld.at.column)
                return
            }
            leftmost_pos_of(fld.value, lo_line, lo_col)
            return
        }
        if (e is ExprSafeField) {
            let fld = e as ExprSafeField
            if (is_implicit_self_field(fld.at, fld.value)) {
                lo_line = int(fld.at.line)
                lo_col = int(fld.at.column)
                return
            }
            leftmost_pos_of(fld.value, lo_line, lo_col)
            return
        }
        if (e is ExprAt) {
            leftmost_pos_of((e as ExprAt).subexpr, lo_line, lo_col)
            return
        }
        if (e is ExprSafeAt) {
            leftmost_pos_of((e as ExprSafeAt).subexpr, lo_line, lo_col)
            return
        }
        if (e is ExprPtr2Ref) {
            leftmost_pos_of((e as ExprPtr2Ref).subexpr, lo_line, lo_col)
            return
        }
        if (e is ExprRef2Value) {
            leftmost_pos_of((e as ExprRef2Value).subexpr, lo_line, lo_col)
            return
        }
        if (e is ExprOp2) {
            // Binary ops: leftmost is the left operand's leftmost.
            leftmost_pos_of((e as ExprOp2).left, lo_line, lo_col)
            return
        }
        if (e is ExprOp3) {
            // Ternary `cond ? t : f`: leftmost is cond.
            leftmost_pos_of((e as ExprOp3).subexpr, lo_line, lo_col)
            return
        }
        if (e is ExprOp1) {
            // Unary like `-x` / `!x`: leftmost is the operator (ExprOp1.at).
            lo_line = int(e.at.line)
            lo_col = int(e.at.column)
            return
        }
        if (e is ExprCall) {
            // Pipe form `arg |> name` desugars to ExprCall whose name lives AT
            // the call name (later in source) but arg[0] is BEFORE it. Take the
            // minimum of call.at and arg[0]'s leftmost.
            let c = e as ExprCall
            let call_l = int(c.at.line)
            let call_c = int(c.at.column)
            if (!empty(c.arguments)) {
                var a_l = 0; var a_c = 0
                leftmost_pos_of(c.arguments[0], a_l, a_c)
                if (a_l != 0 && (a_l < call_l || (a_l == call_l && a_c < call_c))) {
                    lo_line = a_l
                    lo_col = a_c
                    return
                }
            }
            lo_line = call_l
            lo_col = call_c
            return
        }
        lo_line = int(e.at.line)
        lo_col = int(e.at.column)
    }

    def const_int_value(e : Expression? const; var v : int&) : bool {
        let inner = peel_ref2value(e)
        return false if (inner == null)
        if (inner is ExprConstInt) {
            v = (inner as ExprConstInt).value
            return true
        }
        if (inner is ExprConstUInt) {
            v = int((inner as ExprConstUInt).value)
            return true
        }
        return false
    }

    def is_const_pm_one(e : Expression? const; var sign_out : int&) : bool {
        //! True if `e` is the integer/float constant +1 or -1. Sign written to
        //! `sign_out`. Mirrors PERF013's `is_const_one` + `is_const_neg_one`.
        let inner = peel_ref2value(e)
        return false if (inner == null)
        if (inner is ExprConstInt) {
            let v = (inner as ExprConstInt).value
            if (v == 1) {
                sign_out = 1
                return true
            }
            if (v == -1) {
                sign_out = -1
                return true
            }
            return false
        }
        if (inner is ExprConstUInt) {
            if ((inner as ExprConstUInt).value == 1u) {
                sign_out = 1
                return true
            }
            return false
        }
        if (inner is ExprConstInt64) {
            let v = (inner as ExprConstInt64).value
            if (v == 1l) {
                sign_out = 1
                return true
            }
            if (v == -1l) {
                sign_out = -1
                return true
            }
            return false
        }
        if (inner is ExprConstUInt64) {
            if ((inner as ExprConstUInt64).value == 1ul) {
                sign_out = 1
                return true
            }
            return false
        }
        if (inner is ExprConstFloat) {
            let v = (inner as ExprConstFloat).value
            if (v == 1.0f) {
                sign_out = 1
                return true
            }
            if (v == -1.0f) {
                sign_out = -1
                return true
            }
            return false
        }
        if (inner is ExprConstDouble) {
            let v = (inner as ExprConstDouble).value
            if (v == 1.0lf) {
                sign_out = 1
                return true
            }
            if (v == -1.0lf) {
                sign_out = -1
                return true
            }
            return false
        }
        return false
    }

    def is_workhorse_numeric(t : TypeDecl? const) : bool {
        //! Six numeric workhorse scalars (int, uint, int64, uint64, float, double).
        //! Mirrors perf_lint's gate so the fixer matches what the lint flags.
        return false if (t == null || t.baseType == Type.tFixedArray)
        let bt = t.baseType
        return (bt == Type.tInt || bt == Type.tUInt
                || bt == Type.tInt64 || bt == Type.tUInt64
                || bt == Type.tFloat || bt == Type.tDouble)
    }

    def push_edit(at : LineInfo; replacement : string; rule : string) : void {
        // Gate on filename — visit_with_generics may surface AST from another
        // file (e.g. builtin) and we must not emit edits with those coordinates.
        return if (at.fileInfo == null)
        let fname = string(at.fileInfo.name)
        return if (!empty(target_filename) && fname != target_filename)
        push_edit_span(int(at.line), int(at.column),
                             int(at.last_line), int(at.last_column),
                             replacement, rule)
    }

    def push_edit_span(start_line : int; start_col : int;
                       end_line : int; end_col : int;
                       replacement : string; rule : string) : void {
        let e = SourceEdit(
            start_line = start_line,
            start_col = start_col,
            end_line = end_line,
            end_col = end_col,
            rule = rule)
        edits |> push(e)
        replacements |> push(replacement)
    }

    def in_target_file(at : LineInfo) : bool {
        //! Gate that callers using push_edit_span (with explicit ints) must
        //! check first — otherwise out-of-file source coordinates leak through.
        return false if (at.fileInfo == null)
        return true if (empty(target_filename))
        return at.fileInfo.name == target_filename
    }

    def source_line_starts_with_if(at : LineInfo) : bool {
        //! True iff the source line at `at.line` starts (after leading
        //! whitespace, optional `}`) with `if` or `elif`. Distinguishes
        //! real if-statements from postfix-desugared `return X if (cond)`
        //! synthetic ifs — those share LineInfo with the original return.
        return false if (at.fileInfo == null || at.line == 0u)
        var ok = false
        get_file_source_line(at.fileInfo, at.line) $(line) {
            var trimmed = line |> strip_left()
            // Allow `} elif` and `} else if` (elif chain continuation).
            if (trimmed |> starts_with("}")) {
                trimmed = slice(trimmed, 1) |> strip_left()
            }
            ok = (trimmed |> starts_with("if ")
                || trimmed |> starts_with("if(")
                || trimmed |> starts_with("elif ")
                || trimmed |> starts_with("elif("))
        }
        return ok
    }

    def expr_contains_rtti_check(e : Expression? const) : bool {
        //! Detects `x.__rtti == "..."` form — the typer's lowering of `x is T`.
        //! STYLE017(b) can't reliably compute the source rightmost across that
        //! lowering (the synthesized `ExprConstString` carries an `at` that no
        //! longer covers the original type-name source span), so we bail.
        let ee = peel_ref2value(e)
        return false if (ee == null)
        if (ee is ExprOp2) {
            let op = ee as ExprOp2
            if (op.op == "==" || op.op == "!=") {
                let lhs = peel_ref2value(op.left)
                if (lhs != null && lhs is ExprField) {
                    let fld = lhs as ExprField
                    if (fld.name == "__rtti") return true
                }
            }
            return expr_contains_rtti_check(op.left) || expr_contains_rtti_check(op.right)
        }
        if (ee is ExprOp1) return expr_contains_rtti_check((ee as ExprOp1).subexpr)
        if (ee is ExprOp3) {
            let op3 = ee as ExprOp3
            return (expr_contains_rtti_check(op3.subexpr)
                || expr_contains_rtti_check(op3.left)
                || expr_contains_rtti_check(op3.right))
        }
        return false
    }

    // --- STYLE005 / STYLE017: ExprIfThenElse ---

    def override preVisitExprIfThenElse(ifte : ExprIfThenElse?) : void {
        // STYLE017(a) is intentionally NOT auto-fixed: match blocks parse as a
        // chain of ExprIfThenElse where the wildcard arm's body becomes the
        // innermost else, making them indistinguishable from a real if/else
        // at the AST level. Auto-fixing them produces broken match code.
        // Hand-fix STYLE017(a) hits flagged by the linter.
        return if (!in_target_file(ifte.at) || ifte.if_flags.isStatic
                || ifte.if_false != null || !(ifte.if_true is ExprBlock))
        let blk = ifte.if_true as ExprBlock
        return if (length(blk.list) != 1 || !empty(blk.finalList))
        let inner = blk.list[0]
        // Synthetic Block (braceless / postfix-desugared) shares LineInfo with
        // the inner terminator — that's the AST signal we are NOT looking at
        // user-written braces. Skip.
        return if (!(inner is ExprReturn || inner is ExprBreak || inner is ExprContinue)
                || (blk.at.line == inner.at.line && blk.at.column == inner.at.column))
        let inner_text = extract_block_inner(blk.at)
        return if (empty(inner_text))
        // Sanity-check: the extracted source must START with a terminator
        // keyword. Defends against `visit_with_generics` surfacing transformed
        // AST whose block shape doesn't match the user source.
        let stripped = strip(inner_text)
        if (!(stripped |> starts_with("return")
                || stripped |> starts_with("break")
                || stripped |> starts_with("continue"))) return
        push_edit(blk.at, inner_text, "STYLE005")
    }

    // --- PERF013 / PERF017 / STYLE018 ---

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        return if (in_template || !in_target_file(expr.at))
        let opname = string(expr.op)
        // PERF013: a += ±1 / a -= ±1 → a++ / a--
        if (opname == "+=" || opname == "-=") {
            check_perf013(expr, opname)
            return
        }
        // STYLE018: b == true / != false → b ; b == false / != true → !b
        if (opname == "==" || opname == "!=") {
            check_style018(expr, opname)
        }
        return if (opname != "==" && opname != "!=" && opname != "<"
                && opname != "<=" && opname != ">" && opname != ">=")
        let lhs = peel_ref2value(expr.left)
        let rhs = peel_ref2value(expr.right)
        var len_on_left = true
        if (is_length_call(lhs)) {
            len_on_left = true
        } elif (is_length_call(rhs)) {
            len_on_left = false
        } else {
            return
        }
        let len_call = len_on_left ? (lhs as ExprCall) : (rhs as ExprCall)
        let const_side = len_on_left ? rhs : lhs
        var k = 0
        return if (!const_int_value(const_side, k) || (k != 0 && k != 1))
        // Canonicalize so length is on the left; flip op if needed.
        var canon_op = opname
        if (!len_on_left) {
            if (opname == "<") {
                canon_op = ">"
            } elif (opname == "<=") {
                canon_op = ">="
            } elif (opname == ">") {
                canon_op = "<"
            } elif (opname == ">=") {
                canon_op = "<="
            }
        }
        var is_empty_pattern = false
        var is_nonempty_pattern = false
        if (k == 0) {
            if (canon_op == "==" || canon_op == "<=") {
                is_empty_pattern = true
            } elif (canon_op == "!=" || canon_op == ">") {
                is_nonempty_pattern = true
            }
        } else {
            if (canon_op == "<") {
                is_empty_pattern = true
            } elif (canon_op == ">=") {
                is_nonempty_pattern = true
            }
        }
        return if ((!is_empty_pattern && !is_nonempty_pattern) || empty(len_call.arguments))
        let raw_arg = len_call.arguments[0]
        let arg = peel_ref2value(raw_arg)
        // Bail on compound / parenthesized / deref args — their source bounds
        // aren't reliable (parens aren't in AST, ExprPtr2Ref.at covers `*` only).
        return if (!is_simple_subject(arg))
        let arg_text = extract_expr_source(arg, expr.at.fileInfo)
        return if (empty(arg_text))
        let replacement = is_empty_pattern ? "empty({arg_text})" : "!empty({arg_text})"
        // ExprOp2.at covers only the operator token, not the full expression.
        // Worse, the typer may canonicalize Yoda comparisons so the AST left is
        // not the source left. Compute the actual source bounds via min/max of
        // each operand's source span (which for calls means atEnclosure).
        var l_sl = 0; var l_sc = 0; var l_el = 0; var l_ec = 0
        var r_sl = 0; var r_sc = 0; var r_el = 0; var r_ec = 0
        source_span_of(lhs, l_sl, l_sc, l_el, l_ec)
        source_span_of(rhs, r_sl, r_sc, r_el, r_ec)
        var start_line = l_sl
        var start_col = l_sc
        if (r_sl < start_line || (r_sl == start_line && r_sc < start_col)) {
            start_line = r_sl
            start_col = r_sc
        }
        var end_line = l_el
        var end_col = l_ec
        if (r_el > end_line || (r_el == end_line && r_ec > end_col)) {
            end_line = r_el
            end_col = r_ec
        }
        push_edit_span(start_line, start_col, end_line, end_col, replacement, "PERF017")
    }

    def check_perf013(expr : ExprOp2?; opname : string) : void {
        //! a += 1 / a -= 1 / a += -1 / a -= -1 → a++ / a--
        //! Net delta: '+=' with +N has sign of N; '-=' flips the sign.
        // Bail on compound / parenthesized LHS — their span isn't reliable.
        return if (expr.left == null || !is_workhorse_numeric(expr.left._type)
                || !is_simple_subject(peel_ref2value(expr.left)))
        var rhs_sign = 0
        return if (!is_const_pm_one(expr.right, rhs_sign))
        let delta = opname == "+=" ? rhs_sign : -rhs_sign
        return if (delta != 1 && delta != -1)
        // Find the leftmost source position of the LHS (which may be a field
        // chain like `s.count` or an index `arr[i]` whose `.at` is the inner
        // separator, not the root). The LHS source extends from there to the
        // start of the operator (expr.at.column).
        var lhs_lo_line = 0
        var lhs_lo_col = 0
        leftmost_pos_of(expr.left, lhs_lo_line, lhs_lo_col)
        return if (lhs_lo_line == 0)
        let op_line = int(expr.at.line)
        let op_col = int(expr.at.column)
        let lhs_text = strip(extract_source_at(
            lhs_lo_line, lhs_lo_col, op_line, op_col, expr.at.fileInfo))
        return if (empty(lhs_text))
        let suffix = delta == 1 ? "++" : "--"
        // Replacement spans from LHS leftmost to RHS last_column (the const).
        let r_end_line = int(expr.right.at.last_line)
        let r_end_col = int(expr.right.at.last_column)
        push_edit_span(lhs_lo_line, lhs_lo_col,
                             r_end_line, r_end_col,
                             "{lhs_text}{suffix}", "PERF013")
    }

    def is_const_bool_return(e : Expression? const; var bv : bool&) : bool {
        //! True if `e` is `return <ConstBool>` (or 1-stmt block wrapping one).
        //! Writes the bool value to `bv`.
        return false if (e == null)
        if (e is ExprBlock) {
            let blk = e as ExprBlock
            return false if (length(blk.list) != 1 || !empty(blk.finalList))
            return is_const_bool_return(blk.list[0], bv)
        }
        return false if (!(e is ExprReturn))
        let ret = e as ExprReturn
        return false if (ret.subexpr == null)
        let sub = peel_ref2value(ret.subexpr)
        return false if (sub == null || !(sub is ExprConstBool))
        bv = (sub as ExprConstBool).value
        return true
    }

    def check_style017b(blk : ExprBlock?) : void {
        //! Form (b): adjacent `if (cond) return b1` / `return b2` (b1 != b2) →
        //! `return cond` or `return !cond`. Spans both statements.
        return if (in_template || !in_target_file(blk.at))
        let n = length(blk.list)
        return if (n < 2)
        for (i in range(n - 1)) {
            let cur = blk.list[i]
            let nxt = blk.list[i + 1]
            continue if (!(cur is ExprIfThenElse))
            let outer_if = cur as ExprIfThenElse
            // Postfix-desugared form: `return X if (cond)` parses into an
            // ExprIfThenElse but the source line does NOT start with `if`.
            // Skip — the span/replacement logic would clobber the leading
            // `return X` token (corruption observed on utils/fix-lint-errors).
            // Also bail when the condition contains an `is` operator (lowered
            // by the typer to `__rtti == "..."` with a synthetic ExprConstString
            // whose `at` no longer covers the source type name). Rightmost
            // computation is unreliable across this lowering.
            continue if (outer_if.if_false != null || outer_if.if_flags.isStatic
                || !source_line_starts_with_if(outer_if.at)
                || expr_contains_rtti_check(outer_if.cond))
            var b1 = false
            var b2 = false
            continue if (!is_const_bool_return(outer_if.if_true, b1)
                || !is_const_bool_return(nxt, b2)
                || b1 == b2 || outer_if.cond == null)
            let cond = peel_ref2value(outer_if.cond)
            continue if (cond == null || cond_uses_property_call(cond))
            var c_lo_l = 0; var c_lo_c = 0
            var c_hi_l = 0; var c_hi_c = 0
            leftmost_pos_of(cond, c_lo_l, c_lo_c)
            rightmost_pos_of(cond, c_hi_l, c_hi_c)
            if (cond is ExprOp2) {
                let op = cond as ExprOp2
                continue if (op.left == null || op.right == null)
                leftmost_pos_of(op.left, c_lo_l, c_lo_c)
                rightmost_pos_of(op.right, c_hi_l, c_hi_c)
            }
            continue if (c_lo_l == 0 || c_hi_l == 0)
            let cond_text = extract_source_at(c_lo_l, c_lo_c, c_hi_l, c_hi_c, outer_if.at.fileInfo)
            continue if (empty(cond_text))
            // Sanity-check: if the extracted text starts with punctuation, the
            // chain helper landed mid-token — skip rather than emit corruption.
            let c0 = first_character(cond_text)
            continue if (c0 == '.' || c0 == ']' || c0 == ',' || c0 == ')' || c0 == '}')
            let replacement = (b1
                ? "return {cond_text}"
                : (cond is ExprOp2 ? "return !({cond_text})" : "return !{cond_text}"))
            // Span: outer_if.at start to nxt.at end. nxt is ExprReturn whose
            // .at covers `return`; the source end is past the const bool.
            let start_line = int(outer_if.at.line)
            let start_col = int(outer_if.at.column)
            let end_line = int(nxt.at.last_line)
            var end_col = int(nxt.at.last_column)
            // Walk into the return to extend end past the const bool.
            if (nxt is ExprReturn) {
                let ret = nxt as ExprReturn
                if (ret.subexpr != null) {
                    let sub = peel_ref2value(ret.subexpr)
                    if (sub != null && sub is ExprConstBool) {
                        end_col = int(sub.at.last_column)
                    }
                }
            }
            push_edit_span(start_line, start_col, end_line, end_col, replacement, "STYLE017")
        }
    }

    // --- PERF015 / PERF016: ternary min/max/abs ---

    def override preVisitExprOp3(expr : ExprOp3?) : void {
        return if (in_template || !in_target_file(expr.at)
                || expr.op != "?" || expr.subexpr == null
                || !(expr.subexpr is ExprOp2))
        let cmp = expr.subexpr as ExprOp2
        let cop = string(cmp.op)
        return if ((cop != "<" && cop != "<=" && cop != ">" && cop != ">=")  // nolint:STYLE016 — next guard has side effect (try_perf016 must run); not safely OR-able
                || expr.left == null || expr.right == null)
        // PERF016 (abs) takes precedence — its pattern is stricter (one side is zero).
        if (try_perf016(expr, cmp, cop)) return
        try_perf015(expr, cmp, cop)
    }

    def has_paren_at(fi : FileInfo? const; line : uint; col : int; ch : int) : bool {
        //! Reads one byte of source. Used to detect explicit-parens that
        //! aren't in the AST. `col` is 0-based.
        return false if (fi == null)
        var found = false
        get_file_source_line(fi, line) $(line_text) {
            if (col >= 0 && col < length(line_text)) {
                found = character_at(line_text, col) == ch  // nolint:PERF003 — cold: invoked per-call from PERF015/016 paren detection
            }
        }
        return found
    }

    def is_parenthesized_cmp(cmp : ExprOp2?) : bool {
        //! True when the comparison appears in source as `(left op right)` —
        //! parens around it. PERF015/16 spans would swallow the closing `)`
        //! since parens aren't in the AST. Heuristic: look for `)` right after
        //! cmp.right.last_column.
        return false if (cmp == null || cmp.right == null)
        var hi_l = 0; var hi_c = 0
        rightmost_pos_of(cmp.right, hi_l, hi_c)
        return has_paren_at(cmp.at.fileInfo, uint(hi_l), hi_c, ')')
    }

    def try_perf015(expr : ExprOp3?; cmp : ExprOp2?; cop : string) : bool {
        return false if (!has_math_require || is_parenthesized_cmp(cmp))
        let cl = cmp.left
        let cr = cmp.right
        return false if (cl == null || cr == null)
        let t_branch = expr.left
        let f_branch = expr.right
        let tcl = expr_describe_equal(t_branch, cl)
        let tcr = expr_describe_equal(t_branch, cr)
        let fcl = expr_describe_equal(f_branch, cl)
        let fcr = expr_describe_equal(f_branch, cr)
        let same_order = tcl && fcr
        let swap_order = tcr && fcl
        return false if (!same_order && !swap_order)
        var is_min = false
        if (cop == "<" || cop == "<=") {
            is_min = same_order
        } else {
            is_min = swap_order
        }
        let which = is_min ? "min" : "max"
        // Use cmp.left and cmp.right as canonical args — they're guaranteed
        // present (no synthetic order shuffling at this point).
        let a_text = extract_expr_source(cl, expr.at.fileInfo)
        let b_text = extract_expr_source(cr, expr.at.fileInfo)
        return false if (empty(a_text) || empty(b_text))
        let replacement = "{which}({a_text}, {b_text})"
        // Span: min(leftmost over {cmp.left, cmp.right}) to rightmost(f_branch).
        // The typer canonicalizes Yoda comparisons so cmp.left isn't always the
        // source-left operand.
        var s_l = 0; var s_c = 0
        var e_l = 0; var e_c = 0
        var t_l = 0; var t_c = 0
        leftmost_pos_of(cl, t_l, t_c)
        lo_min(s_l, s_c, t_l, t_c)
        leftmost_pos_of(cr, t_l, t_c)
        lo_min(s_l, s_c, t_l, t_c)
        rightmost_pos_of(t_branch, t_l, t_c)
        hi_max(e_l, e_c, t_l, t_c)
        rightmost_pos_of(f_branch, t_l, t_c)
        hi_max(e_l, e_c, t_l, t_c)
        if (s_l == 0) return false
        push_edit_span(s_l, s_c, e_l, e_c, replacement, "PERF015")
        return true
    }

    def try_perf016(expr : ExprOp3?; cmp : ExprOp2?; cop : string) : bool {
        return false if (!has_math_require || is_parenthesized_cmp(cmp))
        var x_on_left = false
        if (is_const_zero(cmp.right)) {
            x_on_left = true
        } elif (is_const_zero(cmp.left)) {
            x_on_left = false
        } else {
            return false
        }
        let x_expr = x_on_left ? cmp.left : cmp.right
        return false if (x_expr == null)
        let t_branch = expr.left
        let f_branch = expr.right
        var neg_then = false
        var ok = false
        if (is_neg_of(t_branch, x_expr) && expr_describe_equal(f_branch, x_expr)) {
            neg_then = true
            ok = true
        } elif (expr_describe_equal(t_branch, x_expr) && is_neg_of(f_branch, x_expr)) {
            neg_then = false
            ok = true
        }
        return false if (!ok)
        var is_abs = false
        if (x_on_left) {
            if ((cop == "<" || cop == "<=") && neg_then) {
                is_abs = true
            } elif ((cop == ">" || cop == ">=") && !neg_then) {
                is_abs = true
            }
        } else {
            if ((cop == ">" || cop == ">=") && neg_then) {
                is_abs = true
            } elif ((cop == "<" || cop == "<=") && !neg_then) {
                is_abs = true
            }
        }
        return false if (!is_abs)
        let x_text = extract_expr_source(x_expr, expr.at.fileInfo)
        return false if (empty(x_text))
        let replacement = "abs({x_text})"
        // Span: min(leftmost over {cmp.left, cmp.right}) to rightmost over
        // {t_branch, f_branch}. Typer may have reordered cmp operands.
        var s_l = 0; var s_c = 0
        var e_l = 0; var e_c = 0
        var t_l = 0; var t_c = 0
        leftmost_pos_of(cmp.left, t_l, t_c)
        lo_min(s_l, s_c, t_l, t_c)
        leftmost_pos_of(cmp.right, t_l, t_c)
        lo_min(s_l, s_c, t_l, t_c)
        rightmost_pos_of(t_branch, t_l, t_c)
        hi_max(e_l, e_c, t_l, t_c)
        rightmost_pos_of(f_branch, t_l, t_c)
        hi_max(e_l, e_c, t_l, t_c)
        if (s_l == 0) return false
        push_edit_span(s_l, s_c, e_l, e_c, replacement, "PERF016")
        return true
    }

    def check_style017a(ifte : ExprIfThenElse?) : void {
        //! Form (a): `if (cond) return b1 else return b2` (b1 != b2) →
        //! `return cond` or `return !cond`.
        return if (in_template || ifte.if_false == null)
        var b1 = false
        var b2 = false
        return if (!is_const_bool_return(ifte.if_true, b1)
                || !is_const_bool_return(ifte.if_false, b2)
                || b1 == b2 || ifte.cond == null)
        let cond = peel_ref2value(ifte.cond)
        return if (cond == null)
        // For the `cond` source, find its leftmost/rightmost bounds.
        var c_lo_l = 0; var c_lo_c = 0
        var c_hi_l = 0; var c_hi_c = 0
        leftmost_pos_of(cond, c_lo_l, c_lo_c)
        rightmost_pos_of(cond, c_hi_l, c_hi_c)
        // `cond` may be a compound ExprOp2 (e.g. `x > 0`). Its `.at` covers only
        // the operator, so use leftmost/rightmost of operands. Skip the rule if
        // we can't reliably bound the cond — leave for hand-fix.
        if (cond is ExprOp2) {
            let op = cond as ExprOp2
            return if (op.left == null || op.right == null)
            leftmost_pos_of(op.left, c_lo_l, c_lo_c)
            rightmost_pos_of(op.right, c_hi_l, c_hi_c)
        }
        return if (c_lo_l == 0 || c_hi_l == 0)
        let cond_text = extract_source_at(c_lo_l, c_lo_c, c_hi_l, c_hi_c, ifte.at.fileInfo)
        return if (empty(cond_text))
        let replacement = (b1
            ? "return {cond_text}"
            : (cond is ExprOp2 ? "return !({cond_text})" : "return !{cond_text}"))
        // Span: from ifte.at start to the end of if_false (entire if/else
        // statement). if_false is an ExprBlock (braces) or bare ExprReturn.
        let start_line = int(ifte.at.line)
        let start_col = int(ifte.at.column)
        let end_line = int(ifte.if_false.at.last_line)
        let end_col = int(ifte.if_false.at.last_column)
        push_edit_span(start_line, start_col, end_line, end_col, replacement, "STYLE017")
    }

    def check_style018(expr : ExprOp2?; opname : string) : void {
        //! `<bool_expr> == true/false` / `<bool_expr> != true/false` → drop the
        //! literal. Only fires when the non-bool side is a simple subject (var /
        //! field / index / call) — compound or parenthesized expressions are
        //! left for hand-cleanup since their source span isn't reliable.
        let lhs = peel_ref2value(expr.left)
        let rhs = peel_ref2value(expr.right)
        let lbool = lhs != null && lhs is ExprConstBool
        let rbool = rhs != null && rhs is ExprConstBool
        return if (lbool == rbool)
        let const_side = lbool ? lhs : rhs
        let other_side = lbool ? rhs : lhs
        return if (!is_simple_subject(other_side))
        let cval = (const_side as ExprConstBool).value
        let positive = (opname == "==" && cval) || (opname == "!=" && !cval)
        // Source span of the non-bool subject (leftmost ... rightmost).
        var sub_lo_l = 0; var sub_lo_c = 0
        var sub_hi_l = 0; var sub_hi_c = 0
        leftmost_pos_of(other_side, sub_lo_l, sub_lo_c)
        rightmost_pos_of(other_side, sub_hi_l, sub_hi_c)
        return if (sub_lo_l == 0 || sub_hi_l == 0)
        let other_text = extract_source_at(sub_lo_l, sub_lo_c, sub_hi_l, sub_hi_c, expr.at.fileInfo)
        return if (empty(other_text))
        let replacement = (positive
            ? other_text
            : (needs_parens_for_negation(other_side)
                ? "!({other_text})"
                : "!{other_text}"))
        // Replacement span: subject's full extent vs the bool literal's. The
        // typer may have swapped operands, so take min/max of each side's
        // (leftmost, rightmost).
        var c_lo_l = 0; var c_lo_c = 0
        var c_hi_l = 0; var c_hi_c = 0
        leftmost_pos_of(const_side, c_lo_l, c_lo_c)
        rightmost_pos_of(const_side, c_hi_l, c_hi_c)
        var start_line = sub_lo_l
        var start_col = sub_lo_c
        if (c_lo_l < start_line || (c_lo_l == start_line && c_lo_c < start_col)) {
            start_line = c_lo_l
            start_col = c_lo_c
        }
        var end_line = sub_hi_l
        var end_col = sub_hi_c
        if (c_hi_l > end_line || (c_hi_l == end_line && c_hi_c > end_col)) {
            end_line = c_hi_l
            end_col = c_hi_c
        }
        push_edit_span(start_line, start_col, end_line, end_col, replacement, "STYLE018")
    }

    def needs_parens_for_negation(e : Expression? const) : bool {
        //! Determines whether `!<src(e)>` could re-parse with the wrong precedence.
        //! Safe atoms (no parens): vars, fields, indices, calls, parenthesized exprs,
        //! existing unary ops, constants. Operator-level expressions need parens.
        return false if (e == null)
        return !(e is ExprVar || e is ExprField || e is ExprSafeField
                || e is ExprAt || e is ExprSafeAt || e is ExprCall
                || e is ExprConstBool || e is ExprConstInt || e is ExprConstUInt
                || e is ExprConstFloat || e is ExprConstDouble || e is ExprConstString
                || e is ExprOp1)
    }

    def extract_source_at(sl : int; sc : int; el : int; ec : int;
                          fi : FileInfo? const) : string {
        //! Like extract_source(LineInfo) but takes ints — for spans we build by
        //! hand from operand source bounds.
        return "" if (fi == null || sl == 0)
        var result = ""
        if (sl == el) {
            get_file_source_line(fi, uint(sl)) $(line) {
                let lo = max(0, sc)
                let hi = min(length(line), ec)
                if (hi > lo) {
                    result := slice(line, lo, hi)
                }
            }
            return result
        }
        for (ln in range(sl, el + 1)) {
            get_file_source_line(fi, uint(ln)) $(line) {
                if (ln == sl) {
                    let lo = max(0, sc)
                    if (lo < length(line)) {
                        result := slice(line, lo)
                    }
                } elif (ln == el) {
                    let hi = min(length(line), ec)
                    if (hi > 0) {
                        result := "{result}\n{slice(line, 0, hi)}"
                    }
                } else {
                    result := "{result}\n{line}"
                }
            }
        }
        return result
    }
}

// =====================================================================
// CLI
// =====================================================================

def is_skip_file(name : string) : bool {
    return name == "builtin.das"
}

def is_skip_dir(name : string) : bool {
    return name |> starts_with(".")
}

def has_expect_directive(file : string) : bool {
    //! Mirrors utils/lint/main.das — skip files with intentional compile errors.
    let text = fread(file)
    return false if (empty(text))
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        return true if (trimmed |> starts_with("expect "))
    }
    return false
}

def scan_das_files(path : string; var files : array<string>; var cache : table<string; void?>) {
    let st = stat(path)
    return if (!st.is_valid)
    if (st.is_reg) {
        if (path |> ends_with(".das") && !cache |> key_exists(path)) {
            let last_slash = max(rfind(path, "/"), rfind(path, "\\"))
            let fname = last_slash >= 0 ? slice(path, last_slash + 1) : path
            if (!is_skip_file(fname)) {
                cache |> insert(path, null)
                files |> push(path)
            }
        }
        return
    }
    return if (!st.is_dir)
    fio::dir(path) $(name) {
        return if (name == "." || name == ".." || name |> starts_with("_") || is_skip_dir(name))
        let full = "{path}/{name}"
        let fst = stat(full)
        return if (!fst.is_valid)
        if (fst.is_dir) {
            scan_das_files(full, files, cache)
        } elif (fst.is_reg && name |> ends_with(".das") && !is_skip_file(name) && !cache |> key_exists(full)) {
            cache |> insert(full, null)
            files |> push(full)
        }
    }
}

struct EditRecord {
    start_line : int
    start_col : int
    end_line : int
    end_col : int
    replacement : string
    rule : string
}

def file_requires_math(file : string) : bool {
    //! True if the source file has a `require math` (any variant). PERF015 /
    //! PERF016 rewrites resolve `min`/`max`/`abs` from the math module, so
    //! files without the require would fail to compile post-rewrite.
    let text = fread(file)
    return false if (empty(text))
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        if (trimmed |> starts_with("require math")
                || trimmed |> starts_with("require_jit math")
                || trimmed == "require math"
                || trimmed |> starts_with("require math ")) return true
        // Stop scanning once past the require block — declarations indicate
        // we're in the body and there are no more requires above this line.
        if (trimmed |> starts_with("def ") || trimmed |> starts_with("[")
                || trimmed |> starts_with("class ") || trimmed |> starts_with("struct ")
                || trimmed |> starts_with("variant ") || trimmed |> starts_with("enum ")
                || trimmed |> starts_with("typedef ")) return false
    }
    return false
}

def collect_edits(file : string; var records : array<EditRecord>) : bool {
    //! Returns true if the file compiled (whether or not edits were collected).
    var ok_out = false
    let math_required = file_requires_math(file)
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; _issues) {
                ok_out = ok
                return if (!ok)
                var astVisitor = new FixerVisitor(has_math_require = math_required, target_filename := file)
                make_visitor(*astVisitor) $(adapter) {
                    visit_with_generics(program, adapter)
                }
                let n = length(astVisitor.edits)
                records |> reserve(length(records) + n)
                for (i in range(n)) {
                    var r = EditRecord(
                        start_line = astVisitor.edits[i].start_line,
                        start_col = astVisitor.edits[i].start_col,
                        end_line = astVisitor.edits[i].end_line,
                        end_col = astVisitor.edits[i].end_col,
                        replacement = astVisitor.replacements[i],
                        rule = astVisitor.edits[i].rule)
                    records |> push(<- r)
                }
                unsafe {
                    delete astVisitor
                }
            }
        }
    }
    return ok_out
}

def edits_overlap(a : EditRecord; b : EditRecord) : bool {
    //! True if [a.start, a.end) and [b.start, b.end) share any column.
    // a ends before b starts → no overlap
    let a_before_b = (a.end_line < b.start_line
        || (a.end_line == b.start_line && a.end_col <= b.start_col))
    let b_before_a = (b.end_line < a.start_line
        || (b.end_line == a.start_line && b.end_col <= a.start_col))
    return !(a_before_b || b_before_a)
}

def try_compile(file : string) : bool {
    //! Best-effort post-rewrite verification. Compiles the file as bin/daslang
    //! would — full shared-module loading, optimizations and const-folding on —
    //! so dependent modules' macros (e.g. dastest's UnrollMacro, which needs
    //! `range(const)`) see the same shape they saw during edit collection.
    //! Stricter settings produced false-positive reverts on files that
    //! transitively require dastest, sqlite_boost, etc.
    var ok_out = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true
            cop.lint_check = false
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; _program; _issues) {
                ok_out = ok
            }
        }
    }
    return ok_out
}

def apply_edits_to_file(file : string; var records : array<EditRecord>; dry_run : bool; no_verify : bool = false) : int {
    //! Applies all edits to a file's source, returning the number written.
    //! Sorts edits by descending end position so earlier-position edits stay
    //! valid through later applications. Drops edits subsumed by a larger one
    //! (e.g. STYLE005 inside STYLE017(b)) — outer wins. After writing, attempts
    //! to recompile; restores the original file if the rewrite broke compile.
    return 0 if (empty(records))
    let original_text = fread(file)
    return 0 if (empty(original_text))
    var lines <- original_text |> split("\n")
    // Sort by (end_line desc, end_col desc) — apply from end of file backwards.
    // Daslang has `sort` from daslib/sort_boost; here we just bubble for small N.
    let n0 = length(records)
    for (i in range(n0)) {
        for (j in range(i + 1, n0)) {
            let a = records[i]
            let b = records[j]
            let a_after_b = (a.end_line > b.end_line
                || (a.end_line == b.end_line && a.end_col > b.end_col))
            if (!a_after_b) {
                var tmp <- records[i]
                records[i] <- records[j]
                records[j] <- tmp
            }
        }
    }
    // Drop subsumed/overlapping edits: walk in order (now end-desc) and keep an
    // edit only if it doesn't overlap any earlier (larger-end) kept edit.
    var keep : array<EditRecord>
    for (r in records) {
        var skip = false
        for (k in keep) {
            if (edits_overlap(k, r)) {
                skip = true
                break
            }
        }
        if (!skip) {
            var clone <- r
            keep |> push(<- clone)
        }
    }
    records |> clear
    records |> reserve(length(keep))
    for (r in keep) {
        var clone <- r
        records |> push(<- clone)
    }
    let n = length(records)
    for (r in records) {
        let s_idx = r.start_line - 1
        let e_idx = r.end_line - 1
        if (s_idx < 0 || e_idx >= length(lines)) continue
        let start_line_text = lines[s_idx]
        let end_line_text = lines[e_idx]
        // Coordinates are 0-based, last_column exclusive (see extract_block_inner).
        // start_col is the index of `{`; everything before is prefix.
        // end_col is one past `}`; everything from end_col on is suffix.
        let prefix_lo = max(0, r.start_col)
        let prefix = ((prefix_lo > length(start_line_text))
            ? start_line_text
            : slice(start_line_text, 0, prefix_lo))
        let suffix_lo = r.end_col
        let suffix = ((suffix_lo >= length(end_line_text))
            ? ""
            : slice(end_line_text, suffix_lo))
        let merged = "{prefix}{r.replacement}{suffix}"
        // Replace lines [s_idx .. e_idx] with the single merged line.
        // We rebuild via a fresh list.
        var new_lines : array<string>
        new_lines |> reserve(length(lines) - (e_idx - s_idx))
        for (k in range(length(lines))) {
            if (k < s_idx || k > e_idx) {
                new_lines |> push(lines[k])
            } elif (k == s_idx) {
                new_lines |> push(merged)
            }
        }
        lines <- new_lines
    }
    if (!dry_run) {
        let new_text = join(lines, "\n")
        fwrite(file, new_text)
        // Safety net: if the rewritten file no longer compiles, restore the
        // original. Prevents bulk-pass cascade failures from rewrites that
        // surface edge cases in span computation. `--no-verify` disables it
        // for debugging the broken-rewrite output.
        if (!no_verify && !try_compile(file)) {
            fwrite(file, original_text)
            return -1
        }
    }
    return n
}

[export]
def main() : int {
    var r <- parse_args(type<Config>)
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "utils/fix-lint-errors/main.das")
        return 1
    }
    let cfg <- r |> move_unwrap
    if (cfg.help) {
        print("Auto-fixer for mechanical lint rules (v1: STYLE005).\n")
        print("Walks the AST, identifies fixable patterns, rewrites the source.\n\n")
        print_help(get_command_info(type<Config>), "utils/fix-lint-errors/main.das")
        print("\nPositional arguments: one or more files or directories.\n")
        return 0
    }
    let cli_args <- get_user_args()
    let paths <- [for (arg in cli_args); arg; where !starts_with(arg, "-")]
    if (empty(paths)) {
        print("Error: no files or directories specified\n\n")
        print_help(get_command_info(type<Config>), "utils/fix-lint-errors/main.das")
        return 1
    }
    var files : array<string>
    var cache : table<string; void?>
    for (p in paths) {
        scan_das_files(p, files, cache)
    }
    if (empty(files)) {
        print("Error: no .das files found\n")
        return 1
    }
    var total_edits = 0
    var total_files_edited = 0
    var total_files_failed = 0
    var total_files_reverted = 0
    for (file in files) {
        if (has_expect_directive(file)) {
            print("SKIP {file} (expect-directive test fixture)\n") if (!cfg.quiet)
            continue
        }
        var records : array<EditRecord>
        let compiled = collect_edits(file, records)
        if (!compiled) {
            total_files_failed++
            print("FAIL {file} (compile error)\n")
            continue
        }
        if (empty(records)) {
            print("PASS {file}\n") if (!cfg.quiet)
            continue
        }
        // Group by rule for the per-file report.
        var rule_counts : table<string; int>
        for (rec in records) {
            if (key_exists(rule_counts, rec.rule)) {
                rule_counts[rec.rule]++
            } else {
                rule_counts[rec.rule] = 1
            }
        }
        // Build summary outside the keys() iteration to keep the table unlocked.
        var rule_strs : array<string>
        rule_strs |> reserve(length(rule_counts))
        for (rule_name in keys(rule_counts)) {
            rule_strs |> push(rule_name)
        }
        let summary = build_string() $(w) {
            for (rule_str in rule_strs) {
                let cnt = rule_counts[rule_str]
                w |> write(" {rule_str}={cnt}")
            }
        }
        let action = cfg.dry_run ? "WOULD FIX" : "FIX"
        print("{action} {file} ({length(records)} edits:{summary})\n")
        let n = apply_edits_to_file(file, records, cfg.dry_run, cfg.no_verify)
        if (n < 0) {
            total_files_reverted++
            print("REVERT {file} (post-rewrite compile failed)\n")
        } else {
            total_edits += n
            total_files_edited++
        }
    }
    print("\n--- Summary ---\n")
    print("{length(files)} files scanned, {total_files_edited} edited, ")
    print("{total_edits} edits applied, {total_files_failed} failed")
    if (total_files_reverted > 0) {
        print(", {total_files_reverted} reverted")
    }
    print("\n")
    if (cfg.dry_run) {
        print("(dry-run — no files written)\n")
    }
    return total_files_failed > 0 ? 1 : 0
}
