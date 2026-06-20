options gen2
options indenting = 4
options strict_smart_pointers = true

module style_lint shared private

//! Style lint module.
//!
//! Detects non-idiomatic patterns in daslang code at compile time.
//! When this module is required, a lint pass runs after compilation and reports
//! warnings as ``CompilationError::runtime_macro_style`` (error code 31209).
//!
//! Rules:
//!   STYLE001 — unnecessary <| pipe before block argument
//!   STYLE002 — <| pipe before parameterless block (remove pipe and $())
//!   STYLE003 — redundant $() on parameterless block argument
//!   STYLE005 — braces around a single-statement early exit ('if (cond) { return }') are noise — drop the braces or use postfix
//!   STYLE006 — string(__rtti) comparison should use `is` operator
//!   STYLE010 — if (true) should be a bare block
//!   STYLE011 — variable declaration followed by immediate assignment
//!   STYLE012 — array<T> var initialized via push/emplace; use an array literal
//!   STYLE013 — struct (or `new`-allocated struct pointer) var with default/empty init followed by a run of field assignments
//!   STYLE014 — comment block exceeds 3 lines at module/public scope (opt-in via ``options _comment_hygiene = true``, suppress with '//!@nolint' or '// nolint:STYLE014')
//!   STYLE015 — comment block exceeds 1 line inside a 'def private' (opt-in via ``options _comment_hygiene = true``, suppress with '// nolint:STYLE015')
//!   STYLE016 — adjacent guards leading to identical early-exit can be combined with '||'
//!   STYLE017 — 'if (cond) return true; else return false' — use 'return cond' (or 'return !cond')
//!   STYLE018 — redundant boolean comparison ('b == true' / 'b != false') — use 'b' / '!b'
//!   STYLE019 — nested min/max — use 'clamp(x, lo, hi)' from math module
//!   STYLE020 — from_JV(v, type<T>, defV) on a scalar — use 'v ?? defV' (operator ?? from json_boost)
//!   STYLE021 — table<string; JsonValue?> built by repeated insert — use 'var v = JV((k1=..., k2=...))'
//!   STYLE022 — bitfield compound-bit-set/clear ('foo |= BfT.m' / 'foo &= ~BfT.m') — use field-assign 'foo.m = true' / 'foo.m = false'
//!   STYLE024 — redundant 'unsafe' wrap; inner expression / block body has no operation requiring unsafe — drop the wrap
//!   STYLE025 — 'unsafe { ... }' block where only one statement needs unsafe — narrow to 'unsafe(<sub-expr>)'
//!   STYLE026 — nested 'unsafe { ... }' block; outer wrap already covers the scope — drop the inner
//!   STYLE027 — var array<T> / table<K;V> with empty default-init followed by a for-loop that only push/insert into it; use a comprehension
//!   STYLE028 — 'self->method(...)' inside a class method is optional — drop 'self->' and call 'method(...)' directly (compiler auto-promotes to the same invoke)
//!   STYLE029 — non-public 'require X' used only for modules X re-exports (transitive-only) — require those directly and drop X (skipped when X provides macros or [init])
//!   STYLE030 — non-public 'require X' that is entirely unused — no symbol from X (or anything it re-exports) is referenced; drop it (skipped when X provides any macro or an [init], or only re-exports builtins used through it, or X or its public re-export closure exports a function matching an unresolved call inside an uninstanced generic body). Suppress a deliberate keep with '// nolint:STYLE030'

require daslib/ast_boost
require daslib/is_local
require daslib/lint_config
require strings

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------

struct UnsafeFrame {
    //! Per-expression subtree summary tracked on `StyleLintVisitor.unsafe_stack`.
    //! `count` is the number of inherently-unsafe leaves in the subtree.
    //! `has_non_local_let_ref` flags an enclosing `let v & = E` where `E` is
    //! non-local-non-temporary (per `isLocalOrGlobal` in ast_infer_type.cpp:4989)
    //! — narrowing the enclosing `unsafe { ... }` to expression-form would
    //! leave the let-ref binding unsatisfied, so STYLE025 must stay silent.
    count : int
    has_non_local_let_ref : bool
}

class StyleLintVisitor : AstVisitor {
    compile_time_errors : bool
    comment_hygiene : bool = false
    // counter
    warning_count : int = 0
    // collection mode
    collect_warnings : bool = false
    warnings : array<string>
    // dedup: suppress duplicate warnings from generic instantiations
    reported_locations : table<uint64>
    // current function (for inferStack reporting)
    @do_not_delete current_function : Function?
    // STYLE011: track uninitialized variables from most recent ExprLet
    @do_not_delete pending_uninit_vars : array<Variable?>
    // STYLE024/025 — tight unsafe checks.
    @do_not_delete skip_userSaidItsSafe : array<Expression?>
    @do_not_delete unsafeExprs : table<Expression?; UnsafeFrame>
    unsafe_stack : array<UnsafeFrame>
    unsafe_block_stack : array<int>
    // STYLE029 — transitive-only require detection. Enabled only when linting the
    // root module. `used_modules` collects names of modules with >=1 direct
    // reference from code written in `this_module`.
    analyze_requires : bool = false
    @do_not_delete this_module : Module?
    used_modules : table<string>
    // Calls inside UNINSTANCED generic bodies stay unresolved at lint time (func==null)
    // and leave no used_modules trace; their names gate STYLE030 instead.
    unresolved_call_names : table<string>

    def StyleLintVisitor() {
        pass
    }

    def location_key(at : LineInfo) : uint64 {
        return uint64(at.line) | (uint64(at.column) << uint64(20))
    }

    // Filters: disabled (denylist) and enabled (whitelist; empty == all).
    // Caller-populated via collect overload; applied alongside // nolint suppression.
    disabled_codes : table<string>
    enabled_codes : table<string>

    def is_suppressed(text : string; at : LineInfo) : bool {
        let code = extract_lint_code(text)
        if (!empty(code)) {
            return true if (key_exists(disabled_codes, code)
                            || (!empty(enabled_codes) && !key_exists(enabled_codes, code)))
        }
        return is_lint_suppressed(at, code)
    }

    def style_warning(text : string; at : LineInfo) : void {
        // Suppress warnings for macro-generated functions — the user didn't write that code.
        if (current_function != null && current_function.flags.generated) return
        let key = location_key(at)
        if (key_exists(reported_locations, key)) return
        reported_locations |> insert(key)
        if (is_suppressed(text, at)) return
        warning_count++
        var msg = text
        if (current_function != null && current_function.fromGeneric != null && !empty(current_function.inferStack)) {
            msg = build_string() $(var w) {
                w |> write(text)
                w |> write("\n  while compiling {current_function.name}")
                for (ih in current_function.inferStack) {
                    w |> write("\n  instanced from {ih.func.name} at {describe(ih.at)}")
                }
            }
        }
        if (compile_time_errors) {
            compiling_program() |> macro_style_warning(at, msg)
        } elif (collect_warnings) {
            warnings |> push("style warning: {msg} at {describe(at)}")
        } else {
            to_log(LOG_WARNING, "style warning: {msg} at {describe(at)}\n")
        }
    }

    // --- source line helpers ---

    def source_line_between(at_start : LineInfo; at_end : LineInfo) : string {
        //! Reads the source text between two line infos (potentially multi-line).
        //! Returns the concatenated lines from at_start.line to at_end.line.
        var result = ""
        if (at_start.fileInfo == null) return result
        for (ln in range(int(at_start.line), int(at_end.line) + 1)) {
            get_file_source_line(at_start.fileInfo, uint(ln)) $(line) {
                result = "{result}{line}\n"
            }
        }
        return result
    }

    // --- function tracking ---

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = fn
        unsafe_block_stack |> push(0)
        check_defer_pipe(fn)
        if (analyze_requires && fn._module == this_module) {
            for (a in fn.annotations) {
                if (a != null && a.annotation != null) st029_note_module(a.annotation._module)
            }
        }
    }

    // --- STYLE001/002 for defer <| (macro erases AST, scan source lines) ---

    def check_defer_pipe(fn : FunctionPtr) : void {
        if (fn.body == null || fn.at.fileInfo == null || fn.at.line == 0u) return
        let body_at = fn.body.at
        for (ln in range(int(body_at.line), int(body_at.last_line) + 1)) {
            var src = ""
            get_file_source_line(fn.at.fileInfo, uint(ln)) $(line) {
                src := line
            }
            let comment_pos = find(src, "//")
            let defer_pos = find(src, "defer")
            if (defer_pos < 0 || (comment_pos >= 0 && comment_pos < defer_pos)) continue
            if (defer_pos > 0) {
                var preceding_ok = false
                peek_data(src) $(data) {
                    let ch = int(data[defer_pos - 1])
                    preceding_ok = ch == ' ' || ch == '\t'
                }
                if (!preceding_ok) continue
            }
            let pipe_pos = find(src, "<|", defer_pos + 5)
            if (pipe_pos < 0) continue
            let after_pipe = strip_left(slice(src, pipe_pos + 2))
            let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
            if (!pipes_block) continue
            var at = fn.at
            at.line = uint(ln)
            at.column = uint(defer_pos)
            at.last_line = uint(ln)
            at.last_column = uint(defer_pos + 5)
            if (starts_with(after_pipe, "$()")) {
                style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", at)
            } else {
                style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", at)
            }
        }
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        current_function = null
        unsafe_block_stack |> pop
        return <- fn
    }

    // --- STYLE001/002/003/011: pipe and redundant $() on block arguments ---

    def check_block_pipe(call_at : LineInfo; var arguments : dasvector`ptr`Expression) : void {
        let nargs = length(arguments)
        if (nargs == 0) return
        // Find ExprMakeBlock among arguments (may not be last due to ExprFakeContext/ExprFakeLineInfo)
        for (i in range(nargs)) {
            var arg = arguments[i]
            if (arg is ExprMakeBlock) {
                var mkblk = arg as ExprMakeBlock
                var blk = mkblk._block
                if (blk is ExprBlock) {
                    var eblk = blk as ExprBlock
                    if (eblk.blockFlags.isClosure) {
                        let block_arg_count = length(eblk.arguments)
                        // Read source around the call to check for <| and $()
                        let src = source_line_between(call_at, eblk.at)
                        let pipe_pos = find(src, "<|")
                        let has_pipe = pipe_pos >= 0
                        if (has_pipe) {
                            // Only warn when <| pipes a block/lambda ($, @, @@, or {).
                            // Value pipes like <| qmacro(...) are fine style.
                            let after_pipe = strip_left(slice(src, pipe_pos + 2))
                            let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
                            if (pipes_block) {
                                // Check for lambda shorthand: <| $(params) => expr
                                // The => syntax requires <|, so don't flag
                                let arrow_pos = find(after_pipe, "=>")
                                let brace_pos = find(after_pipe, "\{")
                                let is_lambda_shorthand = arrow_pos >= 0 && (brace_pos < 0 || arrow_pos < brace_pos)
                                if (!is_lambda_shorthand) {
                                    if (block_arg_count == 0) {
                                        style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", call_at)
                                    } else {
                                        style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", call_at)
                                    }
                                }
                            }
                        }
                        // STYLE003 for $() without pipe is handled in preVisitExprMakeBlock
                    }
                }
                break
            }
        }
    }

    def override preVisitExprCall(var expr : ExprCall?) : void {
        st029_record_func(expr.func)
        st029_record_unresolved_call(expr)
        st029_record_module_getter(expr)
        check_block_pipe(expr.at, expr.arguments)
        check_style019_clamp(expr)
        check_style020_from_jv(expr)
        if (expr.genFlags.generated) return
        if (call_func_needs_unsafe(expr.func, expr.flags.isForLoopSource)) {
            mark_unsafe_in_stack()
            return
        }
        if (expr.func != null && expr.func.flags.isClassMethod
                && expr.func.result != null && expr.func.result.isClass
                && expr.func.result.structType != null
                && expr.func.name == expr.func.result.structType.name) {
            mark_unsafe_in_stack()
        }
    }

    // --- STYLE020: from_JV scalar form → v ?? defV ---

    def is_supported_jv_scalar(t : TypeDecl?) : bool {
        //! True for the scalar types that have a matching `operator ??` overload in
        //! daslib/json_boost — see lines 85-140 of json_boost.das. Bitfield and enum
        //! `from_JV` overloads exist but have no `??` counterpart, so they stay silent.
        if (t == null) return false
        let bt = t.baseType
        return (bt == Type.tInt    || bt == Type.tUInt
                || bt == Type.tInt8  || bt == Type.tUInt8
                || bt == Type.tInt16 || bt == Type.tUInt16
                || bt == Type.tInt64 || bt == Type.tUInt64
                || bt == Type.tFloat || bt == Type.tDouble
                || bt == Type.tBool  || bt == Type.tString)
    }

    def root_generic(fn : Function const?) : Function const? {
        //! Walk fromGeneric chain to its root. Template instantiations of overloaded
        //! generics produce a 2-level chain: instantiated → mangled-overload → root.
        var cur = fn
        while (cur != null && cur.fromGeneric != null) {
            cur = cur.fromGeneric
        }
        return cur
    }

    def check_style020_from_jv(expr : ExprCall?) : void {
        if (expr.func == null || length(expr.arguments) < 2) return
        // The user writes `from_JV(jv, type<int>, 13)`. After template
        // instantiation (the json_boost overloads are `[template(ent)]`), expr.func
        // is the mangled `template`0x..`json_boost`from_JV`..` and the `ent` witness
        // is stripped — arguments shrink to `(jv, defV)`. The instantiation chain is
        // two levels deep: instantiated → mangled-overload → original `from_JV`.
        let root = root_generic(expr.func)
        let is_from_JV = (root != null && root.name == "from_JV"
            && root._module != null && root._module.name == "json_boost")
        if (!is_from_JV) return
        // Skip lint when current_function is itself a json_boost overload or a generic
        // instantiation of one — the dispatch chain inside JV/from_JV templates calls
        // from_JV recursively on struct fields, which would self-trigger otherwise.
        if (current_function != null) {
            let in_cf = (current_function._module != null
                && current_function._module.name == "json_boost")
            let in_fg = (current_function.fromGeneric != null
                && current_function.fromGeneric._module != null
                && current_function.fromGeneric._module.name == "json_boost")
            return if (in_cf || in_fg)
        }
        // Use the call's result type — it's the scalar (or non-scalar) the user
        // would write on the RHS of `??`. Robust under both pre- and post-instantiation
        // argument shapes.
        if (!is_supported_jv_scalar(expr._type)) return
        style_warning("STYLE020: from_JV(v, type<T>, defV) on a supported scalar — use 'v ?? defV' (operator ?? from json_boost)", expr.at)
    }

    // --- STYLE019: nested min/max → clamp ---

    def is_math_call(expr : Expression? const; fname : string) : bool {
        //! True iff `expr` is `math::<fname>(_, _)` with exactly two arguments
        //! (peeling ExprRef2Value). Conservatively skips any non-math overload.
        let inner = peel_ref2value_const(expr)
        if (inner == null || !(inner is ExprCall)) return false
        let call = inner as ExprCall
        if (call.func == null || call.func.name != fname
            || call.func._module.name != "math") return false
        return length(call.arguments) == 2
    }

    def check_style019_clamp(expr : ExprCall?) : void {
        if (expr.func == null || expr.func._module.name != "math"
            || length(expr.arguments) != 2) return
        let outer_name = string(expr.func.name)
        if (outer_name != "min" && outer_name != "max") return
        let inner_name = outer_name == "min" ? "max" : "min"
        // min(max(x, lo), hi) — inner is arg[0]
        // max(min(x, hi), lo) — inner is arg[0] (mirror)
        if (is_math_call(expr.arguments[0], inner_name)) {
            style_warning("STYLE019: nested {outer_name}({inner_name}(...)) — use 'clamp(x, lo, hi)' from math module", expr.at)
            return
        }
        if (is_math_call(expr.arguments[1], inner_name)) {
            style_warning("STYLE019: nested {outer_name}({inner_name}(...)) — use 'clamp(x, lo, hi)' from math module", expr.at)
        }
    }

    def override preVisitExprInvoke(var expr : ExprInvoke?) : void {
        check_block_pipe(expr.at, expr.arguments)
        check_style028_self_arrow(expr)
    }

    // --- STYLE028: self->method(...) inside a class method ---

    def check_style028_self_arrow(expr : ExprInvoke?) : void {
        //! `self->method(args)` inside a class method body lowers to ExprInvoke
        //! (isInvokeMethod=true) with arguments[0]=ExprField(value=ExprTypeDecl, name=method) —
        //! after type inference the same shape is produced by `self.method(args)` AND by the
        //! compiler's auto-promotion of bare `method(args)` to a method invoke. So the
        //! AST alone cannot distinguish the three forms — source inspection is required.
        //! `expr.at.column` points at the dispatch operator: '-' (of `->`) for the arrow form,
        //! '.' for the dot form, or the start of the bare identifier when there is no receiver.
        //! Confirm `self` precedes that operator before flagging.
        if (expr == null || expr.genFlags.generated || !expr.isInvokeMethod ||
            current_function == null || !current_function.flags.isClassMethod ||
            current_function.fromGeneric != null ||
            empty(expr.arguments)) return
        var arg0 = expr.arguments[0]
        if (arg0 != null && arg0 is ExprRef2Value) {
            arg0 = (arg0 as ExprRef2Value).subexpr
        }
        if (arg0 == null || !(arg0 is ExprField)) return
        let f = arg0 as ExprField
        if (expr.at.fileInfo == null || expr.at.line == 0u) return
        var lineTxt = ""
        get_file_source_line(expr.at.fileInfo, expr.at.line) $(s) {
            lineTxt := s
        }
        let col = expr.at.column
        let llen = length(lineTxt) |> uint
        if (col + 1u >= llen) return
        var is_arrow = false
        peek_data(lineTxt) $(data) {
            is_arrow = int(data[col]) == '-' && int(data[col + 1u]) == '>'
        }
        if (!is_arrow) return
        let prefix = strip_right(slice(lineTxt, 0, col |> int))
        if (!ends_with(prefix, "self")) return
        // Make sure the "self" we matched is a whole word.
        let plen = length(prefix)
        if (plen > 4) {
            var prev_ok = false
            peek_data(prefix) $(pd) {
                let ch = int(pd[plen - 5])
                prev_ok = !(is_alpha(ch) || is_number(ch) || ch == '_')
            }
            if (!prev_ok) return
        }
        style_warning("STYLE028: 'self->{f.name}(...)' on the class instance is optional; drop 'self->' and call '{f.name}(...)' directly (compiler auto-promotes to the same method invoke). Use 'self.{f.name}(...)' if you prefer an explicit receiver", expr.at)
    }


    def override preVisitExprMakeGenerator(var expr : ExprMakeGenerator?) : void {
        check_block_pipe(expr.at, expr.arguments)
    }

    // --- STYLE001/002 for desugared generators ---

    def override preVisitExprMakeStruct(var expr : ExprMakeStruct?) : void {
        st029_record_func(expr.constructor)
        if (!expr.genFlags.generated && !expr.makeStructFlags.isNewClass
                && expr.makeType != null && expr.makeType.isClass) {
            mark_unsafe_in_stack()
        }
        // generator<T>() <| $ { ... } is lowered before lint runs — ExprMakeGenerator
        // is replaced by ExprMakeStruct. Detect via source line inspection.
        if (expr.at.fileInfo == null || expr.at.line == 0u) return
        var src = ""
        get_file_source_line(expr.at.fileInfo, expr.at.line) $(line) {
            src := line
        }
        let col = int(expr.at.column)
        if (col >= length(src) || !starts_with(slice(src, col), "generator<")) return
        let pipe_pos = find(src, "<|", col + 10)
        if (pipe_pos < 0) return
        let after_pipe = strip_left(slice(src, pipe_pos + 2))
        let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
        if (!pipes_block) return
        let arrow_pos = find(after_pipe, "=>")
        let brace_pos = find(after_pipe, "\{")
        let is_lambda_shorthand = arrow_pos >= 0 && (brace_pos < 0 || arrow_pos < brace_pos)
        if (is_lambda_shorthand) return
        if (starts_with(after_pipe, "$()")) {
            style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", expr.at)
        } else {
            style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", expr.at)
        }
    }

    // --- STYLE003: redundant $() on parameterless blocks (all contexts) ---

    def override preVisitExprMakeBlock(var expr : ExprMakeBlock?) : void {
        var blk = expr._block
        if (!(blk is ExprBlock)) return
        var eblk = blk as ExprBlock
        if (!eblk.blockFlags.isClosure || eblk.blockFlags.isLambdaBlock || !empty(eblk.arguments)) return
        let src = source_line_between(expr.at, eblk.at)
        let dollar_pos = find(src, "$()")
        // Skip when there is no $() at all, or when <| pipe is present
        // (STYLE002 already covers the piped form).
        if (dollar_pos < 0 || find(src, "<|") >= 0) return
        // Skip if explicit return type follows: $() : Type { ... }
        let after_dollar = strip_left(slice(src, dollar_pos + 3))
        if (starts_with(after_dollar, ":")) return
        style_warning("STYLE003: remove redundant $() for parameterless block; use $ \{", expr.at)
    }

    // --- STYLE006: string(__rtti) == "..." should use `is` ---
    // --- STYLE018: b == true / b == false / b != true / b != false ---

    def peel_ref2value_const(e : Expression? const) : Expression? const {
        if (e != null && e is ExprRef2Value) return (e as ExprRef2Value.subexpr)
        return e
    }

    def expr_equal_struct(a : Expression? const; b : Expression? const) : bool {
        //! Structural equality via describe(). No purity filter.
        if (a == null || b == null) return false
        return describe(a) == describe(b)
    }

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated
                && call_func_needs_unsafe(expr.func, expr.flags.isForLoopSource)) {
            mark_unsafe_in_stack()
        }
        if (expr.op == "==") {
            if (is_string_rtti_comparison(expr.left) || is_string_rtti_comparison(expr.right)) {
                style_warning("STYLE006: string(__rtti) comparison should use `is` operator; e.g. expr is ExprFoo", expr.at)
            }
        }
        // STYLE018: b == true / b == false / b != true / b != false
        if (expr.op == "==" || expr.op == "!=") {
            let lhs = peel_ref2value_const(expr.left)
            let rhs = peel_ref2value_const(expr.right)
            let lbool = lhs != null && lhs is ExprConstBool
            let rbool = rhs != null && rhs is ExprConstBool
            // Avoid double-flagging `true == false` on both sides — flag only when
            // exactly one side is a bool literal (the other side is the bool var/expr).
            if (lbool != rbool) {
                let const_side = lbool ? lhs : rhs
                let cval = (const_side as ExprConstBool).value
                let positive = (expr.op == "==" && cval) || (expr.op == "!=" && !cval)
                let suggestion = positive ? "b" : "!b"
                style_warning("STYLE018: redundant boolean comparison — use '{suggestion}' directly (drop the '{expr.op} {cval}')", expr.at)
            }
        }
        // STYLE022: foo |= BfT.m  /  foo &= ~BfT.m  on a bitfield variable.
        if (expr.op == "|=" || expr.op == "&=") {
            check_style022_bitfield_set_clear(expr)
        }
        // STYLE023: int_cast(bf & BfT.m) !=/== 0  →  [!]bf.m
        if (expr.op == "==" || expr.op == "!=") {
            check_style023_bitfield_test(expr)
        }
    }

    // --- STYLE022: bitfield-as-field compound-set/clear ---

    def is_bitfield_value_type(t : TypeDecl?) : bool {
        //! True for plain bitfield-typed expressions (not arrays of bitfield).
        return t != null && t.baseType == Type.tBitfield
    }

    def extract_single_bit_name(expr : Expression? const; bf_type : TypeDecl?) : string {
        //! If `expr` resolves to a single named bit of `bf_type`, return its name; else "".
        //! Accepts two shapes:
        //!   - ExprField(value=ExprVar(BfTypeName), name="m") — common when running under
        //!     lint policies (no_optimizations + no_infer_time_folding), which keep the
        //!     source-level field access intact.
        //!   - ExprConstBitfield(value=<single-bit mask>) — what regular compilation
        //!     leaves after folding; map the mask back to the bit name via argNames.
        if (expr == null) return ""
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        if (inner == null) return ""
        if (inner is ExprField) {
            let f = inner as ExprField
            if (f.value != null && f.value is ExprVar) return string(f.name)
            return ""
        }
        if (inner is ExprConstBitfield && bf_type != null) {
            let cb = inner as ExprConstBitfield
            let v = uint64(cb.value)
            // Single-bit mask: power of two, exactly one bit set.
            if (v == 0ul || (v & (v - 1ul)) != 0ul) return ""
            // Map bit index → argNames[index].
            var idx = 0
            var probe = v
            while (probe > 1ul) {
                probe = probe >> 1ul
                idx++
            }
            if (idx < 0 || idx >= length(bf_type.argNames)) return ""
            return string(bf_type.argNames[idx])
        }
        return ""
    }

    def is_zero_int_const(expr : Expression? const) : bool {
        //! True for ExprConstInt/UInt/Int64/UInt64 whose value is zero. The lint
        //! runner disables folding, so `0` / `0u` / `0l` / `0ul` literals stay as
        //! distinct typed const nodes — we accept all four.
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt.value) == 0
        if (expr is ExprConstUInt) return (expr as ExprConstUInt.value) == 0u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64.value) == 0l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64.value) == 0ul
        return false
    }

    def unwrap_int_like_cast(expr : Expression? const) : Expression? const {
        //! Peel one int/uint/int64/uint64 single-arg call. Returns the inner
        //! expression, or null if not such a cast.
        if (expr == null || !(expr is ExprCall)) return null
        let call = expr as ExprCall
        if (call.func == null) return null
        let name = string(call.func.name)
        if ((name != "int" && name != "uint" && name != "int64" && name != "uint64") || length(call.arguments) != 1) return null
        return call.arguments[0]
    }

    def check_style023_bitfield_test(expr : ExprOp2?) : void {
        //! STYLE023: `int_cast(bf & BfT.bit) !=/== 0` collapses to `bf.bit` / `!bf.bit`.
        //! Detect: one side is a typed zero constant; the other side is
        //! `int|uint|int64|uint64(bf & SingleBit)` where `bf` is bitfield-typed
        //! and `SingleBit` is a single named bit of the same bitfield.
        let lhs = peel_ref2value_const(expr.left)
        let rhs = peel_ref2value_const(expr.right)
        let lzero = is_zero_int_const(lhs)
        let rzero = is_zero_int_const(rhs)
        if (lzero == rzero) return    // 0 == 0 and cast == cast both skipped
        let cast_side = lzero ? rhs : lhs
        let inner = unwrap_int_like_cast(cast_side)
        if (inner == null) return
        // inner must be `bf & SingleBit`. Peel ref2value, require ExprOp2 op="&".
        var and_expr = inner
        if (and_expr != null && and_expr is ExprRef2Value) {
            and_expr = (and_expr as ExprRef2Value.subexpr)
        }
        if (and_expr == null || !(and_expr is ExprOp2)) return
        let op2 = and_expr as ExprOp2
        if (op2.op != "&" || op2.left == null || !is_bitfield_value_type(op2.left._type)) return
        let bf_type = op2.left._type
        let bit_name = extract_single_bit_name(op2.right, bf_type)
        if (empty(bit_name)) return
        let positive = expr.op == "!="    // !=0 means "the bit is set"
        let suggestion = positive ? "bf.{bit_name}" : "!bf.{bit_name}"
        style_warning("STYLE023: bitfield test via int-cast can be written directly — use '{suggestion}' instead (drop the cast and the '{expr.op} 0' compare)", expr.at)
    }

    def check_style022_bitfield_set_clear(expr : ExprOp2?) : void {
        if (expr.left == null || !is_bitfield_value_type(expr.left._type)) return
        let bf_type = expr.left._type
        if (expr.op == "|=") {
            // foo |= BfT.m  →  foo.m = true
            let name = extract_single_bit_name(expr.right, bf_type)
            if (empty(name)) return
            style_warning("STYLE022: 'foo |= BfT.{name}' on a bitfield is the bit-set idiom; write 'foo.{name} = true' instead (bitfield-as-field assignment)", expr.at)
        } elif (expr.op == "&=") {
            // foo &= ~BfT.m  →  foo.m = false
            // The folded form is ExprConstBitfield with the complement mask; the
            // un-folded form is ExprOp1(~, ExprField). Cover both.
            var inner = expr.right
            if (inner != null && inner is ExprRef2Value) {
                inner = (inner as ExprRef2Value.subexpr)
            }
            // ExprOp1(~, X): peel the operator, match a single bit on X.
            if (inner != null && inner is ExprOp1) {
                let op1 = inner as ExprOp1
                if (op1.op != "~") return
                let name = extract_single_bit_name(op1.subexpr, bf_type)
                if (empty(name)) return
                style_warning("STYLE022: 'foo &= ~BfT.{name}' on a bitfield is the bit-clear idiom; write 'foo.{name} = false' instead (bitfield-as-field assignment)", expr.at)
                return
            }
            // ExprConstBitfield: must be the complement of a single bit.
            if (inner != null && inner is ExprConstBitfield) {
                let cb = inner as ExprConstBitfield
                let v = uint64(cb.value)
                // Build the valid-bit mask for the bitfield width. Shifting by ≥ 64 is UB,
                // so special-case bitfield64 (length(argNames) == 64) to ~0ul.
                let n = uint64(length(bf_type.argNames))
                let valid_mask = n >= 64ul ? ~0ul : ((1ul << n) - 1ul)
                let complement = (~v) & valid_mask
                // Single-bit complement: complement is a power of two.
                if (complement == 0ul || (complement & (complement - 1ul)) != 0ul) return
                var idx = 0
                var probe = complement
                while (probe > 1ul) {
                    probe = probe >> 1ul
                    idx++
                }
                if (idx < 0 || idx >= length(bf_type.argNames)) return
                let name = string(bf_type.argNames[idx])
                style_warning("STYLE022: 'foo &= ~BfT.{name}' on a bitfield is the bit-clear idiom; write 'foo.{name} = false' instead (bitfield-as-field assignment)", expr.at)
            }
        }
    }

    // --- STYLE005: braces around a single-statement early exit ---
    // --- STYLE010: if (true) should be bare block ---
    // --- STYLE016(b): if (a) { return X } else if (b) { return X } ---
    // --- STYLE017(a): if (cond) return true else return false ---

    def single_terminator_body(body : Expression? const) : Expression? const {
        //! If `body` is a bare ExprReturn/Break/Continue, or a 1-statement ExprBlock
        //! wrapping one, return the inner terminator. Otherwise null.
        if (body == null) return null
        if (body is ExprReturn || body is ExprBreak || body is ExprContinue) return body
        if (body is ExprBlock) {
            let blk = body as ExprBlock
            if (length(blk.list) == 1 && empty(blk.finalList)) {
                let stmt = blk.list[0]
                if (stmt is ExprReturn || stmt is ExprBreak || stmt is ExprContinue) return stmt
            }
        }
        return null
    }

    def terminators_payload_equal(a : Expression? const; b : Expression? const) : bool {
        //! Two terminators have the same payload iff they are the same kind and
        //! their return-subexpr (if any) is structurally equal. Break==Break and
        //! Continue==Continue have no payload.
        if (a == null || b == null) return false
        if (a is ExprBreak) return b is ExprBreak
        if (a is ExprContinue) return b is ExprContinue
        if (a is ExprReturn) {
            if (!(b is ExprReturn)) return false
            let ra = a as ExprReturn
            let rb = b as ExprReturn
            if (ra.subexpr == null && rb.subexpr == null) return true
            if (ra.subexpr == null || rb.subexpr == null) return false
            return expr_equal_struct(ra.subexpr, rb.subexpr)
        }
        return false
    }

    def is_const_bool_return(term : Expression? const; var b_out : bool&) : bool {
        //! True if `term` is `return ExprConstBool(b)`. Writes the bool to b_out.
        if (term == null || !(term is ExprReturn)) return false
        let ret = term as ExprReturn
        let sub = peel_ref2value_const(ret.subexpr)
        if (sub == null || !(sub is ExprConstBool)) return false
        b_out = (sub as ExprConstBool).value
        return true
    }

    def unwrap_inner_ifte(if_false : Expression? const) : ExprIfThenElse? {
        //! Strip a 1-stmt ExprBlock wrapping an ExprIfThenElse, returning the inner if.
        //! Otherwise return the if_false directly cast to ExprIfThenElse?, or null.
        if (if_false == null) return null
        if (if_false is ExprIfThenElse) return (if_false as ExprIfThenElse)
        if (if_false is ExprBlock) {
            let blk = if_false as ExprBlock
            if (length(blk.list) == 1 && empty(blk.finalList)) {
                let stmt = blk.list[0]
                if (stmt is ExprIfThenElse) return (stmt as ExprIfThenElse)
            }
        }
        return null
    }

    def override preVisitExprIfThenElse(ifte : ExprIfThenElse?) : void {
        if (ifte.if_flags.isStatic) return
        // STYLE010: if (true) { ... }
        var cond = ifte.cond
        if (cond is ExprConstBool) {
            var cbool = cond as ExprConstBool
            if (cbool.value && ifte.if_false == null) {
                style_warning("STYLE010: if (true) is always taken; use a bare block instead", ifte.at)
            }
        }
        // Detect terminator in the `then` branch — used by STYLE005, STYLE016(b), STYLE017(a)
        let outer_term = single_terminator_body(ifte.if_true)
        // STYLE005: braces around a single-statement early exit. The parser builds a
        // synthetic ExprBlock for both braceless `if (c) return` and postfix-desugared
        // `return X if (c)`, sharing its LineInfo with the inner terminator. A source-
        // written `{...}` has its own LineInfo (starts at the `{`). So `blk.at != inner.at`
        // ⇔ user wrote braces.
        if (ifte.if_false == null && outer_term != null && ifte.if_true is ExprBlock) {
            let blk = ifte.if_true as ExprBlock
            if (blk.at.line != outer_term.at.line || blk.at.column != outer_term.at.column) {
                style_warning("STYLE005: braces around a single-statement early exit are noise; use 'if (cond) <terminator>' or postfix '<terminator> if (cond)'", ifte.at)
            }
        }
        // STYLE016(b): if (a) { return X } else if (b) { return X }
        if (outer_term != null && ifte.if_false != null) {
            let inner = unwrap_inner_ifte(ifte.if_false)
            if (inner != null && inner.if_false == null) {
                let inner_term = single_terminator_body(inner.if_true)
                if (inner_term != null && terminators_payload_equal(outer_term, inner_term)) {
                    style_warning("STYLE016: adjacent guards leading to identical early-exit can be combined with '||'", ifte.at)
                }
            }
        }
        // STYLE017(a): if (cond) return true else return false (or vice versa)
        if (outer_term != null && ifte.if_false != null) {
            let else_term = single_terminator_body(ifte.if_false)
            if (else_term != null) {
                var b1 = false
                var b2 = false
                if (is_const_bool_return(outer_term, b1) && is_const_bool_return(else_term, b2) && b1 != b2) {
                    let suggestion = b1 ? "return cond" : "return !cond"
                    style_warning("STYLE017: 'if (cond) return {b1}; else return {b2}' — use '{suggestion}' directly", ifte.at)
                }
            }
        }
    }

    // --- STYLE011: variable declaration followed by immediate assignment ---

    def override preVisitExprLetVariable(expr : ExprLet?; var v : VariablePtr; last : bool) : void {
        if (v.init == null && !v.flags.generated && !v.flags.inScope && (current_function == null || current_function.fromGeneric == null)) {
            pending_uninit_vars |> push(v)
        }
        // STYLE024/025: `let v & = unsafe(expr)` — local reference binding to
        // a non-local expression requires unsafe per the compiler. Record the
        // init-expression's `at` so preVisitExpression won't flag the wrap as
        // redundant (the wrap is required by the let-binding, not by the inner
        // expression's shape).
        if (v.init != null && v._type != null && v._type.flags.ref
            && v.init.genFlags.userSaidItsSafe) {
            skip_userSaidItsSafe |> push(v.init)
        }
    }

    def find_assign_var(expr : Expression?) : Variable? {
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        return (inner as ExprVar.variable)
    }

    // --- STYLE016(a): adjacent guards `if (a) { return X }; if (b) { return X }` ---
    // --- STYLE017(b): adjacent `if (cond) { return b1 }; return b2` (b1 != b2) ---

    def override preVisitExprBlock(blk : ExprBlock?) : void {
        if (blk.blockFlags.isClosure) {
            unsafe_block_stack |> push(0)
        }
        check_style012_chain_runs(blk)
        let n = length(blk.list)
        for (i in range(n - 1)) {
            let cur = blk.list[i]
            let nxt = blk.list[i + 1]
            if (!(cur is ExprIfThenElse)) continue
            let outer_if = cur as ExprIfThenElse
            if (outer_if.if_false != null) continue
            let outer_term = single_terminator_body(outer_if.if_true)
            if (outer_term == null) continue
            // STYLE016(a): adjacent guard with same payload
            if (nxt is ExprIfThenElse) {
                let nxt_if = nxt as ExprIfThenElse
                if (nxt_if.if_false == null) {
                    let nxt_term = single_terminator_body(nxt_if.if_true)
                    if (nxt_term != null && terminators_payload_equal(outer_term, nxt_term)) {
                        style_warning("STYLE016: adjacent guards leading to identical early-exit can be combined with '||'", outer_if.at)
                        continue
                    }
                }
            }
            // STYLE017(b): adjacent return-true-then-return-false
            var b1 = false
            var b2 = false
            if (is_const_bool_return(outer_term, b1) && is_const_bool_return(nxt, b2) && b1 != b2) {
                let suggestion = b1 ? "return cond" : "return !cond"
                style_warning("STYLE017: 'if (cond) return {b1}; return {b2}' — use '{suggestion}' directly", outer_if.at)
            }
        }
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            unsafe_block_stack |> pop
        }
        return blk
    }

    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) : void {
        if (!empty(pending_uninit_vars)) {
            if (expr is ExprCopy) {
                let v = find_assign_var((expr as ExprCopy).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            } elif (expr is ExprClone) {
                let v = find_assign_var((expr as ExprClone).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            } elif (expr is ExprMove) {
                let v = find_assign_var((expr as ExprMove).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            }
            pending_uninit_vars |> resize(0)
        }
        if (expr is ExprLet) {
            check_style012_array_init(blk, expr as ExprLet)
            check_style013_struct_field_init(blk, expr as ExprLet)
            check_style021_jv_table_init(blk, expr as ExprLet)
            check_style027_for_loop_comprehension(blk, expr as ExprLet)
        }
    }

    // --- STYLE012: array<T> var then contiguous push/emplace instead of literal ---

    def check_style012_array_init(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        // Find this ExprLet's index in the enclosing block's statement list.
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) return
        for (v in elet.variables) {
            // Only flag uninitialized array<T> locals from user code (skip generic
            // host instantiations and compiler-synthesized vars).
            if (v.init != null
                || v.flags.generated || v.flags.inScope
                || v._type == null || v._type.baseType != Type.tArray) continue
            var count = 0
            for (j in range(idx + 1, n)) {
                let stmt = blk.list[j]
                if (!(stmt is ExprCall) || !is_push_or_emplace_of(stmt as ExprCall, v)) break
                count++
            }
            if (count >= 2) {
                style_warning("STYLE012: array<T> initialized by a run of push/emplace calls; use an array literal `var a <- [x, <-y, z]` (or `var a <- array<T>(x, y, z)` when you need an explicit element type, e.g. for polymorphic upcasts)", v.at)
            }
        }
    }

    // --- STYLE012 (chain form): push/emplace run into `rs.nums` / `fo[13].bar.far` ---

    def check_style012_chain_runs(blk : ExprBlock?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        let n = length(blk.list)
        var i = 0
        while (i < n) {
            let recv = chain_push_receiver(blk.list[i])
            if (recv == null || chain_link_count(recv) < 1) {
                i++
                continue
            }
            var count = 1
            while (i + count < n) {
                let nrecv = chain_push_receiver(blk.list[i + count])
                if (nrecv == null || !chain_equal(recv, nrecv)) break
                count++
            }
            if (count >= 2) {
                emit_style012_chain(blk, i, recv)
            }
            i += count
        }
    }

    def chain_push_receiver(stmt : Expression const?) : Expression const? {
        //! When stmt is `<pure chain> |> push/emplace(value)` on an array<T>,
        //! returns the receiver chain expression; null otherwise. Exactly 2 args —
        //! the 3-arg `push(a, v, at_index)` positional-insert overload must not
        //! collapse into a literal.
        if (stmt == null || stmt.genFlags.generated || !(stmt is ExprCall)) return null
        let call = stmt as ExprCall
        if (call.func == null || call.func.fromGeneric == null
            || (call.func.fromGeneric.name != "push" && call.func.fromGeneric.name != "emplace")
            || length(call.arguments) != 2) return null
        let recv = call.arguments[0]
        if (recv == null || recv._type == null
            || recv._type.baseType != Type.tArray) return null
        return is_pure_chain(recv) ? recv : null
    }

    def peel_chain_adapter(e0 : Expression const?) : Expression const? {
        //! Strips typer-synthesized value adapters (Ref2Value, Ptr2Ref) that wrap
        //! chain links on pointer auto-deref — invisible to the user, safe to peel.
        var e : Expression const? = e0
        while (e != null) {
            if (e is ExprRef2Value) {
                e = (e as ExprRef2Value).subexpr
            } elif (e is ExprPtr2Ref) {
                e = (e as ExprPtr2Ref).subexpr
            } else {
                break
            }
        }
        return e
    }

    def is_pure_chain(e0 : Expression const?) : bool {
        //! Chain grammar: ExprVar root, ExprField links, ExprAt links with a
        //! const-int/uint/string or simple-var index. Anything else (calls, ?.,
        //! computed indexes) is impure — receiver identity across the run can't
        //! be guaranteed, so the run is not collapsible.
        let e = peel_chain_adapter(e0)
        if (e == null) return false
        if (e is ExprVar) return true
        if (e is ExprField) return is_pure_chain((e as ExprField).value)
        if (e is ExprAt) {
            let ea = e as ExprAt
            let idx = peel_chain_adapter(ea.index)
            if (idx == null
                || (!(idx is ExprVar) && !(idx is ExprConstInt)
                    && !(idx is ExprConstUInt) && !(idx is ExprConstString))) return false
            return is_pure_chain(ea.subexpr)
        }
        return false
    }

    def chain_link_count(e0 : Expression const?) : int {
        let e = peel_chain_adapter(e0)
        if (e is ExprField) return 1 + chain_link_count((e as ExprField).value)
        if (e is ExprAt) return 1 + chain_link_count((e as ExprAt).subexpr)
        return 0
    }

    def chain_equal(a0 : Expression const?; b0 : Expression const?) : bool {
        let a = peel_chain_adapter(a0)
        let b = peel_chain_adapter(b0)
        if (a == null || b == null) return false
        if (a is ExprVar) {
            return false if (!(b is ExprVar))
            return (a as ExprVar).variable == (b as ExprVar).variable
        }
        if (a is ExprField) {
            return false if (!(b is ExprField))
            let af = a as ExprField
            let bf = b as ExprField
            return af.name == bf.name && chain_equal(af.value, bf.value)
        }
        if (a is ExprAt) {
            return false if (!(b is ExprAt))
            let aa = a as ExprAt
            let ba = b as ExprAt
            return chain_index_equal(aa.index, ba.index) && chain_equal(aa.subexpr, ba.subexpr)
        }
        return false
    }

    def chain_index_equal(a0 : Expression const?; b0 : Expression const?) : bool {
        let a = peel_chain_adapter(a0)
        let b = peel_chain_adapter(b0)
        if (a == null || b == null) return false
        if (a is ExprVar && b is ExprVar) {
            return (a as ExprVar).variable == (b as ExprVar).variable
        }
        if (a is ExprConstInt && b is ExprConstInt) {
            return (a as ExprConstInt).value == (b as ExprConstInt).value
        }
        if (a is ExprConstUInt && b is ExprConstUInt) {
            return (a as ExprConstUInt).value == (b as ExprConstUInt).value
        }
        if (a is ExprConstString && b is ExprConstString) {
            return (a as ExprConstString).value == (b as ExprConstString).value
        }
        return false
    }

    def chain_describe(e0 : Expression const?) : string {
        let e = peel_chain_adapter(e0)
        if (e is ExprVar) return string((e as ExprVar).name)
        if (e is ExprField) {
            let ef = e as ExprField
            return "{chain_describe(ef.value)}.{ef.name}"
        }
        if (e is ExprAt) {
            let ea = e as ExprAt
            let idx = peel_chain_adapter(ea.index)
            var istr = "?"
            if (idx is ExprVar) {
                istr = string((idx as ExprVar).name)
            } elif (idx is ExprConstInt) {
                istr = "{(idx as ExprConstInt).value}"
            } elif (idx is ExprConstUInt) {
                istr = "{(idx as ExprConstUInt).value}"
            } elif (idx is ExprConstString) {
                istr = "\"{(idx as ExprConstString).value}\""
            }
            return "{chain_describe(ea.subexpr)}[{istr}]"
        }
        return "?"
    }

    def ctor_field_state(v : Variable const?; fname : das_string) : int {
        //! Classifies field `fname` of a just-declared local after construction:
        //! 1 = provably default-empty (struct value, no struct-field initializer,
        //! init is null or an ExprMakeStruct without a where-block that does not
        //! mention the field); 2 = provably initialized (MakeStruct entry or
        //! struct-field initializer → pushes are a deliberate append); 0 = unknown
        //! (pointer root, non-MakeStruct init, where-block).
        var st : Structure const? = null
        if (v._type != null && v._type.baseType == Type.tStructure) {
            st = v._type.structType
        }
        if (st == null) return 0
        for (fld in st.fields) {
            if (fld.name == fname) {
                if (fld.init != null) return 2
                break
            }
        }
        var ms : ExprMakeStruct const? = null
        if (v.init != null) {
            return 0 if (!(v.init is ExprMakeStruct))
            ms = v.init as ExprMakeStruct
            return 0 if (ms._block != null)
        }
        if (ms != null) {
            for (row in ms.structs) {
                for (mf in *row) {
                    if (mf.name == fname) return 2
                }
            }
        }
        return 1
    }

    def emit_style012_chain(blk : ExprBlock?; idx : int; recv : Expression const?) : void {
        let head = blk.list[idx]
        // Fresh-construction fold: chain is `v.field`, v declared in the statement
        // right above. Provably-empty field → fold suggestion; field explicitly
        // initialized → deliberate append, stay silent; unknown → general arm.
        if (idx > 0 && blk.list[idx - 1] is ExprLet) {
            let elet = blk.list[idx - 1] as ExprLet
            let e = peel_chain_adapter(recv)
            if (e is ExprField) {
                let ef = e as ExprField
                let base = peel_chain_adapter(ef.value)
                if (base is ExprVar) {
                    let bvar = (base as ExprVar).variable
                    for (v in elet.variables) {
                        if (v != bvar || v.flags.generated || v.flags.inScope) continue
                        let state = ctor_field_state(v, ef.name)
                        if (state == 1) {
                            let tname = string(v._type.structType.name)
                            style_warning("STYLE012: array field '{ef.name}' of freshly constructed '{v.name}' filled by a run of push/emplace; fold an array literal into the constructor `var {v.name} <- {tname}({ef.name} <- [x, <-y, z], ...)`", head.at)
                            return
                        }
                        if (state == 2) return
                        break
                    }
                }
            }
        }
        let chain = chain_describe(recv)
        style_warning("STYLE012: run of push/emplace into '{chain}'; use an array literal move-assign `{chain} <- [x, <-y, z]` (suppress with nolint when appending to existing content is intended)", head.at)
    }

    // --- STYLE013: struct var with default/empty init then a run of field assignments ---

    def check_style013_struct_field_init(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) return
        for (v in elet.variables) {
            // Skip generic-host instantiations and any non-{struct or struct-pointer}
            // local that isn't a default/empty init we'd want to flag.
            if (v.flags.generated || v.flags.inScope
                || !is_struct_or_struct_ptr(v._type)
                || !is_default_struct_init(v)) continue
            var count = 0
            for (j in range(idx + 1, n)) {
                if (!is_field_assign_of(blk.list[j], v)) break
                count++
            }
            if (count >= 2) {
                // Collect actual field names from the assignment run to make the
                // suggestion concrete (`var a = Foo(x = ..., y = ...)` instead of
                // a generic `bar/far` placeholder).
                var field_names : array<string>
                for (j in range(idx + 1, idx + 1 + count)) {
                    let name = field_name_of(blk.list[j])
                    if (!empty(name)) field_names |> push(name)
                }
                let is_new = v.init != null && v.init is ExprNew
                let struct_type = is_new ? v._type.firstType : v._type
                let type_name = (struct_type != null && struct_type.structType != null) ? string(struct_type.structType.name) : ""
                let ctor_prefix = is_new ? "new " : ""
                let init_desc = is_new ? "zero-arg `new`" : "default-init or uninitialized"
                let msg = build_string() $(var w) {
                    w |> write("STYLE013: var ({init_desc}) followed by assignments to fields \{")
                    for (k in range(length(field_names))) {
                        if (k != 0) w |> write(", ")
                        w |> write(field_names[k])
                    }
                    w |> write("\}; use a named-argument constructor `var {v.name} = {ctor_prefix}{type_name}(")
                    for (k in range(length(field_names))) {
                        if (k != 0) w |> write(", ")
                        w |> write("{field_names[k]} = ...")
                    }
                    w |> write(")`")
                }
                style_warning(msg, v.at)
            }
        }
    }

    def is_struct_or_struct_ptr(t : TypeDecl?) : bool {
        //! Match struct values AND pointers-to-struct, both flat (not array).
        if (t == null) return false
        let is_struct_val = t.baseType == Type.tStructure
        let is_struct_ptr = (t.baseType == Type.tPointer && t.firstType != null
                && t.firstType.baseType == Type.tStructure)
        return is_struct_val || is_struct_ptr
    }

    def is_default_struct_init(v : Variable?) : bool {
        //! True for `var a : Foo` (no init, struct-valued ONLY — pointer null-init
        //! is a different bug and is NOT flagged here), `var a = Foo()` (zero-arg
        //! MakeStruct), or `var a = new Foo()` / `var a = new Foo` (zero-arg
        //! ExprNew). False for any non-empty constructor, `make_foo()` call, or
        //! pipe chain — those are "modify an existing value" cases.
        // Pointer null-init (`var a : Foo?`) is a guaranteed NPE on `a.x = 1` —
        // a different bug; only flag uninitialized STRUCT values, not pointers.
        if (v.init == null) return v._type != null && v._type.baseType == Type.tStructure
        if (v.init is ExprMakeStruct) {
            var ms = v.init as ExprMakeStruct
            if (ms._block != null) return false
            for (row in ms.structs) {
                for (_mf in *row) {
                    // any field present → not the zero-arg constructor
                    return false
                }
            }
            return true
        }
        if (v.init is ExprNew) {
            var en = v.init as ExprNew
            return empty(en.arguments)
        }
        return false
    }

    def is_field_assign_of(stmt : Expression?; target : Variable?) : bool {
        var lhs : Expression? = null
        if (stmt is ExprCopy) {
            lhs = (stmt as ExprCopy).left
        } elif (stmt is ExprClone) {
            lhs = (stmt as ExprClone).left
        } elif (stmt is ExprMove) {
            lhs = (stmt as ExprMove).left
        } else {
            return false
        }
        return false if (lhs == null || !(lhs is ExprField))
        var src = (lhs as ExprField).value
        // Pointer auto-deref `ptr.field` lowers to ExprField(value=ExprRef2Value(ExprVar(ptr)))
        // — the typer-synthesized Ref2Value is invisible to users and safe to peel.
        if (src != null && src is ExprRef2Value) {
            src = (src as ExprRef2Value).subexpr
        }
        return false if (src == null || !(src is ExprVar))
        return (src as ExprVar).variable == target
    }

    def field_name_of(stmt : Expression?) : string {
        //! Return the ExprField name of stmt's LHS, or "" if stmt isn't a field
        //! assignment. Caller is expected to have already passed `is_field_assign_of`.
        //! Same Ptr2Ref-peel as is_field_assign_of, since pointer auto-deref wraps
        //! the field-receiver in ExprPtr2Ref.
        var lhs : Expression? = null
        if (stmt is ExprCopy) {
            lhs = (stmt as ExprCopy).left
        } elif (stmt is ExprClone) {
            lhs = (stmt as ExprClone).left
        } elif (stmt is ExprMove) {
            lhs = (stmt as ExprMove).left
        } else {
            return ""
        }
        return "" if (lhs == null || !(lhs is ExprField))
        return string((lhs as ExprField).name)
    }

    // --- STYLE021: table<string; JsonValue?> built via repeated insert → JV(named-tuple) ---

    def is_jv_pointer_type(t : TypeDecl?) : bool {
        if (t == null || t.baseType != Type.tPointer || t.firstType == null
                || t.firstType.baseType != Type.tHandle
                || t.firstType.annotation == null) return false
        return t.firstType.annotation.name == "JsonValue"
    }

    def is_jv_table_type(t : TypeDecl?) : bool {
        //! Matches `table<string; JsonValue?>` (or any pointer-to-JsonValue value type).
        if (t == null || t.baseType != Type.tTable
                || t.firstType == null
                || t.firstType.baseType != Type.tString) return false
        return is_jv_pointer_type(t.secondType)
    }

    def is_insert_of_const_key(call : ExprCall?; target : Variable?) : bool {
        //! True when `call` is `target |> insert(<const string>, <any>)` — i.e.
        //! pipe-lowered to a 3-arg insert call on this exact variable, with a
        //! constant string key. Computed keys disqualify the chain.
        if (call.func == null || call.func.fromGeneric == null
            || call.func.fromGeneric.name != "insert"
            || length(call.arguments) < 3) return false
        var recv = call.arguments[0]
        if (recv is ExprRef2Value) {
            recv = (recv as ExprRef2Value.subexpr)
        }
        if (recv == null || !(recv is ExprVar)
                || (recv as ExprVar).variable != target) return false
        var key = call.arguments[1]
        if (key is ExprRef2Value) {
            key = (key as ExprRef2Value.subexpr)
        }
        return key != null && key is ExprConstString
    }

    def check_style021_jv_table_init(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        // Find this ExprLet's index in the enclosing block's statement list.
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) return
        for (v in elet.variables) {
            // Only flag uninitialized table<string; JsonValue?> locals from user code.
            if (v.init != null
                || v.flags.generated || v.flags.inScope
                || !is_jv_table_type(v._type)) continue
            var count = 0
            for (j in range(idx + 1, n)) {
                let stmt = blk.list[j]
                if (!(stmt is ExprCall) || !is_insert_of_const_key(stmt as ExprCall, v)) break
                count++
            }
            if (count >= 2) {
                style_warning("STYLE021: table<string; JsonValue?> built by repeated insert calls; use 'var v = JV((k1=..., k2=..., ...))' (named-tuple JV form from json_boost)", v.at)
            }
        }
    }

    // --- STYLE027: var array<T>/table<K;V> followed by for-loop push/insert → comprehension ---

    def check_style027_for_loop_comprehension(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) return
        let nextStmt = blk.list[idx + 1]
        if (!(nextStmt is ExprFor)) return
        let forE = nextStmt as ExprFor
        for (v in elet.variables) {
            if (v.init != null
                || v.flags.generated || v.flags.inScope
                || v._type == null) continue
            let isArray = v._type.baseType == Type.tArray
            let isTable = v._type.baseType == Type.tTable
            if ((!isArray && !isTable)
                || !for_loop_matches_comprehension(forE, v, isArray, 1)) continue
            let kindLabel = isArray ? "array<T>" : "table<K;V>"
            let rewriteForm = (isArray
                ? "var {v.name} <- [for (x in SRC); EXPR; where COND]"
                : "var {v.name} <- \{for (x in SRC); KEY => VAL; where COND\}")
            style_warning("STYLE027: var {kindLabel} '{v.name}' followed by a for-loop that only push/insert into it; use a comprehension `{rewriteForm}` (drop `; where COND` when there is no filter)", v.at)
        }
    }

    def for_loop_matches_comprehension(forE : ExprFor?; target : Variable?;
                                       isArray : bool; for_budget : int) : bool {
        //! Recursively verifies forE's body is built solely from pushes/inserts/at-assigns
        //! into `target`, optionally wrapped in if-filters and nested-for (up to `for_budget`
        //! additional nested ExprFor levels). Returns false on any side-effecting statement
        //! or unrelated shape, so the all-or-nothing structure guarantees the suggested
        //! comprehension rewrite is semantically equivalent.
        if (forE == null || forE.genFlags.generated
            || forE.body == null || !(forE.body is ExprBlock)) return false
        let blk = forE.body as ExprBlock
        if (length(blk.list) != 1) return false
        return stmt_matches_in_for(blk.list[0], target, isArray, for_budget)
    }

    def stmt_matches_in_for(stmt : Expression?; target : Variable?;
                            isArray : bool; for_budget : int) : bool {
        if (stmt == null || stmt.genFlags.generated) return false
        if (stmt is ExprIfThenElse) {
            let ifte = stmt as ExprIfThenElse
            if (ifte.if_false != null || ifte.if_flags.isStatic
                || ifte.if_true == null || !(ifte.if_true is ExprBlock)) return false
            let tb = ifte.if_true as ExprBlock
            if (length(tb.list) != 1) return false
            return stmt_matches_in_for(tb.list[0], target, isArray, for_budget)
        }
        if (stmt is ExprFor) {
            if (for_budget <= 0) return false
            return for_loop_matches_comprehension(stmt as ExprFor, target, isArray, for_budget - 1)
        }
        if (isArray) {
            if (!(stmt is ExprCall)) return false
            let call = stmt as ExprCall
            return (is_named_target_call(call, target, 2, "push")
                || is_named_target_call(call, target, 2, "push_clone"))
        }
        if (stmt is ExprCall && is_named_target_call(stmt as ExprCall, target, 3, "insert")) return true
        return is_at_assign_of(stmt, target)
    }

    def is_named_target_call(call : ExprCall?; target : Variable?;
                             min_args : int; fname : string) : bool {
        //! Pipe-lowered call shape: `target |> fname(...)` resolves to fname(target, ...).
        //! Returns true when call's func.fromGeneric.name == fname, arg count >= min_args,
        //! and arg[0] (after one ExprRef2Value peel) is ExprVar(target).
        if (call == null || call.func == null || call.func.fromGeneric == null
            || call.func.fromGeneric.name != fname
            || length(call.arguments) < min_args) return false
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (!(arg is ExprVar)) return false
        return (arg as ExprVar).variable == target
    }

    def is_at_assign_of(stmt : Expression?; target : Variable?) : bool {
        //! True when stmt is `target[<key>] = <expr>` — ExprCopy/Clone/Move whose LHS is
        //! ExprAt with subexpr resolving (after one ExprRef2Value peel) to ExprVar(target).
        var lhs : Expression? = null
        if (stmt is ExprCopy) {
            lhs = (stmt as ExprCopy).left
        } elif (stmt is ExprClone) {
            lhs = (stmt as ExprClone).left
        } elif (stmt is ExprMove) {
            lhs = (stmt as ExprMove).left
        } else {
            return false
        }
        return false if (lhs == null || !(lhs is ExprAt))
        var src = (lhs as ExprAt).subexpr
        if (src != null && src is ExprRef2Value) {
            src = (src as ExprRef2Value).subexpr
        }
        return false if (src == null || !(src is ExprVar))
        return (src as ExprVar).variable == target
    }

    def is_push_or_emplace_of(call : ExprCall?; target : Variable?) : bool {
        // Exactly 2 args — the 3-arg `push(a, v, at_index)` positional-insert
        // overload must not collapse into a literal.
        if (call.func == null || call.func.fromGeneric == null
            || (call.func.fromGeneric.name != "push" && call.func.fromGeneric.name != "emplace")
            || length(call.arguments) != 2) return false
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (!(arg is ExprVar)) return false
        return (arg as ExprVar).variable == target
    }

    // --- STYLE014/STYLE015: large comment blocks (post-visit pass) ---

    // ===== STYLE029 — transitive-only require =====

    def st029_expr_in_scope() : bool {
        // Record refs for any code belonging to the linted (root) module — INCLUDING
        // generated functions (class methods desugar to generated funcs; macro-
        // generated code holds real uses too). Excluding generated here would miss
        // those uses and mis-report the require as unused.
        // current_function == null means a global/constant initializer — record
        // those too (uses there keep a require live). Over-recording at global
        // scope is the safe direction for an unused-require check.
        return (analyze_requires
            && (current_function == null || current_function._module == this_module))
    }

    def st029_note_module(m : Module const?) : void {
        // Unconditional insert — the caller has already scoped (decl._module for
        // declarations, expr_in_scope for expressions via st029_record_mod).
        st029_note_module_name(m == null ? "" : "{m.name}")
    }

    def st029_note_module_name(n : string) : void {
        if (this_module == null || n == "" || n == "$" || n == "builtin" || n == "{this_module.name}") return
        if (!key_exists(used_modules, n)) used_modules |> insert(n)
    }

    def st029_record_module_getter(expr : ExprCall?) : void {
        // get_module("X") / find_module("X") / has_module("X") load X for runtime
        // reflection — a real use of X even though no X symbol is referenced.
        // (das2rst's document_module_* are the canonical case.)
        if (expr.func == null || !st029_expr_in_scope() || empty(expr.arguments)) return
        // find_module is [generic] — match on the template name, not the instance.
        let fn = expr.func.fromGeneric != null ? "{expr.func.fromGeneric.name}" : "{expr.func.name}"
        if (fn != "get_module" && fn != "find_module" && fn != "has_module" && fn != "find_compiling_module") return
        let arg0 = expr.arguments[0]
        if (arg0 != null && arg0 is ExprConstString) {
            st029_note_module_name(string((arg0 as ExprConstString).value))
        }
    }

    def st029_record_mod(m : Module const?) : void {
        // Expression/type context: record only refs from the linted module's code.
        if (st029_expr_in_scope()) st029_note_module(m)
    }

    def st029_record_func(fun : Function?) : void {
        return if (fun == null)
        // Generic instances point _module at the instantiating module; the
        // symbol's real origin is fromGeneric._module.
        if (fun.fromGeneric != null) st029_record_mod(fun.fromGeneric._module)
        else st029_record_mod(fun._module)
    }

    def st029_record_unresolved_call(expr : ExprCall?) : void {
        // Calls inside generic bodies that never instantiate stay unresolved
        // (func==null); keep their unqualified names so STYLE030 can match them
        // against a required module's exports instead of flagging it unused.
        if (expr.func != null || !st029_expr_in_scope()) return
        var nm = "{expr.name}"
        let sep = nm |> rfind("::")   // strip any module qualification, e.g. `_::foo`
        if (sep >= 0) {
            nm = nm |> slice(sep + 2)
        }
        if (!empty(nm) && !key_exists(unresolved_call_names, nm)) {
            unresolved_call_names |> insert(nm)
        }
    }

    def st030_exports_unresolved_name(mod : Module?) : bool {
        // True when `mod` — or a module it publicly re-exports (e.g. daslib/rtti
        // re-exporting the builtin rtti module) — exports a function or generic whose
        // name matches an unresolved generic-body call; the require is (potentially)
        // load-bearing for instantiations the lint pass never sees.
        if (empty(unresolved_call_names)) return false
        var seen : table<string>
        return st030_exports_unresolved_rec(mod, seen)
    }

    def st030_exports_unresolved_rec(mod : Module?; var seen : table<string>) : bool {
        var found = false
        for_each_function(mod, "") $(fn) {
            if (found) return
            if (key_exists(unresolved_call_names, "{fn.name}")) {
                found = true
            }
        }
        if (found) return true
        for_each_generic(mod, "") $(fn) {
            if (found) return
            if (key_exists(unresolved_call_names, "{fn.name}")) {
                found = true
            }
        }
        if (found) return true
        module_for_each_dependency(mod) $(dep, is_pub) {
            if (found || !is_pub || dep == null) return
            let dn = "{dep.name}"
            return if (dn == "" || dn == "$" || dn == "builtin" || key_exists(seen, dn))
            seen |> insert(dn)
            if (st030_exports_unresolved_rec(dep, seen)) {
                found = true
            }
        }
        return found
    }

    def override preVisitTypeDecl(typ : TypeDeclPtr) : void {
        // Type references — st029_record_mod self-gates on expr_in_scope (global
        // scope or this module's code; never a foreign function body).
        if (typ.baseType == Type.tStructure && typ.structType != null) st029_record_mod(typ.structType._module)
        elif (typ.baseType == Type.tEnumeration && typ.enumType != null) st029_record_mod(typ.enumType._module)
        elif (typ.baseType == Type.tHandle && typ.annotation != null) st029_record_mod(typ.annotation._module)
    }

    def override preVisitStructure(var st : StructurePtr) : void {
        // Declaration context: scoped by the struct's own module, so use the
        // un-gated note_module (current_function is unrelated here).
        if (analyze_requires && st._module == this_module) {
            if (st.parent != null) st029_note_module(st.parent._module)
            for (a in st.annotations) {
                if (a != null && a.annotation != null) st029_note_module(a.annotation._module)
            }
        }
    }

    def override preVisitEnumeration(var en : EnumerationPtr) : void {
        if (analyze_requires && en._module == this_module) {
            for (a in en.annotations) {
                if (a != null && a.annotation != null) st029_note_module(a.annotation._module)
            }
        }
    }

    def st029_module_provides_macro(mod : Module?) : bool {
        // Pessimistic guard: a required module that registers any compile-time
        // macro fires by syntax and leaves no traceable AST node, so we cannot
        // see its uses. Treat it as used rather than risk a false positive.
        var found = false
        for_each_call_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_reader_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_variant_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_for_loop_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_typeinfo_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_typemacro(mod) $(_) { found = true; }
        if (found) return true
        // Pass macros (infer / optimization / lint) leave no symbol ref either —
        // this is what keeps `require daslib/style_lint` (a [lint_macro] module)
        // from being flagged unused.
        for_each_pass_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_capture_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_simulate_macro(mod) $(_) { found = true; }
        if (found) return true
        // Function/tag macros ([tag_function_macro], e.g. macro_boost's macro_verify)
        // are FunctionAnnotations that rewrite calls away at compile time, erasing
        // the reference. Handle/struct annotations (fio's FILE) are NOT counted.
        for_each_function_annotation(mod) $(_) { found = true; }
        if (found) return true
        // A [comment_reader] (e.g. daslib/rst_comment) processes //! doc-comments
        // at compile time, also leaving no symbol reference.
        return module_has_comment_reader(mod)
    }

    def st029_module_has_init(mod : Module?) : bool {
        // Modules with an [init] function run side effects on load even with no
        // visible reference, so they are never "unused".
        var found = false
        for_each_function(mod, "") $(fn) {
            if (found) return
            for (a in fn.annotations) {
                if (a != null && a.annotation != null && a.annotation.name == "init") {
                    found = true
                    break
                }
            }
        }
        return found
    }

    def st029_collect_public_deps(mod : Module?; var exported : table<string; bool>) : void {
        module_for_each_dependency(mod) $(dep, is_pub) {
            if (!is_pub || dep == null) return
            let dn = "{dep.name}"
            if (dn == "" || dn == "$" || dn == "builtin") return
            if (!key_exists(exported, dn)) {
                exported |> insert(dn, "{dep.fileName}" != "")
                st029_collect_public_deps(dep, exported)
            }
        }
    }

    def st029_check_requires(prog : ProgramPtr) : void {
        if (!analyze_requires || this_module == null) return
        let this_name = "{this_module.name}"
        // Source files that belong to the linted module — used to ignore require
        // declarations issued from other source-compiled modules. Seed with the
        // module's own fileName so a file with no functions/structures (only
        // requires + enums/globals/typedefs) is still covered.
        var module_files : table<string>
        let primary = string(this_module.fileName)
        if (!empty(primary)) module_files |> insert(primary)
        for_each_function(this_module, "") $(fn) {
            if (fn.at.fileInfo != null && fn._module == this_module) {
                let fname = string(fn.at.fileInfo.name)
                if (!key_exists(module_files, fname)) module_files |> insert(fname)
            }
        }
        for_each_structure(this_module) $(st) {
            if (st.at.fileInfo != null && st._module == this_module) {
                let fname = string(st.at.fileInfo.name)
                if (!key_exists(module_files, fname)) module_files |> insert(fname)
            }
        }
        for_each_enumeration(this_module) $(en) {
            if (en.at.fileInfo != null && en._module == this_module) {
                let fname = string(en.at.fileInfo.name)
                if (!key_exists(module_files, fname)) module_files |> insert(fname)
            }
        }
        prog |> for_each_require_declaration() $ [unused_argument(file)] (mod, req_name, file, pub, at) {
            if (mod == null || pub
                || at.fileInfo == null || !key_exists(module_files, string(at.fileInfo.name))) return
            let mname = "{mod.name}"
            if (mname == "" || mname == this_name || mname == "$" || mname == "builtin"
                || key_exists(used_modules, mname)
                || st029_module_provides_macro(mod)
                || st029_module_has_init(mod)
                || st030_exports_unresolved_name(mod)) return
            var closure : table<string; bool>
            st029_collect_public_deps(mod, closure)
            var via : array<string>
            var used_any = false
            for (cm in keys(closure)) {
                if (!key_exists(used_modules, cm)) continue
                used_any = true
                if (closure?[cm] ?? false) via |> push(cm)
            }
            var line_at = at
            if (!empty(via)) {
                sort(via)
                let mods = build_string() $(w) {
                    for (i in range(length(via))) {
                        if (i > 0) { w |> write(", ") }
                        w |> write(via[i])
                    }
                }
                style_warning("STYLE029: require {req_name} is used only for modules it re-exports ({mods}); require those directly and drop {req_name}", line_at)
                return
            }
            // Skip when only builtin-facade re-exports are used (legitimate).
            return if (used_any)
            // Nothing direct, nothing transitive, no macro/[init] — genuinely unused.
            style_warning("STYLE030: require {req_name} is unused — no symbol from it (or a module it re-exports) is referenced; drop it", line_at)
        }
    }

    def scan_long_comment_blocks(prog : ProgramPtr) : void {
        let mod = prog.getThisModule
        if (mod == null) return
        var file_first_decl : table<string; uint>
        var file_max_line : table<string; uint>
        var fn_file : array<string>
        var fn_starts : array<uint>
        var fn_ends : array<uint>
        var fn_private : array<bool>
        for_each_function(mod, "") $(var fn) {
            if (fn.body == null || fn.at.fileInfo == null || fn.at.line == 0u
                || fn.flags.generated || fn.fromGeneric != null
                || fn._module != mod || fn.moreFlags.isTemplate) return
            let fname = string(fn.at.fileInfo.name)
            let s = fn.at.line
            let e = fn.body.at.last_line
            if (file_first_decl |> key_exists(fname)) {
                if (s < file_first_decl[fname]) {
                    file_first_decl[fname] = s
                }
                if (e > file_max_line[fname]) {
                    file_max_line[fname] = e
                }
            } else {
                file_first_decl |> insert(fname, s)
                file_max_line |> insert(fname, e)
            }
            fn_file |> push(fname)
            fn_starts |> push(s)
            fn_ends |> push(e)
            fn_private |> push(fn.flags.privateFunction)
        }
        for_each_structure(mod) $(var st) {
            if (st.at.fileInfo == null || st.at.line == 0u
                || st.flags.generated || st._module != mod || st.flags.isTemplate) return
            var is_template_instance = false
            peek(st.name) $(name) {
                is_template_instance = find(name, "<") >= 0
            }
            if (is_template_instance) return
            let fname = string(st.at.fileInfo.name)
            let s = st.at.line
            if (file_first_decl |> key_exists(fname)) {
                if (s < file_first_decl[fname]) {
                    file_first_decl[fname] = s
                }
                if (s > file_max_line[fname]) {
                    file_max_line[fname] = s
                }
            } else {
                file_first_decl |> insert(fname, s)
                file_max_line |> insert(fname, s)
            }
        }
        for_each_enumeration(mod) $(var en) {
            if (en.at.fileInfo == null || en.at.line == 0u) return
            let fname = string(en.at.fileInfo.name)
            let s = en.at.line
            if (file_first_decl |> key_exists(fname)) {
                if (s < file_first_decl[fname]) {
                    file_first_decl[fname] = s
                }
                if (s > file_max_line[fname]) {
                    file_max_line[fname] = s
                }
            } else {
                file_first_decl |> insert(fname, s)
                file_max_line |> insert(fname, s)
            }
        }
        for_each_global(mod) $(var v) {
            if (v.at.fileInfo == null || v.at.line == 0u || v.flags.generated) return
            let fname = string(v.at.fileInfo.name)
            let s = v.at.line
            if (file_first_decl |> key_exists(fname)) {
                if (s < file_first_decl[fname]) {
                    file_first_decl[fname] = s
                }
                if (s > file_max_line[fname]) {
                    file_max_line[fname] = s
                }
            } else {
                file_first_decl |> insert(fname, s)
                file_max_line |> insert(fname, s)
            }
        }
        for_each_typedef(mod) $(_name, value) {
            if (value.at.fileInfo == null || value.at.line == 0u) return
            let fname = string(value.at.fileInfo.name)
            let s = value.at.line
            if (file_first_decl |> key_exists(fname)) {
                if (s < file_first_decl[fname]) {
                    file_first_decl[fname] = s
                }
                if (s > file_max_line[fname]) {
                    file_max_line[fname] = s
                }
            } else {
                file_first_decl |> insert(fname, s)
                file_max_line |> insert(fname, s)
            }
        }
        var seen : table<string; bool>
        for_each_function(mod, "") $(var fn) {
            if (fn.at.fileInfo == null || fn._module != mod
                || fn.flags.generated || fn.fromGeneric != null || fn.moreFlags.isTemplate) return
            let fname = string(fn.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(fn.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_structure(mod) $(var st) {
            if (st.at.fileInfo == null || st._module != mod
                || st.flags.generated || st.flags.isTemplate) return
            var is_template_instance = false
            peek(st.name) $(name) {
                is_template_instance = find(name, "<") >= 0
            }
            if (is_template_instance) return
            let fname = string(st.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(st.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_enumeration(mod) $(var en) {
            if (en.at.fileInfo == null) return
            let fname = string(en.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(en.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_global(mod) $(var v) {
            if (v.at.fileInfo == null || v.flags.generated) return
            let fname = string(v.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(v.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_typedef(mod) $(_name, value) {
            if (value.at.fileInfo == null) return
            let fname = string(value.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(value.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
    }

    def scan_file_lines(sample_at : LineInfo; file_name : string;
                        first_decl, max_line : uint;
                        fn_file : array<string>; fn_starts, fn_ends : array<uint>;
                        fn_private : array<bool>) : void {
        var ln : uint = 1u
        var in_block = false
        var block_start : uint = 0u
        var block_count = 0
        var block_first_line = ""
        while (true) {
            var got = false
            var line_text = ""
            get_file_source_line(sample_at.fileInfo, ln) $(s) {
                got = true
                line_text = string(s)
            }
            if (!got) break
            let trimmed = strip_left(line_text)
            let is_comment = starts_with(trimmed, "//")
            if (is_comment) {
                if (!in_block) {
                    in_block = true
                    block_start = ln
                    block_count = 0
                    block_first_line = trimmed
                }
                block_count++
            } else {
                if (in_block) {
                    emit_long_comment(sample_at, file_name, first_decl, block_start, block_count, block_first_line,
                        fn_file, fn_starts, fn_ends, fn_private)
                    in_block = false
                }
            }
            ln++
        }
        if (in_block) {
            emit_long_comment(sample_at, file_name, first_decl, block_start, block_count, block_first_line,
                fn_file, fn_starts, fn_ends, fn_private)
        }
    }

    def has_nolint_directive(first_line : string) : bool {
        // Positional check — `@nolint` MUST be the first token after `//`/`//!`
        // (and optional whitespace) so we don't mis-match URLs/prose that
        // happen to contain the literal `@nolint`.
        var rest = first_line
        if (starts_with(rest, "//!")) {
            rest = slice(rest, 3)
        } elif (starts_with(rest, "//")) {
            rest = slice(rest, 2)
        } else {
            return false
        }
        rest = strip_left(rest)
        return starts_with(rest, "@nolint")
    }

    def emit_long_comment(sample_at : LineInfo; file_name : string;
                          first_decl_line, block_start : uint; line_count : int;
                          first_line_text : string;
                          fn_file : array<string>; fn_starts, fn_ends : array<uint>;
                          fn_private : array<bool>) : void {
        // Skip when the block is too short, lives before the first AST decl
        // (module-leading docstring), or carries an explicit @nolint marker.
        if (line_count <= 1
            || block_start < first_decl_line
            || has_nolint_directive(first_line_text)) return
        var enclosing_priv = false
        var inside_function = false
        var enclosing_size : uint = 0xFFFFFFFFu
        for (i in range(length(fn_file))) {
            if (fn_file[i] != file_name) continue
            let s = fn_starts[i]
            let e = fn_ends[i]
            if (s <= block_start && block_start <= e) {
                let size = e - s
                if (size < enclosing_size) {
                    enclosing_size = size
                    enclosing_priv = fn_private[i]
                    inside_function = true
                }
            }
        }
        let private_ctx = inside_function && enclosing_priv
        let threshold = private_ctx ? 1 : 3
        if (line_count <= threshold) return
        var at = sample_at
        at.line = block_start
        at.last_line = block_start
        at.column = 1u
        at.last_column = 1u
        var msg = ""
        if (private_ctx) {
            msg = "STYLE015: {line_count}-line comment inside 'def private' exceeds 1-line cap; private symbols don't get public docs — keep at most one WHY line, or suppress on the first line with '// nolint:STYLE015'"
        } else {
            msg = "STYLE014: comment block of {line_count} lines exceeds 3-line cap; trim multi-paragraph prose to a 1-line WHY (move design notes to .md), or suppress on the first line with '//!@nolint' (//! blocks) or '// nolint:STYLE014' (// blocks)"
        }
        style_warning(msg, at)
    }

    // --- helpers ---

    def is_string_rtti_comparison(expr : Expression?) : bool {
        //! Returns true if expr is string(x.__rtti) pattern.
        return false if (expr == null)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return false if (inner == null || !(inner is ExprCall))
        var call = inner as ExprCall
        if (call.name != "string" || empty(call.arguments)) return false
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (arg is ExprField) {
            var field = arg as ExprField
            return field.name == "__rtti"
        }
        return false
    }

    // --- STYLE024/STYLE025: redundant or over-broad `unsafe` ---

    def unsafe_frame_for(expr : Expression?) : UnsafeFrame {
        return UnsafeFrame() if (expr == null)
        return unsafeExprs |> key_exists(expr) ? unsafeExprs[expr] : UnsafeFrame()
    }

    def mark_unsafe_in_stack() : void {
        //! Add 1 to the current top of `unsafe_stack`.
        let n = length(unsafe_stack)
        if (n > 0) {
            unsafe_stack[n - 1].count++
        }
    }

    def mark_non_local_let_ref_in_stack() : void {
        //! Flag the current top of `unsafe_stack` as containing a non-local
        //! let-ref binding. Propagates upward through `visitExpression`.
        let n = length(unsafe_stack)
        if (n > 0) {
            unsafe_stack[n - 1].has_non_local_let_ref = true
        }
    }

    def override preVisitExpression(expr : ExpressionPtr) : void {
        //! Push a fresh frame for this node.
        unsafe_stack |> push(UnsafeFrame())
    }

    def override visitExpression(var expr : ExpressionPtr) : ExpressionPtr {
        //! Pop the slot for this node. The popped frame is the subtree's
        //! summary. Use it to:
        //!   - store `unsafeExprs[expr] = frame` for later queries
        //!     (used by `visitExprUnsafe` to decide STYLE024 vs STYLE025),
        //!   - propagate count + non-local-let-ref flag to the parent slot,
        //!   - fire STYLE024 if `expr` is an `unsafe(...)` wrap target
        //!     (`userSaidItsSafe`) and its subtree had count 0.
        let n = length(unsafe_stack)
        let frame = unsafe_stack[n - 1]
        unsafe_stack |> pop
        if (frame.count > 0 || frame.has_non_local_let_ref) {
            unsafeExprs |> insert(expr, frame)
            let m = length(unsafe_stack)
            if (m > 0) {
                unsafe_stack[m - 1].count = unsafe_stack[m - 1].count + frame.count
                if (frame.has_non_local_let_ref) {
                    unsafe_stack[m - 1].has_non_local_let_ref = true
                }
            }
        }
        // STYLE024 expression form: parser sets `userSaidItsSafe` on the
        // inner expr of `unsafe(expr)`. Suppress when the wrap is required
        // by an enclosing `let x & = ...` ref-binding.
        if (expr.genFlags.userSaidItsSafe && !expr.genFlags.generated
            && (current_function == null || current_function.fromGeneric == null)
            && !(skip_userSaidItsSafe |> has_value(expr))
            && frame.count == 0) {
            style_warning("STYLE024: redundant 'unsafe(...)' wrap; the inner expression has no operation that requires unsafe — drop the wrap", expr.at)
        }
        return expr
    }

    def override preVisitExprLet(expr : ExprLet?) : void {
        //! Mirror `isLocalOrGlobal` check from ast_infer_type.cpp:4989 —
        //! `let v & = E` requires unsafe at statement level when `E` is
        //! non-local-non-temporary. Mark the let's frame so the enclosing
        //! `unsafe { ... }` block can detect it (STYLE025 must stay silent
        //! when narrowing would leave the let-ref binding unsatisfied).
        if (expr.genFlags.generated) return
        for (v in expr.variables) {
            continue if (
                v._type == null ||
                !v._type.flags.ref ||
                v.init == null ||
                v.init.genFlags.alwaysSafe ||
                (v.init._type != null && v.init._type.flags.temporary) ||
                is_local_or_global_expr(v.init)
            )
            mark_non_local_let_ref_in_stack()
            return
        }
    }

    def call_func_needs_unsafe(func : Function const?; is_for_loop_src : bool) : bool {
        return (func == null
            || func.flags.unsafeOperation
            || (func.moreFlags.unsafeOutsideOfFor && !is_for_loop_src))
    }

    // Inherently-unsafe leaves — mark unconditionally (when not generated).

    def override preVisitExprDelete(expr : ExprDelete?) : void {
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprAddr(expr : ExprAddr?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprCopy(expr : ExprCopy?) : void { st029_record_func(expr.func) }
    def override preVisitExprMove(expr : ExprMove?) : void { st029_record_func(expr.func) }
    def override preVisitExprClone(expr : ExprClone?) : void { st029_record_func(expr.func) }
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (expr.variable != null) st029_record_mod(expr.variable._module)
    }
    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) : void {
        if (expr.enumType != null) st029_record_mod(expr.enumType._module)
    }
    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) : void {
        if (expr.bitfieldType != null && expr.bitfieldType.annotation != null) {
            st029_record_mod(expr.bitfieldType.annotation._module)
        }
    }
    def override preVisitExprRef2Ptr(expr : ExprRef2Ptr?) : void {
        // `addr(x)` — address-of-reference. Always unsafe.
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprAsVariant(expr : ExprAsVariant?) : void {
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprSafeAsVariant(expr : ExprSafeAsVariant?) : void {
        // `?as` on a non-pointer variant requires unsafe (unsafe_variant_safe_as); mark
        // unconditionally like `as` above — over-marking only misses a redundant-unsafe note.
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }

    // Conditional own-contribution.

    def override preVisitExprCast(expr : ExprCast?) : void {
        if (!expr.genFlags.generated
                && (expr.castFlags.upcastCast || expr.castFlags.reinterpretCast)) {
            mark_unsafe_in_stack()
        }
    }

    def override preVisitExprAt(expr : ExprAt?) : void {
        // Table / raw-pointer index needs unsafe; array-index is safe. Lint
        // pass sometimes leaves `_type` unset on the subexpr — bias to
        // "needs unsafe" when unknown so the wrap stays.
        if (expr.genFlags.generated) return
        let unknown_subexpr_type = (expr.subexpr == null || expr.subexpr._type == null)
        let is_unsafe_index = (!unknown_subexpr_type
            && (expr.subexpr._type.baseType == Type.tTable
                || expr.subexpr._type.baseType == Type.tPointer))
        if (unknown_subexpr_type || is_unsafe_index) {
            mark_unsafe_in_stack()
        }
    }

    def override preVisitExprSafeAt(expr : ExprSafeAt?) : void {
        // `?[]` on table<> / array<> / pointer-to-(table|array|pointer)
        // requires unsafe — see ast_infer_type.cpp errors
        // unsafe_table_safe_index / unsafe_array_safe_index /
        // unsafe_pointer_safe_index. Safe-at on vector / fixed_array is OK.
        if (expr.genFlags.generated) return
        if (expr.subexpr == null || expr.subexpr._type == null) {
            mark_unsafe_in_stack()
            return
        }
        let bt = expr.subexpr._type.baseType
        if (bt == Type.tTable || bt == Type.tArray) {
            mark_unsafe_in_stack()
            return
        }
        if (bt == Type.tPointer && expr.subexpr._type.firstType != null) {
            let inner = expr.subexpr._type.firstType.baseType
            if (inner == Type.tTable || inner == Type.tArray || inner == Type.tPointer) {
                mark_unsafe_in_stack()
            }
        }
    }

    def override preVisitExprField(expr : ExprField?) : void {
        // variant.field requires unsafe (write context; over-mark on read
        // is safer than under-mark — biases toward keeping wraps).
        if (!expr.genFlags.generated && expr.value != null
                && expr.value._type != null
                && expr.value._type.baseType == Type.tVariant) {
            mark_unsafe_in_stack()
        }
    }

    def override preVisitExprReturn(expr : ExprReturn?) : void {
        // Returning a reference or a temporary requires unsafe
        if (expr.genFlags.generated) return
        var has = expr.returnFlags.returnReference
        if (!has && expr.subexpr != null && expr.subexpr._type != null) {
            let t = expr.subexpr._type
            has = t.flags.ref || t.flags.temporary
        }
        if (!has && current_function != null && current_function.result != null) {
            let rt = current_function.result
            has = rt.flags.ref || rt.flags.temporary
        }
        if (!has && expr.returnFlags.moveSemantics && expr.subexpr != null) {
            has = (expr.subexpr._type == null
                || expr.subexpr._type.baseType == Type.autoinfer
                || expr.subexpr._type.baseType == Type.alias
                || expr.subexpr._type.flags.ref || expr.subexpr._type.flags.temporary)
        }
        if (has) mark_unsafe_in_stack()
    }

    def override preVisitExprOp1(expr : ExprOp1?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated
                && call_func_needs_unsafe(expr.func, expr.flags.isForLoopSource)) {
            mark_unsafe_in_stack()
        }
    }
    def override preVisitExprOp3(expr : ExprOp3?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated
                && call_func_needs_unsafe(expr.func, expr.flags.isForLoopSource)) {
            mark_unsafe_in_stack()
        }
    }
    def override preVisitExprNew(expr : ExprNew?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated
                && (expr.func == null || expr.func.flags.unsafeOperation)) {
            mark_unsafe_in_stack()
        }
    }

    def override preVisitExprUnsafe(expr : ExprUnsafe?) : void {
        let n = length(unsafe_block_stack)
        if (expr != null && !expr.genFlags.generated
            && (current_function == null || current_function.fromGeneric == null)
            && n > 0 && unsafe_block_stack[n - 1] > 0) {
            style_warning("STYLE026: nested 'unsafe \{ ... }' block; the outer 'unsafe' already covers this scope — drop the inner wrap", expr.at)
        }
        if (n > 0) {
            unsafe_block_stack[n - 1]++
        }
    }

    def override visitExprUnsafe(var expr : ExprUnsafe?) : ExpressionPtr {
        let n = length(unsafe_block_stack)
        if (n > 0) {
            unsafe_block_stack[n - 1]--
        }
        // Post-order: `unsafe { ... }` block form. The body's own
        // `visitExpression` already aggregated its statements' counts into
        // `unsafeExprs[body]` — just look it up.
        //   0 unsafe leaves → wrap redundant (STYLE024)
        //   1 unsafe leaf   → narrow to `unsafe(<that-leaf>)` (STYLE025)
        //   ≥ 2 leaves      → block is justified, silent
        if (expr == null || expr.body == null || expr.genFlags.generated
            || (current_function != null && current_function.fromGeneric != null)
            || !(expr.body is ExprBlock)) return expr
        let blk = expr.body as ExprBlock
        let frame = unsafe_frame_for(blk)
        if (frame.count == 0) {
            style_warning("STYLE024: redundant 'unsafe \{ ... }' block; no statement requires unsafe — drop the wrap", expr.at)
        } elif (frame.count == 1 &&
                !(blk.list[0] is ExprYield ||
                  blk.list[0] is ExprDelete ||
                  blk.list[0] is ExprReturn ||
                  blk.list[0] is ExprNew)
                && !frame.has_non_local_let_ref) {
            style_warning("STYLE025: 'unsafe \{ ... }' block scope is too broad; only one operation requires unsafe — narrow to 'unsafe(<sub-expr>)' wrapping that operation", expr.at)
        }
        return expr
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

def private require_analysis_enabled(disabled, enabled : table<string>) : bool {
    // Require analysis (STYLE029/STYLE030) runs only if at least one of those
    // codes survives the disabled denylist and enabled whitelist.
    var any = false
    for (code in ["STYLE029", "STYLE030"]) {
        if (key_exists(disabled, code) || (!empty(enabled) && !key_exists(enabled, code))) continue
        any = true
    }
    return any
}

def public style_lint(prog : ProgramPtr; compile_time_errors : bool; comment_hygiene : bool = false) : int {
    //! Runs the style lint visitor on the compiled program.
    //! Returns the number of warnings found.
    //! Pass ``comment_hygiene = true`` to enable STYLE014/STYLE015 checks.
    let empty_set : table<string>
    return style_lint(prog, compile_time_errors, empty_set, [comment_hygiene = comment_hygiene])
}

def public style_lint(prog : ProgramPtr; compile_time_errors : bool; disabled_codes : table<string>; comment_hygiene : bool = false) : int {
    //! Filter-aware compile-time variant. ``disabled_codes`` suppresses
    //! the listed rule IDs (e.g. "STYLE005") alongside the usual
    //! ``// nolint:CODE`` directives.
    var astVisitor = new StyleLintVisitor(compile_time_errors = compile_time_errors, comment_hygiene = comment_hygiene)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.this_module = prog.getThisModule
    let empty_enabled : table<string>
    astVisitor.analyze_requires = astVisitor.this_module != null && require_analysis_enabled(disabled_codes, empty_enabled)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (comment_hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    astVisitor->st029_check_requires(prog)
    // STYLE024/025 stack-balance sanity check.
    if (!empty(astVisitor.unsafe_stack)) {
        panic("style_lint internal: unsafe_stack not balanced after walk (size={length(astVisitor.unsafe_stack)})")
    }
    let count = astVisitor.warning_count
    unsafe { delete astVisitor; }
    return count
}

def public style_lint_collect(prog : ProgramPtr; var warnings : array<string>; comment_hygiene : bool = false) : int {
    //! Runs the style lint visitor and collects warnings as strings.
    //! Returns the number of warnings found.
    //! Pass ``comment_hygiene = true`` to enable STYLE014/STYLE015 checks.
    let empty_set : table<string>
    return style_lint_collect(prog, warnings, empty_set, empty_set, comment_hygiene)
}

def public style_lint_collect(prog : ProgramPtr; var warnings : array<string>;
                              disabled_codes, enabled_codes : table<string>;
                              comment_hygiene : bool = false) : int {
    //! Filter-aware overload. `disabled_codes` is a denylist; `enabled_codes`
    //! is a whitelist (empty == all). Codes use the bare form (e.g. "STYLE024").
    var astVisitor = new StyleLintVisitor(compile_time_errors = false, collect_warnings = true, comment_hygiene = comment_hygiene)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    astVisitor.this_module = prog.getThisModule
    astVisitor.analyze_requires = astVisitor.this_module != null && require_analysis_enabled(disabled_codes, enabled_codes)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (comment_hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    astVisitor->st029_check_requires(prog)
    // STYLE024/025 stack-balance sanity check.
    if (!empty(astVisitor.unsafe_stack)) {
        panic("style_lint internal: unsafe_stack not balanced after walk (size={length(astVisitor.unsafe_stack)})")
    }
    let count = astVisitor.warning_count
    warnings |> push_from(astVisitor.warnings)
    unsafe { delete astVisitor; }
    return count
}

// ---------------------------------------------------------------------------
// Lint macro (auto-runs when module is required)
// ---------------------------------------------------------------------------

[lint_macro]
class StyleLintMacro : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        let hygiene = prog._options |> find_arg("_comment_hygiene") ?as tBool ?? false
        let disabled <- build_lint_macro_disabled(prog)
        style_lint(prog, true, disabled, [comment_hygiene = hygiene])
        return true
    }
}
