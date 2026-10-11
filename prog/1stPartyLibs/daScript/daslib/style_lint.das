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
//!   STYLE014 — comment block exceeds 3 lines at module/public scope (on by default under daslib/, opt-in elsewhere via ``options _comment_hygiene = true``; suppress with '//!@nolint' or '// nolint:STYLE014')
//!   STYLE015 — comment block inside a 'def private' over its cap: 3 lines for a '//!' contract doc, 1 line for a plain '//' block (on by default under daslib/, opt-in elsewhere via ``options _comment_hygiene = true``; suppress with '// nolint:STYLE015')
//!   STYLE016 — adjacent guards leading to identical early-exit can be combined with '||'
//!   STYLE017 — 'if (cond) return true; else return false' — use 'return cond' (or 'return !cond')
//!   STYLE018 — redundant boolean comparison ('b == true' / 'b != false') — use 'b' / '!b'
//!   STYLE019 — nested min/max — use 'clamp(x, lo, hi)' from math module
//!   STYLE020 — from_JV(v, type<T>, defV) on a scalar — use 'v ?? defV' (operator ?? from json_boost)
//!   STYLE021 — table<string; JsonValue?> built by repeated insert — use 'var v = JV((k1=..., k2=...))'
//!   STYLE022 — bitfield compound-bit-set/clear ('foo |= BfT.m' / 'foo &= ~BfT.m') — use field-assign 'foo.m = true' / 'foo.m = false'
//!   STYLE023 — bitfield test via int-cast ('int(bf & BfT.m) != 0' / '== 0') — use 'bf.m' / '!bf.m' directly
//!   STYLE024 — redundant 'unsafe' wrap; inner expression / block body has no operation requiring unsafe — drop the wrap
//!   STYLE025 — 'unsafe { ... }' block where only one statement needs unsafe — narrow to 'unsafe(<sub-expr>)'
//!   STYLE026 — nested 'unsafe { ... }' block; outer wrap already covers the scope — drop the inner
//!   STYLE027 — var array<T> / table<K;V> with empty default-init followed by a for-loop that only push/insert into it; use a comprehension
//!   STYLE028 — 'self->method(...)' inside a class method is optional — drop 'self->' and call 'method(...)' directly (compiler auto-promotes to the same invoke)
//!   STYLE029 — non-public 'require X' used only for ONE module X re-exports — require it directly and drop X (skipped when ≥2 re-exports are used: aggregation facade; when X provides macros or [init]; or when X is a member of a 'require [group]', whose members are one unit)
//!   STYLE030 — non-public 'require X' that is entirely unused — no symbol from X (or anything it re-exports) is referenced; drop it (skipped when X provides any macro, or an [init]/[finalize]/[before_reload]/[after_reload]/[live_command] function, or only re-exports builtins used through it, or X or its public re-export closure exports a function matching an unresolved call inside an uninstanced generic body). A 'require [group]' is one unit: reported only when no member is used. Suppress a deliberate keep with '// nolint:STYLE030'
//!   STYLE031 — table<K;V> (or table<K> set) var initialized via a run of insert / '[]=' statements; use a table (set) literal move-assign
//!   STYLE032 — array<T> with empty default-init then a single 'push_from' / 'push_clone_from' from another array<T>; that is a clone — use 'var w := SRC' (clone-assign). Cloneable elements only (a copyable-but-not-cloneable element such as a lambda stays silent — ':=' would not compile)
//!   STYLE033 — run of >=2 'push_from' / 'push_clone_from' from arrays; into a fresh-empty array that is a concatenation (use 'var c <- concat(s1, s2, ...)', needs 'require daslib/linq'; cloneable elements only, as STYLE032); into an existing array — plain var or a pure chain like 'obj.arr' / 'bags[0].nums' — collapse to one variadic call of the same name ('dst |> push_from(...)' or 'dst |> push_clone_from(...)')
//!   STYLE034 — reinterpret<T?>(addr(x)) — use addr<T?>(x); one unsafe() covers both halves (pointer targets only; the sugar's own desugared output is exempt via fromAddrSugar)
//!   STYLE035 — non-int numeric variable compared with a built-in numeric cast of '<char>' — make the variable int and compare directly with the character literal
//!   STYLE036 — type contract ('-const', '-&', '-[]', '-#', '==const', '==&') on an already-resolved cast/reinterpret/upcast/addr target does nothing — drop it
//!   STYLE037 — function or closure cyclomatic complexity exceeds the limit (override per module with 'options _cyclomatic_complexity = N'; 0 disables) — split it into smaller functions
//!   STYLE038 — function exceeds the line-count limit (physical lines; override per module with 'options _function_length = N'; 0 disables) — split it into smaller functions
//!   STYLE040 — a run of statements duplicated elsewhere in the module that a helper function could absorb (on by default under daslib/ and utils/, override per module with 'options _duplicate_regions'; thresholds via 'options _dupe_min_nodes' / '_dupe_min_statements'; suppress with '// nolint:STYLE040' on the reported line)
//!   STYLE039 — non-ASCII byte in a string literal (on by default under daslib/ and modules/, override per module with 'options _ascii_strings'; suppress with '// nolint:STYLE039') — emitted strings and error messages must be ASCII unless intended
//!   STYLE041 — bool flag initialized false, set true on terminal paths, then consumed by a single 'if (flag) return ...' — a return value in disguise; return directly at each set site (walk-abort callbacks, negative polarity, non-terminal sets, and init-true flags are exempt)

require daslib/ast_boost
require daslib/dupe_detect
require daslib/is_local
require daslib/lint_config
require math
require strings

// STYLE033 arity caps — never suggest a call shape with no matching overload.
let MAX_CONCAT_ARITY = 8
let MAX_VARIADIC_PUSH_ARITY = 4

let STYLE037_DEFAULT_MAX = 20

let ST041_MAX_ARG_DEPTH = 8

let STYLE038_DEFAULT_MAX_LINES = 80

struct UnsafeFrame {
    //! Per-expression subtree summary tracked on `StyleLintVisitor.unsafe_stack`.
    count : int
    has_non_local_let_ref : bool
}

struct St041Scan {
    //! Transient per-candidate classification state for check_style041_flag_return.
    rejected : bool
    write_count : int
    write_top : int = -1
    writes_terminal : bool = true
    guard_stack : array<bool>
    plumb_count : int
    plumb_tops : array<int>
    consume_count : int
    consume_top : int = -1
    consume_top_level : bool
    consume_ok : bool
    consume_moved : bool
}

enum private St041Terminal {
    not_terminal
    then_break
    then_continue
    arm_end
}

enum private CtorFieldState {
    unknown
    default_empty
    initialized
}

struct St041Candidate {
    //! Armed candidate awaiting the visitFunction reference-count cross-check.
    classified_refs : int
    observed_refs : int
    poisoned : bool
    msg : string
}

class StyleLintVisitor : AstVisitor {
    compile_time_errors : bool
    comment_hygiene : bool = false
    ascii_strings : bool = false
    strict_smart_pointers : bool = true
    with_module_is_unsafe : bool = false
    warning_count : int = 0
    collect_warnings : bool = false
    warnings : array<string>
    issues : array<LintIssue>
    reported_locations : table<uint64>
    @do_not_delete current_function : Function?
    @do_not_delete pending_uninit_vars : array<Variable?>
    @do_not_delete skip_userSaidItsSafe : array<Expression?>
    @do_not_delete unsafeExprs : table<Expression?; UnsafeFrame>
    unsafe_stack : array<UnsafeFrame>
    unsafe_block_stack : array<int>
    complexity_stack : array<int>
    max_complexity : int = STYLE037_DEFAULT_MAX
    max_function_length : int = STYLE038_DEFAULT_MAX_LINES
    analyze_requires : bool = false
    @do_not_delete this_module : Module?
    used_modules : table<string>
    unresolved_call_names : table<string>
    @do_not_delete st041_candidates : table<Variable?; St041Candidate>

    def report_key(at : LineInfo; code : string) : uint64 {
        //! fileInfo is part of the key, where line|column alone collides across files.
        let pos = uint64(at.line) | (uint64(at.column) << uint64(20))
        return (pos ^ (intptr(at.fileInfo) * 1099511628211ul)) ^ hash(code)
    }

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
        if (!is_user_authored_body(current_function) || is_suppressed(text, at)) return
        let key = report_key(at, extract_lint_code(text))
        if (key_exists(reported_locations, key)) return
        reported_locations |> insert(key)
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
            issues |> push(make_lint_issue(msg, at))
        } else {
            to_log(LOG_WARNING, "style warning: {msg} at {describe(at)}\n")
        }
    }

    def source_line_between(at_start : LineInfo; at_end : LineInfo) : string {
        //! Reads the source text between two line infos (potentially multi-line).
        var result = ""
        if (at_start.fileInfo == null) return result
        for (ln in range(int(at_start.line), int(at_end.line) + 1)) {
            get_file_source_line(at_start.fileInfo, uint(ln)) $(line) {
                result = "{result}{line}\n"
            }
        }
        return result
    }

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = fn
        unsafe_block_stack |> push(0)
        complexity_stack |> push(1)
        st041_candidates |> clear()
        check_defer_pipe(fn)
        if (analyze_requires && fn._module == this_module) {
            for (a in fn.annotations) {
                if (a != null && a.annotation != null) st029_note_module(a.annotation._module)
            }
        }
    }

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
            var trailing_ok = true
            peek_data(src) $(data) {
                if (defer_pos + 5 < length(data)) {
                    let ch = int(data[defer_pos + 5])
                    trailing_ok = !(is_alpha(ch) || is_number(ch) || ch == '_')
                }
            }
            if (!trailing_ok) continue
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
        st037_pop_and_check("function '{fn.name}'", "split it into smaller functions", fn.at)
        if (fn.body != null) {
            st038_check_length("function '{fn.name}'", "split it into smaller functions", fn.at, fn.body.at)
        }
        st041_flush()
        current_function = null
        unsafe_block_stack |> pop
        return <- fn
    }

    def st038_check_length(what : string; fix : string; at : LineInfo; body_at : LineInfo) : void {
        if (max_function_length <= 0 || at.line == 0u || foreign_module_code()
                || (current_function != null && current_function.flags.generated)) return
        let nlines = int(body_at.last_line) - int(at.line) + 1
        if (nlines > max_function_length) {
            style_warning("STYLE038: {what} is {nlines} lines long (max {max_function_length}); {fix}", at)
        }
    }

    def st037_bump() : void {
        let n = length(complexity_stack)
        if (n > 0) complexity_stack[n - 1]++
    }

    def foreign_module_code() : bool {
        if (current_function == null || this_module == null) return false
        if (current_function._module != this_module) return true
        var root = current_function.fromGeneric
        while (root != null && root.fromGeneric != null) {
            root = root.fromGeneric
        }
        return root != null && root._module != this_module
    }

    def st037_pop_and_check(what : string; fix : string; at : LineInfo) : void {
        let n = length(complexity_stack)
        if (n == 0) return
        let cc = complexity_stack[n - 1]
        complexity_stack |> pop
        if (max_complexity > 0 && cc > max_complexity && !foreign_module_code()
                && !(current_function != null && current_function.flags.generated)) {
            style_warning("STYLE037: {what} has cyclomatic complexity {cc} (max {max_complexity}); {fix}", at)
        }
    }

    def override preVisitExprWhile(expr : ExprWhile?) : void {
        if (!expr.genFlags.generated) st037_bump()
    }

    def override preVisitExprFor(expr : ExprFor?) : void {
        if (!expr.genFlags.generated) st037_bump()
    }

    def override preVisitExprTryCatch(expr : ExprTryCatch?) : void {
        if (!expr.genFlags.generated) st037_bump()
    }

    def check_block_pipe(call_at : LineInfo; var arguments : dasvector`ptr`Expression) : void {
        let nargs = length(arguments)
        if (nargs == 0) return
        for (i in range(nargs)) {
            var arg = arguments[i]
            if (arg is ExprMakeBlock) {
                var mkblk = arg as ExprMakeBlock
                var blk = mkblk._block
                if (blk is ExprBlock) {
                    var eblk = blk as ExprBlock
                    if (eblk.blockFlags.isClosure) {
                        let block_arg_count = length(eblk.arguments)
                        let src = source_line_between(call_at, eblk.at)
                        let pipe_pos = find(src, "<|")
                        let has_pipe = pipe_pos >= 0
                        if (has_pipe) {
                            let after_pipe = strip_left(slice(src, pipe_pos + 2))
                            let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
                            if (pipes_block) {
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
        if (lowered_unsafe_delete(expr)) {
            mark_unsafe_in_stack()
            return
        }
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

    def is_supported_jv_scalar(t : TypeDecl?) : bool {
        //! True for the scalar types that have a matching `operator ??` overload in daslib/json_boost — see lines 85-140 of json_boost.das. Bitfield and enum `from_JV` overloads exist but have no `??` counterpart, so they stay silent.
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
        //! Walk fromGeneric chain to its root. Template instantiations of overloaded generics produce a 2-level chain: instantiated → mangled-overload → root.
        var cur = fn
        while (cur != null && cur.fromGeneric != null) {
            cur = cur.fromGeneric
        }
        return cur
    }

    def check_style020_from_jv(expr : ExprCall?) : void {
        if (expr.func == null || length(expr.arguments) < 2) return
        let root = root_generic(expr.func)
        let is_from_JV = (root != null && root.name == "from_JV"
            && root._module != null && root._module.name == "json_boost")
        if (!is_from_JV) return
        if (current_function != null) {
            let in_cf = (current_function._module != null
                && current_function._module.name == "json_boost")
            let in_fg = (current_function.fromGeneric != null
                && current_function.fromGeneric._module != null
                && current_function.fromGeneric._module.name == "json_boost")
            return if (in_cf || in_fg)
        }
        if (!is_supported_jv_scalar(expr._type)) return
        style_warning("STYLE020: from_JV(v, type<T>, defV) on a supported scalar - use 'v ?? defV' (operator ?? from json_boost)", expr.at)
    }

    def is_math_call(expr : Expression? const; fname : string) : bool {
        //! True iff `expr` is `math::<fname>(_, _)` with exactly two arguments (peeling ExprRef2Value). Conservatively skips any non-math overload.
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
        if (is_math_call(expr.arguments[0], inner_name)) {
            style_warning("STYLE019: nested {outer_name}({inner_name}(...)) - use 'clamp(x, lo, hi)' from math module", expr.at)
            return
        }
        if (is_math_call(expr.arguments[1], inner_name)) {
            style_warning("STYLE019: nested {outer_name}({inner_name}(...)) - use 'clamp(x, lo, hi)' from math module", expr.at)
        }
    }

    def override preVisitExprInvoke(var expr : ExprInvoke?) : void {
        check_block_pipe(expr.at, expr.arguments)
        check_style028_self_arrow(expr)
    }

    def check_style028_self_arrow(expr : ExprInvoke?) : void {
        //! `self->method(args)` inside a class method body lowers to ExprInvoke (isInvokeMethod=true) with arguments[0]=ExprField(value=ExprTypeDecl, name=method) — after type inference the same shape is produced by `self.method(args)` AND by the compiler's auto-promotion of bare `method(args)` to a method invoke. So the AST alone cannot distinguish the three forms — source inspection is required.
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

    def override preVisitExprMakeStruct(var expr : ExprMakeStruct?) : void {
        st029_record_func(expr.constructor)
        if (!expr.genFlags.generated && !expr.makeStructFlags.isNewClass
                && expr.makeType != null && expr.makeType.isClass) {
            mark_unsafe_in_stack()
        }
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

    def override preVisitExprMakeBlock(var expr : ExprMakeBlock?) : void {
        if (!empty(st041_candidates)) {
            for (cap in expr._capture) {
                for (vk, cv in keys(st041_candidates), values(st041_candidates)) {
                    if (vk != null && vk.name == cap.name) {
                        cv.poisoned = true
                    }
                }
            }
        }
        var blk = expr._block
        if (!(blk is ExprBlock)) return
        var eblk = blk as ExprBlock
        if (!eblk.blockFlags.isClosure || eblk.blockFlags.isLambdaBlock || !empty(eblk.arguments)) return
        let src = source_line_between(expr.at, eblk.at)
        let dollar_pos = find(src, "$()")
        if (dollar_pos < 0 || find(src, "<|") >= 0) return
        let after_dollar = strip_left(slice(src, dollar_pos + 3))
        if (starts_with(after_dollar, ":")) return
        style_warning("STYLE003: remove redundant $() for parameterless block; use $ \{", expr.at)
    }

    def peel_ref2value_const(e : Expression? const) : Expression? const {
        if (e != null && e is ExprRef2Value) return (e as ExprRef2Value.subexpr)
        return e
    }

    def expr_equal_struct(a : Expression? const; b : Expression? const) : bool {
        //! Structural equality via describe(). No purity filter.
        if (a == null || b == null) return false
        return describe(a) == describe(b)
    }

    def style035_numeric_cast_target(name : string) : Type {
        //! Built-in scalar numeric casts for which a character-code variable should instead use the language's native `int` character type.
        if (name == "int") return Type.tInt
        if (name == "int8") return Type.tInt8
        if (name == "int16") return Type.tInt16
        if (name == "int64") return Type.tInt64
        if (name == "uint") return Type.tUInt
        if (name == "uint8") return Type.tUInt8
        if (name == "uint16") return Type.tUInt16
        if (name == "uint64") return Type.tUInt64
        if (name == "float") return Type.tFloat
        if (name == "double") return Type.tDouble
        return Type.none
    }

    def plain_style035_variable(expr : Expression?; target : Type) : Variable const? {
        //! Return the variable behind a plain ExprVar read of the cast's numeric type. Fields, indexes, calls, and other expressions are deliberately excluded: STYLE035's fix changes the variable declaration itself.
        if (target == Type.none || target == Type.tInt || expr == null) return null
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        if (inner == null || !(inner is ExprVar)) return null
        let ev = inner as ExprVar
        let v = ev.variable
        if (v == null || v._type == null || v._type.baseType != target) return null
        return v
    }

    def numeric_char_cast_target(expr : Expression const?) : Type {
        //! T('<char>') folds to a typed ExprConst before this pass under normal compilation, but LineInfo retains the exact source span. Return T's base type, or Type.none for non-numeric/non-character casts.
        if (expr != null && expr is ExprCall) {
            let call = expr as ExprCall
            if (call.func == null || length(call.arguments) != 1
                    || call.arguments[0] == null) return Type.none
            let target = style035_numeric_cast_target(string(call.func.name))
            if (target == Type.none) return Type.none
            let arg = call.arguments[0]
            if (arg.at.fileInfo == null || arg.at.line == 0u
                    || arg.at.line != arg.at.last_line
                    || arg.at.last_column <= arg.at.column) return Type.none
            var arg_src = ""
            get_file_source_line(arg.at.fileInfo, arg.at.line) $(line) {
                let start = int(arg.at.column)
                let finish = int(arg.at.last_column)
                if (start >= 0 && finish <= length(line)) {
                    arg_src = slice(line, start, finish)
                }
            }
            let inner = strip(arg_src)
            if (length(inner) >= 3 && starts_with(inner, "'") && ends_with(inner, "'")) return target
            return Type.none
        }
        if (expr == null || expr.at.fileInfo == null
                || expr.at.line == 0u || expr.at.line != expr.at.last_line
                || expr.at.last_column <= expr.at.column) return Type.none
        var src = ""
        get_file_source_line(expr.at.fileInfo, expr.at.line) $(line) {
            let start = int(expr.at.column)
            let finish = int(expr.at.last_column)
            if (start >= 0 && finish <= length(line)) {
                src = slice(line, start, finish)
            }
        }
        let spelling = strip(src)
        let open = find(spelling, "(")
        if (open <= 0 || !ends_with(spelling, ")")) return Type.none
        let target = style035_numeric_cast_target(strip(slice(spelling, 0, open)))
        if (target == Type.none) return Type.none
        let inner = strip(slice(spelling, open + 1, length(spelling) - 1))
        if (length(inner) >= 3 && starts_with(inner, "'") && ends_with(inner, "'")) return target
        return Type.none
    }

    def check_style035_numeric_char_compare(expr : ExprOp2?) : void {
        if (expr.genFlags.generated
                || (expr.op != "==" && expr.op != "!=" && expr.op != "<"
                    && expr.op != "<=" && expr.op != ">" && expr.op != ">=")) return
        var target = numeric_char_cast_target(expr.right)
        var v = plain_style035_variable(expr.left, target)
        if (v == null) {
            target = numeric_char_cast_target(expr.left)
            v = plain_style035_variable(expr.right, target)
        }
        if (v == null) return
        style_warning("STYLE035: non-int numeric variable '{v.name}' is compared with a cast character literal; make '{v.name}' int and compare directly with the character literal", v.at)
    }

    def string_has_non_ascii(s : string const implicit; var badOffset : int&; var badByte : int&) : bool {
        var hit = false
        peek_data(s) $(arr) {
            for (i in range(length(arr))) {
                let b = int(arr[i])
                if (b >= 128) {
                    badOffset = i
                    badByte = b
                    hit = true
                    break
                }
            }
        }
        return hit
    }

    def source_span_has_non_ascii(at : LineInfo) : bool {
        if (at.fileInfo == null) return true
        var hit = false
        let l0 = int(at.line)
        let l1 = max(int(at.last_line), l0)
        for (ln in range(l0, l1 + 1)) {
            get_file_source_line(at.fileInfo, uint(ln)) $(line) {
                let len = length(line)
                let from = ln == l0 ? min(int(at.column), len) : 0
                let to = ln == l1 ? min(int(at.last_column), len) : len
                var o = 0
                var b = 0
                if (to > from && string_has_non_ascii(slice(line, from, to), o, b)) {
                    hit = true
                }
            }
            if (hit) break
        }
        return hit
    }

    def override preVisitExprConstString(expr : ExprConstString?) : void {
        var badOffset = 0
        var badByte = 0
        if (!ascii_strings || expr.genFlags.generated
                || !string_has_non_ascii(string(expr.value), badOffset, badByte)
                || !source_span_has_non_ascii(expr.at)) return
        style_warning("STYLE039: non-ASCII byte {badByte} at offset {badOffset} in string literal; emitted strings must be ASCII - spell it out (e.g. '-' for an em-dash), or '// nolint:STYLE039' when the character is intended", expr.at)
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
        if (expr.op == "==" || expr.op == "!=") {
            let lhs = peel_ref2value_const(expr.left)
            let rhs = peel_ref2value_const(expr.right)
            let lbool = lhs != null && lhs is ExprConstBool
            let rbool = rhs != null && rhs is ExprConstBool
            if (lbool != rbool) {
                let const_side = lbool ? lhs : rhs
                let cval = (const_side as ExprConstBool).value
                let positive = (expr.op == "==" && cval) || (expr.op == "!=" && !cval)
                let suggestion = positive ? "b" : "!b"
                style_warning("STYLE018: redundant boolean comparison - use '{suggestion}' directly (drop the '{expr.op} {cval}')", expr.at)
            }
        }
        if (expr.op == "|=" || expr.op == "&=") {
            check_style022_bitfield_set_clear(expr)
        }
        if (expr.op == "==" || expr.op == "!=") {
            check_style023_bitfield_test(expr)
        }
        check_style035_numeric_char_compare(expr)
    }

    def is_bitfield_value_type(t : TypeDecl?) : bool {
        //! True for plain bitfield-typed expressions (not arrays of bitfield).
        return t != null && t.baseType == Type.tBitfield
    }

    def extract_single_bit_name(expr : Expression? const; bf_type : TypeDecl?) : string {
        //! If `expr` resolves to a single named bit of `bf_type`, return its name; else "".
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
            if (v == 0ul || (v & (v - 1ul)) != 0ul) return ""
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
        //! True for ExprConstInt/UInt/Int64/UInt64 whose value is zero. The lint runner disables folding, so `0` / `0u` / `0l` / `0ul` literals stay as distinct typed const nodes — we accept all four.
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt.value) == 0
        if (expr is ExprConstUInt) return (expr as ExprConstUInt.value) == 0u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64.value) == 0l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64.value) == 0ul
        return false
    }

    def unwrap_int_like_cast(expr : Expression? const) : Expression? const {
        //! Peel one int/uint/int64/uint64 single-arg call. Returns the inner expression, or null if not such a cast.
        if (expr == null || !(expr is ExprCall)) return null
        let call = expr as ExprCall
        if (call.func == null) return null
        let name = string(call.func.name)
        if ((name != "int" && name != "uint" && name != "int64" && name != "uint64") || length(call.arguments) != 1) return null
        return call.arguments[0]
    }

    def check_style023_bitfield_test(expr : ExprOp2?) : void {
        //! STYLE023: `int_cast(bf & BfT.bit) !=/== 0` collapses to `bf.bit` / `!bf.bit`.
        let lhs = peel_ref2value_const(expr.left)
        let rhs = peel_ref2value_const(expr.right)
        let lzero = is_zero_int_const(lhs)
        let rzero = is_zero_int_const(rhs)
        if (lzero == rzero) return
        let cast_side = lzero ? rhs : lhs
        let inner = unwrap_int_like_cast(cast_side)
        if (inner == null) return
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
        let positive = expr.op == "!="
        let recv = describe(op2.left)
        let suggestion = positive ? "{recv}.{bit_name}" : "!{recv}.{bit_name}"
        style_warning("STYLE023: bitfield test via int-cast can be written directly - use '{suggestion}' instead (drop the cast and the '{expr.op} 0' compare)", expr.at)
    }

    def check_style022_bitfield_set_clear(expr : ExprOp2?) : void {
        if (expr.left == null || !is_bitfield_value_type(expr.left._type)) return
        let bf_type = expr.left._type
        if (expr.op == "|=") {
            let name = extract_single_bit_name(expr.right, bf_type)
            if (empty(name)) return
            style_warning("STYLE022: 'foo |= BfT.{name}' on a bitfield is the bit-set idiom; write 'foo.{name} = true' instead (bitfield-as-field assignment)", expr.at)
        } elif (expr.op == "&=") {
            var inner = expr.right
            if (inner != null && inner is ExprRef2Value) {
                inner = (inner as ExprRef2Value.subexpr)
            }
            if (inner != null && inner is ExprOp1) {
                let op1 = inner as ExprOp1
                if (op1.op != "~") return
                let name = extract_single_bit_name(op1.subexpr, bf_type)
                if (empty(name)) return
                style_warning("STYLE022: 'foo &= ~BfT.{name}' on a bitfield is the bit-clear idiom; write 'foo.{name} = false' instead (bitfield-as-field assignment)", expr.at)
                return
            }
            if (inner != null && inner is ExprConstBitfield) {
                let cb = inner as ExprConstBitfield
                let v = uint64(cb.value)
                let n = uint64(length(bf_type.argNames))
                let valid_mask = n >= 64ul ? ~0ul : ((1ul << n) - 1ul)
                let complement = (~v) & valid_mask
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

    def single_terminator_body(body : Expression? const) : Expression? const {
        //! If `body` is a bare ExprReturn/Break/Continue, or a 1-statement ExprBlock wrapping one, return the inner terminator. Otherwise null.
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
        //! Two terminators have the same payload iff they are the same kind and their return-subexpr (if any) is structurally equal. Break==Break and Continue==Continue have no payload.
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
        if (ifte.if_flags.isStatic || ifte.genFlags.generated) return
        st037_bump()
        var cond = ifte.cond
        if (cond is ExprConstBool) {
            var cbool = cond as ExprConstBool
            if (cbool.value && ifte.if_false == null) {
                style_warning("STYLE010: if (true) is always taken; use a bare block instead", ifte.at)
            }
        }
        let outer_term = single_terminator_body(ifte.if_true)
        if (ifte.if_false == null && outer_term != null && ifte.if_true is ExprBlock) {
            let blk = ifte.if_true as ExprBlock
            if (blk.at.line != outer_term.at.line || blk.at.column != outer_term.at.column) {
                style_warning("STYLE005: braces around a single-statement early exit are noise; use 'if (cond) <terminator>' or postfix '<terminator> if (cond)'", ifte.at)
            }
        }
        if (outer_term != null && ifte.if_false != null) {
            let inner = unwrap_inner_ifte(ifte.if_false)
            if (inner != null && inner.if_false == null) {
                let inner_term = single_terminator_body(inner.if_true)
                if (inner_term != null && terminators_payload_equal(outer_term, inner_term)) {
                    style_warning("STYLE016: adjacent guards leading to identical early-exit can be combined with '||'", ifte.at)
                }
            }
        }
        if (outer_term != null && ifte.if_false != null) {
            let else_term = single_terminator_body(ifte.if_false)
            if (else_term != null) {
                var b1 = false
                var b2 = false
                if (is_const_bool_return(outer_term, b1) && is_const_bool_return(else_term, b2) && b1 != b2) {
                    let suggestion = b1 ? "return cond" : "return !cond"
                    style_warning("STYLE017: 'if (cond) return {b1}; else return {b2}' - use '{suggestion}' directly", ifte.at)
                }
            }
        }
    }

    def override preVisitExprLetVariable(expr : ExprLet?; var v : VariablePtr; last : bool) : void {
        if (v.init == null && !v.flags.generated && !v.flags.inScope && (current_function == null || current_function.fromGeneric == null)) {
            pending_uninit_vars |> push(v)
        }
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

    def override preVisitExprBlock(blk : ExprBlock?) : void {
        if (blk.blockFlags.isClosure) {
            unsafe_block_stack |> push(0)
            if (!blk.genFlags.generated) complexity_stack |> push(1)
        }
        check_style012_chain_runs(blk)
        check_style033_existing_runs(blk)
        check_style033_existing_chain_runs(blk)
        let n = length(blk.list)
        for (i in range(n - 1)) {
            let cur = blk.list[i]
            let nxt = blk.list[i + 1]
            if (!(cur is ExprIfThenElse) || cur.genFlags.generated || nxt.genFlags.generated) continue
            let outer_if = cur as ExprIfThenElse
            if (outer_if.if_false != null) continue
            let outer_term = single_terminator_body(outer_if.if_true)
            if (outer_term == null) continue
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
            var b1 = false
            var b2 = false
            if (is_const_bool_return(outer_term, b1) && is_const_bool_return(nxt, b2) && b1 != b2) {
                let suggestion = b1 ? "return cond" : "return !cond"
                style_warning("STYLE017: 'if (cond) return {b1}; return {b2}' - use '{suggestion}' directly", outer_if.at)
            }
        }
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            unsafe_block_stack |> pop
            if (!blk.genFlags.generated) {
                st037_pop_and_check("closure", "extract it into a named function", blk.at)
            }
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
            check_style031_table_init(blk, expr as ExprLet)
            check_style032_clone_init(blk, expr as ExprLet)
            check_style041_flag_return(blk, expr as ExprLet)
        }
    }

    def check_style041_flag_return(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function == null || current_function.fromGeneric != null) return
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 2 >= n) return
        for (v in elet.variables) {
            if (v.init == null || v.flags.generated || v.flags.inScope
                    || v._type == null || v._type.baseType != Type.tBool) continue
            let ini = peel_ref2value_const(v.init)
            if (ini == null || !(ini is ExprConstBool) || (ini as ExprConstBool).value) continue
            st041_analyze_candidate(blk, idx, n, v)
        }
    }

    def st041_analyze_candidate(blk : ExprBlock?; idx, n : int; v : VariablePtr) : void {
        var inscope st = St041Scan()
        for (t in range(idx + 1, n)) {
            st041_scan_stmt(blk.list[t], blk, t, t, true, v, st)
            return if (st.rejected)
        }
        return if (st.write_count == 0 || !st.writes_terminal || st.write_top < 0
            || st.consume_count != 1 || !st.consume_top_level || st.consume_top != st.write_top + 1)
        for (pt in st.plumb_tops) {
            return if (pt != st.write_top)
        }
        return if (!st.consume_ok)
        let msg = (st.consume_moved
            ? "STYLE041: bool flag '{v.name}' is a return value in disguise; move the 'if ({v.name})' body to each '{v.name} = true' site and drop the flag"
            : "STYLE041: bool flag '{v.name}' is a return value in disguise; return directly at each '{v.name} = true' site and drop the flag")
        st041_candidates[v] = St041Candidate(
            classified_refs = st.write_count + st.plumb_count + 1,
            observed_refs = 0,
            msg = msg)
    }

    def st041_classify_write(cp : ExprCopy?; arm : ExprBlock?; pos, top : int;
                             v : VariablePtr; var st : St041Scan) : void {
        return if (find_assign_var(cp.left) != v)
        let rhs = peel_ref2value_const(cp.right)
        if (rhs == null || !(rhs is ExprConstBool) || !(rhs as ExprConstBool).value || arm == null) {
            st.rejected = true
            return
        }
        st.write_count++
        if (st.write_top == -1) {
            st.write_top = top
        } elif (st.write_top != top) {
            st.rejected = true
            return
        }
        let kind = st041_terminal_kind(arm, pos)
        var guarded = kind != St041Terminal.not_terminal
        for (i in range(length(st.guard_stack) - (kind == St041Terminal.then_break ? 1 : 0))) {
            if (!st.guard_stack[i]) {
                guarded = false
            }
        }
        if (!guarded) {
            st.writes_terminal = false
        }
    }

    def st041_scan_stmt(stmt : ExpressionPtr; arm : ExprBlock?; pos, top : int; is_top : bool;
                        v : VariablePtr; var st : St041Scan) : void {
        return if (stmt == null || st.rejected || stmt.genFlags.generated)
        if (stmt is ExprCopy) {
            st041_classify_write(stmt as ExprCopy, arm, pos, top, v, st)
            return
        }
        if (stmt is ExprIfThenElse) {
            let ifte = stmt as ExprIfThenElse
            return if (ifte.if_flags.isStatic)
            if (st041_is_flag_read(ifte.cond, v)) {
                let term = single_terminator_body(ifte.if_true)
                if (ifte.if_false == null && term != null && (term is ExprBreak || term is ExprContinue)) {
                    st.plumb_count++
                    st.plumb_tops |> push(top)
                } elif (st.consume_count == 0) {
                    st.consume_count = 1
                    st.consume_top = top
                    st.consume_top_level = is_top
                    st.consume_ok = st041_consume_body_ok(ifte, st.consume_moved)
                } else {
                    st.consume_count++
                }
                return
            }
            st041_scan_arm(ifte.if_true, top, v, st)
            st041_scan_arm(ifte.if_false, top, v, st)
            return
        }
        if (stmt is ExprFor) {
            st.guard_stack |> push(st041_loop_guarded((stmt as ExprFor).body, v))
            st041_scan_arm((stmt as ExprFor).body, top, v, st)
            st.guard_stack |> pop
            return
        }
        if (stmt is ExprWhile) {
            st.guard_stack |> push(st041_loop_guarded((stmt as ExprWhile).body, v))
            st041_scan_arm((stmt as ExprWhile).body, top, v, st)
            st.guard_stack |> pop
            return
        }
        if (stmt is ExprBlock) {
            st041_scan_arm(stmt, top, v, st)
        }
    }

    def st041_scan_arm(body : ExpressionPtr; top : int; v : VariablePtr; var st : St041Scan) : void {
        return if (body == null || st.rejected)
        if (body is ExprIfThenElse) {
            st041_scan_stmt(body, null, 0, top, false, v, st)
            return
        }
        return if (!(body is ExprBlock))
        let ab = body as ExprBlock
        let m = length(ab.list)
        for (p in range(m)) {
            st041_scan_stmt(ab.list[p], ab, p, top, false, v, st)
            return if (st.rejected)
        }
    }

    def st041_is_flag_read(e : Expression? const; v : VariablePtr) : bool {
        let pe = peel_ref2value_const(e)
        if (pe == null || !(pe is ExprVar)) return false
        return (pe as ExprVar).variable == v
    }

    def st041_terminal_kind(arm : ExprBlock?; pos : int) : St041Terminal {
        return St041Terminal.not_terminal if (!empty(arm.finalList))
        let m = length(arm.list)
        return St041Terminal.arm_end if (pos == m - 1)
        return St041Terminal.not_terminal if (pos != m - 2)
        return St041Terminal.then_break if (arm.list[m - 1] is ExprBreak)
        return St041Terminal.then_continue if (arm.list[m - 1] is ExprContinue)
        return St041Terminal.not_terminal
    }

    def st041_loop_guarded(body : Expression? const; v : VariablePtr) : bool {
        if (body == null || !(body is ExprBlock)) return false
        let ab = body as ExprBlock
        return false if (empty(ab.list))
        let stmt = ab.list[0]
        return false if (!(stmt is ExprIfThenElse))
        let ifte = stmt as ExprIfThenElse
        return false if (ifte.if_false != null || !st041_is_flag_read(ifte.cond, v))
        let term = single_terminator_body(ifte.if_true)
        return term != null && term is ExprBreak
    }

    def st041_consume_body_ok(ifte : ExprIfThenElse?; var moved_body : bool&) : bool {
        if (ifte == null || ifte.if_false != null || ifte.if_true == null) return false
        if (ifte.if_true is ExprReturn) {
            moved_body = false
            return st041_return_value_ok((ifte.if_true as ExprReturn).subexpr)
        }
        return false if (!(ifte.if_true is ExprBlock))
        let body = ifte.if_true as ExprBlock
        return false if (!empty(body.finalList))
        let m = length(body.list)
        return false if (m < 1 || !(body.list[m - 1] is ExprReturn))
        for (p in range(m - 1)) {
            let stmt = body.list[p]
            return false if (!(stmt is ExprCall))
            for (a in (stmt as ExprCall).arguments) {
                return false if (!st041_arg_repeatable(a, 0))
            }
        }
        moved_body = m > 1
        return st041_return_value_ok((body.list[m - 1] as ExprReturn).subexpr)
    }

    def st041_is_const_literal(e : Expression? const) : bool {
        return (e is ExprConstBool || e is ExprConstInt || e is ExprConstUInt
            || e is ExprConstInt64 || e is ExprConstUInt64 || e is ExprConstFloat
            || e is ExprConstDouble || e is ExprConstString || e is ExprConstEnumeration
            || e is ExprConstBitfield || e is ExprConstPtr)
    }

    def st041_return_value_ok(e0 : Expression? const) : bool {
        if (e0 == null) return true
        let e = peel_ref2value_const(e0)
        if (e == null) return false
        if (e is ExprVar) {
            let ev = e as ExprVar
            if (ev.variable == null || ev.variable._type == null) return false
            if (ev.varFlags.argument || ev.variable.flags.global) return ev.variable._type.flags.constant
            return false
        }
        if (e is ExprOp1) {
            let sub = peel_ref2value_const((e as ExprOp1).subexpr)
            return sub != null && st041_is_const_literal(sub)
        }
        return st041_is_const_literal(e)
    }

    def st041_arg_repeatable(e0 : Expression? const; depth : int) : bool {
        return false if (depth > ST041_MAX_ARG_DEPTH)
        let e = peel_ref2value_const(e0)
        if (e == null) return false
        return true if (e is ExprFakeContext || e is ExprFakeLineInfo)
        if (e is ExprVar) {
            let ev = e as ExprVar
            return ev.varFlags.argument || (ev.variable != null && ev.variable.flags.global)
        }
        return true if (st041_is_const_literal(e))
        if (e is ExprField) return st041_arg_repeatable((e as ExprField).value, depth + 1)
        if (e is ExprStringBuilder) {
            for (el in (e as ExprStringBuilder).elements) {
                return false if (!st041_arg_repeatable(el, depth + 1))
            }
            return true
        }
        if (e is ExprCall) {
            for (a in (e as ExprCall).arguments) {
                return false if (!st041_arg_repeatable(a, depth + 1))
            }
            return true
        }
        return false
    }

    def st041_flush() : void {
        for (vk, cv in keys(st041_candidates), values(st041_candidates)) {
            if (vk != null && !cv.poisoned && cv.observed_refs == cv.classified_refs) {
                style_warning(cv.msg, vk.at)
            }
        }
        st041_candidates |> clear()
    }

    def check_style012_array_init(blk : ExprBlock?; elet : ExprLet?) : void {
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
        //! When stmt is `<pure chain> |> push/emplace(value)` on an array<T>, returns the receiver chain expression; null otherwise. Exactly 2 args — the 3-arg `push(a, v, at_index)` positional-insert overload must not collapse into a literal.
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
        //! Strips typer-synthesized value adapters (Ref2Value, Ptr2Ref) that wrap chain links on pointer auto-deref — invisible to the user, safe to peel.
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
        //! Chain grammar: ExprVar root, ExprField links, ExprAt links with a const-int/uint/string or simple-var index. Anything else (calls, ?., computed indexes) is impure — receiver identity across the run can't be guaranteed, so the run is not collapsible.
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

    def ctor_field_state(v : Variable const?; fname : das_string) : CtorFieldState {
        var st : Structure const? = null
        if (v._type != null && v._type.baseType == Type.tStructure) {
            st = v._type.structType
        }
        if (st == null) return CtorFieldState.unknown
        for (fld in st.fields) {
            if (fld.name == fname) {
                if (fld.init != null) return CtorFieldState.initialized
                break
            }
        }
        var ms : ExprMakeStruct const? = null
        if (v.init != null) {
            return CtorFieldState.unknown if (!(v.init is ExprMakeStruct))
            ms = v.init as ExprMakeStruct
            return CtorFieldState.unknown if (ms._block != null)
        }
        if (ms != null) {
            for (row in ms.structs) {
                for (mf in *row) {
                    if (mf.name == fname) return CtorFieldState.initialized
                }
            }
        }
        return CtorFieldState.default_empty
    }

    def emit_style012_chain(blk : ExprBlock?; idx : int; recv : Expression const?) : void {
        let head = blk.list[idx]
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
                        if (state == CtorFieldState.default_empty) {
                            let tname = string(v._type.structType.name)
                            style_warning("STYLE012: array field '{ef.name}' of freshly constructed '{v.name}' filled by a run of push/emplace; fold an array literal into the constructor `var {v.name} <- {tname}({ef.name} <- [x, <-y, z], ...)`", head.at)
                            return
                        }
                        if (state == CtorFieldState.initialized) return
                        break
                    }
                }
            }
        }
        let chain = chain_describe(recv)
        style_warning("STYLE012: run of push/emplace into '{chain}'; use an array literal move-assign `{chain} <- [x, <-y, z]` (suppress with nolint when appending to existing content is intended)", head.at)
    }

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
            if (v.flags.generated || v.flags.inScope
                || !is_struct_or_struct_ptr(v._type)
                || !is_default_struct_init(v)) continue
            var count = 0
            for (j in range(idx + 1, n)) {
                if (!is_field_assign_of(blk.list[j], v)) break
                count++
            }
            if (count >= 2) {
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
        //! True for `var a : Foo` (no init, struct-valued ONLY — pointer null-init is a different bug and is NOT flagged here), `var a = Foo()` (zero-arg MakeStruct), or `var a = new Foo()` / `var a = new Foo` (zero-arg ExprNew). False for any non-empty constructor, `make_foo()` call, or pipe chain — those are "modify an existing value" cases.
        if (v.init == null) return v._type != null && v._type.baseType == Type.tStructure
        if (v.init is ExprMakeStruct) {
            var ms = v.init as ExprMakeStruct
            if (ms._block != null) return false
            for (row in ms.structs) {
                for (_mf in *row) {
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
        if (src != null && src is ExprRef2Value) {
            src = (src as ExprRef2Value).subexpr
        }
        return false if (src == null || !(src is ExprVar))
        return (src as ExprVar).variable == target
    }

    def field_name_of(stmt : Expression?) : string {
        //! Return the ExprField name of stmt's LHS, or "" if stmt isn't a field assignment. Caller is expected to have already passed `is_field_assign_of`.
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

    def is_jv_pointer_type(t : TypeDecl?) : bool {
        if (t == null || t.baseType != Type.tPointer || t.firstType == null
                || t.firstType.baseType != Type.tStructure
                || t.firstType.structType == null) return false
        return (t.firstType.structType.name == "JsonValue"
            && t.firstType.structType._module != null
            && t.firstType.structType._module.name == "json")
    }

    def is_jv_table_type(t : TypeDecl?) : bool {
        //! Matches `table<string; JsonValue?>` (or any pointer-to-JsonValue value type).
        if (t == null || t.baseType != Type.tTable
                || t.firstType == null
                || t.firstType.baseType != Type.tString) return false
        return is_jv_pointer_type(t.secondType)
    }

    def is_insert_of_const_key(call : ExprCall?; target : Variable?) : bool {
        //! A computed key disqualifies the chain.
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
        //! Recursively verifies forE's body is built solely from pushes/inserts/at-assigns into `target`, optionally wrapped in if-filters and nested-for (up to `for_budget` additional nested ExprFor levels). Returns false on any side-effecting statement or unrelated shape, so the all-or-nothing structure guarantees the suggested comprehension rewrite is semantically equivalent.
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
        //! True when stmt is `target[<key>] = <expr>` — ExprCopy/Clone/Move whose LHS is ExprAt with subexpr resolving (after one ExprRef2Value peel) to ExprVar(target).
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

    def check_style032_clone_init(blk : ExprBlock?; elet : ExprLet?) : void {
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
            if (v.init != null
                || v.flags.generated || v.flags.inScope
                || v._type == null || v._type.baseType != Type.tArray
                || v._type.firstType == null || !v._type.firstType.canClone) continue
            var count = 0
            for (j in range(idx + 1, n)) {
                let stmt = blk.list[j]
                if (stmt == null || stmt.genFlags.generated || !(stmt is ExprCall)
                    || !is_array_clone_init_call(stmt as ExprCall, v)) break
                count++
            }
            if (count == 1) {
                style_warning("STYLE032: array<T> '{v.name}' default-initialized then filled by a single `push_from`/`push_clone_from`; that is a clone of the source - use `var {v.name} := SRC` (clone-assign)", v.at)
            } elif (count >= 2 && count <= MAX_CONCAT_ARITY) {
                style_warning("STYLE033: array<T> '{v.name}' default-initialized then filled by a run of {count} `push_from`/`push_clone_from` from arrays - that is a concatenation; use `var {v.name} <- concat(s1, s2, ...)` (needs `require daslib/linq`)", v.at)
            }
        }
    }

    def bulk_src_matches_elem(src : Expression? const; destElem : TypeDecl? const) : bool {
        //! A genuine bulk clone/copy: `src` is an `array<E>` whose element type E equals `destElem`. A *nested* source (`array<array<E>>` into a flat `array<E>`, or any deeper mismatch) binds the recursive-flatten push_from(Arr, src) overload (builtin.das) — that is a flatten, NOT a `:=`/concat/variadic-collapse, so it must NOT match. A C-array source (tFixedArray) is not tArray and has no clean array<T> rewrite either.
        if (src == null || src._type == null || src._type.baseType != Type.tArray
            || src._type.firstType == null || destElem == null) return false
        return describe(src._type.firstType) == describe(destElem)
    }

    def bulk_src_matches_dest(src : Expression? const; dest : Variable? const) : bool {
        if (dest == null || dest._type == null) return false
        return bulk_src_matches_elem(src, dest._type.firstType)
    }

    def is_array_clone_init_call(call : ExprCall?; target : Variable?) : bool {
        //! `target |> push_from(SRC)` / `push_clone_from(SRC)` (exactly 2 args, arg[0] == target) where SRC is an `array<E>` with E == target's element type. Into a fresh-empty target this bulk copy/clone is exactly `var target := SRC`.
        if ((!is_named_target_call(call, target, 2, "push_from")
             && !is_named_target_call(call, target, 2, "push_clone_from"))
            || length(call.arguments) != 2) return false
        return bulk_src_matches_dest(call.arguments[1], target)
    }

    def bulk_from_call_target(stmt : Expression?; var fname : string&) : Variable? {
        //! When stmt is `T |> push_from(SRC)` / `push_clone_from(SRC)` (exactly 2 args, SRC an array<E> with E == T's element type — a genuine bulk append, not a flatten), return T's variable and set fname to the call name; else null.
        if (stmt == null || stmt.genFlags.generated || !(stmt is ExprCall)) return null
        let call = stmt as ExprCall
        if (call.func == null || call.func.fromGeneric == null
            || length(call.arguments) != 2) return null
        let isFrom = call.func.fromGeneric.name == "push_from"
        let isCloneFrom = call.func.fromGeneric.name == "push_clone_from"
        if (!isFrom && !isCloneFrom) return null
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (!(arg is ExprVar)
            || !bulk_src_matches_dest(call.arguments[1], (arg as ExprVar).variable)) return null
        fname = isFrom ? "push_from" : "push_clone_from"
        return (arg as ExprVar).variable
    }

    def run_preceded_by_empty_array_decl(blk : ExprBlock?; runStart : int; v : Variable?) : bool {
        //! True when the statement just before the run is `var v : array<T>` (no init) — the fresh-empty case, owned by check_style032_clone_init (its STYLE033 concat branch).
        if (runStart <= 0) return false
        let prev = blk.list[runStart - 1]
        if (prev == null || !(prev is ExprLet)) return false
        for (lv in (prev as ExprLet).variables) {
            if (lv == v && lv.init == null
                && lv._type != null && lv._type.baseType == Type.tArray) return true
        }
        return false
    }

    def check_style033_existing_runs(blk : ExprBlock?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        let n = length(blk.list)
        var i = 0
        while (i < n) {
            var fname = ""
            let target = bulk_from_call_target(blk.list[i], fname)
            if (target == null) {
                i++
                continue
            }
            var count = 1
            while (i + count < n) {
                var fname2 = ""
                let t2 = bulk_from_call_target(blk.list[i + count], fname2)
                if (t2 != target || fname2 != fname) break
                count++
            }
            if (count >= 2 && count <= MAX_VARIADIC_PUSH_ARITY
                && target._type != null && target._type.firstType != null
                && target._type.firstType.canCloneFromConst
                && !run_preceded_by_empty_array_decl(blk, i, target)) {
                style_warning("STYLE033: run of {count} `{fname}` calls into '{target.name}' - collapse to one variadic call `{target.name} |> {fname}(s1, s2, ...)`", blk.list[i].at)
            }
            i += count
        }
    }

    def bulk_from_chain_receiver(stmt : Expression const?; var fname : string&) : Expression const? {
        //! Like bulk_from_call_target but for a pure-CHAIN destination (>=1 field/index link, e.g. `obj.arr`): when stmt is `<chain> |> push_from/push_clone_from(SRC)` (2 args, SRC an array<E> with E == chain element type — bulk append, not flatten), return the receiver chain and set fname; else null. Plain ExprVar receivers (link_count 0) are left to bulk_from_call_target's variable form, so the two never double-fire.
        if (stmt == null || stmt.genFlags.generated || !(stmt is ExprCall)) return null
        let call = stmt as ExprCall
        if (call.func == null || call.func.fromGeneric == null
            || length(call.arguments) != 2) return null
        let isFrom = call.func.fromGeneric.name == "push_from"
        let isCloneFrom = call.func.fromGeneric.name == "push_clone_from"
        if (!isFrom && !isCloneFrom) return null
        let recv = call.arguments[0]
        if (recv == null || recv._type == null || recv._type.baseType != Type.tArray
            || !is_pure_chain(recv) || chain_link_count(recv) < 1
            || !bulk_src_matches_elem(call.arguments[1], recv._type.firstType)) return null
        fname = isFrom ? "push_from" : "push_clone_from"
        return recv
    }

    def check_style033_existing_chain_runs(blk : ExprBlock?) : void {
        if (current_function != null && current_function.fromGeneric != null) return
        let n = length(blk.list)
        var i = 0
        while (i < n) {
            var fname = ""
            let recv = bulk_from_chain_receiver(blk.list[i], fname)
            if (recv == null) {
                i++
                continue
            }
            var count = 1
            while (i + count < n) {
                var fname2 = ""
                let nrecv = bulk_from_chain_receiver(blk.list[i + count], fname2)
                if (nrecv == null || fname2 != fname || !chain_equal(recv, nrecv)) break
                count++
            }
            if (count >= 2 && count <= MAX_VARIADIC_PUSH_ARITY
                    && recv._type != null && recv._type.firstType != null
                    && recv._type.firstType.canCloneFromConst) {
                let cd = chain_describe(recv)
                style_warning("STYLE033: run of {count} `{fname}` calls into '{cd}' - collapse to one variadic call `{cd} |> {fname}(s1, s2, ...)`", blk.list[i].at)
            }
            i += count
        }
    }

    def is_table_insert_of(call : ExprCall?; target : Variable?; nargs : int) : bool {
        if (call.func == null || call.func.fromGeneric == null
            || call.func.fromGeneric.name != "insert"
            || length(call.arguments) != nargs) return false
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (!(arg is ExprVar)) return false
        return (arg as ExprVar).variable == target
    }

    def at_assign_key(stmt : Expression?) : Expression? {
        var lhs : Expression? = null
        if (stmt is ExprCopy) {
            lhs = (stmt as ExprCopy).left
        } elif (stmt is ExprClone) {
            lhs = (stmt as ExprClone).left
        } elif (stmt is ExprMove) {
            lhs = (stmt as ExprMove).left
        }
        return null if (lhs == null)
        return (lhs as ExprAt).index
    }

    def const_key_repr(key0 : Expression?) : string {  // nolint:STYLE037 — `is` on handled types is exact-type, so one arm per const class is irreducible
        //! Type-tagged stringification of a constant-literal key; "" for computed keys (not comparable at lint time).
        var key = key0
        if (key != null && key is ExprRef2Value) {
            key = (key as ExprRef2Value).subexpr
        }
        if (key == null) return ""
        if (key is ExprConstString) return "s:{(key as ExprConstString).value}"
        if (key is ExprConstInt) return "i:{(key as ExprConstInt).value}"
        if (key is ExprConstUInt) return "u:{(key as ExprConstUInt).value}"
        if (key is ExprConstInt64) return "i64:{(key as ExprConstInt64).value}"
        if (key is ExprConstUInt64) return "u64:{(key as ExprConstUInt64).value}"
        if (key is ExprConstFloat) return "f:{(key as ExprConstFloat).value}"
        if (key is ExprConstDouble) return "d:{(key as ExprConstDouble).value}"
        if (key is ExprConstBool) return "b:{(key as ExprConstBool).value}"
        if (key is ExprConstEnumeration) return "e:{(key as ExprConstEnumeration).value}"
        return ""
    }

    def st031_scan_insert_run(blk : ExprBlock?; idx, n : int; v : VariablePtr; isSet : bool; var dup_const_key : bool&) : int {
        var count = 0
        var seen_keys : table<string>
        for (j in range(idx + 1, n)) {
            let stmt = blk.list[j]
            var key : Expression? = null
            if (stmt is ExprCall && is_table_insert_of(stmt as ExprCall, v, isSet ? 2 : 3)) {
                key = (stmt as ExprCall).arguments[1]
            } elif (!isSet && is_at_assign_of(stmt, v)) {
                key = at_assign_key(stmt)
            } else {
                break
            }
            let repr = const_key_repr(key)
            if (repr != "") {
                if (key_exists(seen_keys, repr)) {
                    dup_const_key = true
                }
                seen_keys |> insert(repr)
            }
            count++
        }
        delete seen_keys
        return count
    }

    def st031_jv_const_key_run(blk : ExprBlock?; idx, n : int; v : VariablePtr) : int {
        var jv = 0
        for (j in range(idx + 1, n)) {
            let stmt = blk.list[j]
            if (!(stmt is ExprCall) || !is_insert_of_const_key(stmt as ExprCall, v)) break
            jv++
        }
        return jv
    }

    def check_style031_table_init(blk : ExprBlock?; elet : ExprLet?) : void {
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
            if (v.init != null
                || v.flags.generated || v.flags.inScope
                || v._type == null || v._type.baseType != Type.tTable) continue
            let isSet = v._type.secondType == null || v._type.secondType.baseType == Type.tVoid
            var dup_const_key = false
            let count = st031_scan_insert_run(blk, idx, n, v, isSet, dup_const_key)
            if (count < 2 || dup_const_key) continue
            if (is_jv_table_type(v._type)) {
                continue if (st031_jv_const_key_run(blk, idx, n, v) >= 2)
            }
            if (isSet) {
                style_warning("STYLE031: table<K> set initialized by a run of insert calls; use a set literal move-assign `var {v.name} <- \{k1, k2, ...\}`", v.at)
            } else {
                style_warning("STYLE031: table<K;V> initialized by a run of insert/`[]=` statements; use a table literal move-assign `var {v.name} <- \{k1 => v1, k2 => v2, ...\}`", v.at)
            }
        }
    }

    def st029_expr_in_scope() : bool {
        return (analyze_requires
            && (current_function == null || current_function._module == this_module))
    }

    def st029_note_module(m : Module const?) : void {
        st029_note_module_name(m == null ? "" : "{m.name}")
    }

    def st029_note_module_name(n : string) : void {
        if (this_module == null || n == "" || n == "$" || n == "builtin" || n == "{this_module.name}") return
        if (!key_exists(used_modules, n)) used_modules |> insert(n)
    }

    def st029_record_module_getter(expr : ExprCall?) : void {
        if (expr.func == null || !st029_expr_in_scope() || empty(expr.arguments)) return
        let fn = expr.func.fromGeneric != null ? "{expr.func.fromGeneric.name}" : "{expr.func.name}"
        if (fn != "get_module" && fn != "find_module" && fn != "has_module" && fn != "find_compiling_module") return
        let arg0 = expr.arguments[0]
        if (arg0 != null && arg0 is ExprConstString) {
            st029_note_module_name(string((arg0 as ExprConstString).value))
        }
    }

    def st029_record_mod(m : Module const?) : void {
        if (st029_expr_in_scope()) st029_note_module(m)
    }

    def st029_record_func(fun : Function?) : void {
        return if (fun == null)
        if (fun.fromGeneric != null) st029_record_mod(fun.fromGeneric._module)
        else st029_record_mod(fun._module)
    }

    def st029_record_unresolved_call(expr : ExprCall?) : void {
        if (expr.func != null || !st029_expr_in_scope()) return
        var nm = "{expr.name}"
        let sep = nm |> rfind("::")
        if (sep >= 0) {
            nm = nm |> slice(sep + 2)
        }
        if (!empty(nm) && !key_exists(unresolved_call_names, nm)) {
            unresolved_call_names |> insert(nm)
        }
    }

    def st030_exports_unresolved_name(mod : Module?) : bool {
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
        if (typ.baseType == Type.tStructure && typ.structType != null) st029_record_mod(typ.structType._module)
        elif (typ.baseType == Type.tEnumeration && typ.enumType != null) st029_record_mod(typ.enumType._module)
        elif (typ.baseType == Type.tHandle && typ.annotation != null) st029_record_mod(typ.annotation._module)
    }

    def override preVisitStructure(var st : StructurePtr) : void {
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
        for_each_pass_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_capture_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_simulate_macro(mod) $(_) { found = true; }
        if (found) return true
        for_each_function_annotation(mod) $(_) { found = true; }
        if (found) return true
        return module_has_comment_reader(mod)
    }

    def st029_module_has_lifecycle_hook(mod : Module?) : bool {
        var found = false
        for_each_function(mod, "") $(fn) {
            if (found) return
            for (a in fn.annotations) {
                if (a != null && a.annotation != null) {
                    let an = "{a.annotation.name}"
                    if (an == "init" || an == "finalize" || an == "before_reload"
                            || an == "after_reload" || an == "live_command") {
                        found = true
                        break
                    }
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

    //! every module the require reaches, public or not - what vanishes with the require
    def st029_collect_all_deps(mod : Module?; var reached : table<string; Module?>) : void {
        module_for_each_dependency(mod) $(dep, _is_pub) {
            if (dep == null) return
            let dn = "{dep.name}"
            if (dn == "" || dn == "$" || dn == "builtin") return
            if (!key_exists(reached, dn)) {
                reached |> insert(dn, dep)
                st029_collect_all_deps(dep, reached)
            }
        }
    }

    def st029_cached_has_hook(mod : Module?; var hook_cache : table<string; bool>) : bool {
        let mn = "{mod.name}"
        if (key_exists(hook_cache, mn)) return hook_cache?[mn] ?? false
        let has = st029_module_has_lifecycle_hook(mod)
        hook_cache |> insert(mn, has)
        return has
    }

    //! true when a lifecycle hook rides only through this require: a hook-bearing module in
    //! `closure` that no other require of the file reaches, and that `keep` - the module the
    //! rule would have the file require instead - does not reach either
    def st029_hook_only_through(closure : table<string; Module?>; hook_reach : table<string; int>;
                                var keep : Module?) : bool {
        var kept : table<string; Module?>
        if (keep != null) {
            kept |> insert("{keep.name}", keep)
            st029_collect_all_deps(keep, kept)
        }
        for (mn, reach in keys(hook_reach), values(hook_reach)) {
            if (reach > 1 || !key_exists(closure, mn) || key_exists(kept, mn)) continue
            return true
        }
        return false
    }

    def st029_check_requires(prog : ProgramPtr) : void {
        if (!analyze_requires || this_module == null) return
        let this_name = "{this_module.name}"
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
        // per hook-bearing module, how many of this file's requires reach it - one means it rides only through that require
        var hook_cache : table<string; bool>
        var hook_reach : table<string; int>
        prog |> for_each_require_declaration() $ [unused_argument(req_name, file, pub)] (mod, req_name, file, pub, at) {
            if (mod == null || at.fileInfo == null || !key_exists(module_files, string(at.fileInfo.name))) return
            var reached : table<string; Module?>
            reached |> insert("{mod.name}", mod)
            st029_collect_all_deps(mod, reached)
            for (mn, m in keys(reached), values(reached)) {
                if (st029_cached_has_hook(m, hook_cache)) {
                    hook_reach[mn] = (hook_reach?[mn] ?? 0) + 1
                }
            }
        }
        st030_report_requires(prog, module_files, this_name, hook_reach)
    }

    [arch(at="ARCHITECTURE_LINT.md#style-lint-require-group")]
    def st030_report_requires(prog : ProgramPtr; module_files : table<string>; this_name : string; hook_reach : table<string; int>) : void {
        var group_line : table<string; bool>
        prog |> for_each_require_declaration() $ [unused_argument(req_name, file)] (mod, req_name, file, pub, at) {
            if (mod == null || pub || at.fileInfo == null || !key_exists(module_files, string(at.fileInfo.name))) return
            let key = st030_decl_key(at)
            if (!key_exists(group_line, key)) {
                group_line[key] = st030_is_group_line(rtti_get_source_line(at.fileInfo, at.line))
            }
        }
        var keep : table<string; bool>
        var verdict : table<string; string>
        var members : table<string; string>
        prog |> for_each_require_declaration() $ [unused_argument(file)] (mod, req_name, file, pub, at) {
            if (mod == null || pub
                || at.fileInfo == null || !key_exists(module_files, string(at.fileInfo.name))) return
            let key = st030_decl_key(at)
            let mname = "{mod.name}"
            if (mname == "" || mname == this_name || mname == "$" || mname == "builtin"
                || key_exists(used_modules, mname)
                || st029_module_provides_macro(mod)
                || st029_module_has_lifecycle_hook(mod)
                || st030_exports_unresolved_name(mod)) {
                keep[key] = true
                return
            }
            var closure : table<string; bool>
            st029_collect_public_deps(mod, closure)
            var via : array<string>
            var used_count = 0
            for (cm in keys(closure)) {
                if (!key_exists(used_modules, cm)) continue
                used_count++
                if (closure?[cm] ?? false) via |> push(cm)
            }
            var reached : table<string; Module?>
            reached |> insert(mname, mod)
            st029_collect_all_deps(mod, reached)
            if (used_count == 1 && !empty(via)) {
                let mods = via[0]
                if ((group_line?[key] ?? false) || st029_hook_only_through(reached, hook_reach, reached?[mods] ?? null)) {
                    keep[key] = true
                    return
                }
                verdict["{key}|{req_name}"] = "029:{mods}"
                return
            }
            if (used_count > 0 || st029_hook_only_through(reached, hook_reach, null)) {
                keep[key] = true
                return
            }
            verdict["{key}|{req_name}"] = "030"
            let listed = members?[key] ?? ""
            members[key] = listed == "" ? "{req_name}" : "{listed}, {req_name}"
        }
        var group_reported : table<string; bool>
        prog |> for_each_require_declaration() $ [unused_argument(mod, file, pub)] (mod, req_name, file, pub, at) {
            if (at.fileInfo == null || !key_exists(module_files, string(at.fileInfo.name))) return
            let key = st030_decl_key(at)
            if (keep?[key] ?? false) return
            let v = verdict?["{key}|{req_name}"] ?? ""
            if (v == "") return
            if (starts_with(v, "029:")) {
                let mods = slice(v, 4)
                style_warning("STYLE029: require {req_name} is used only for the module it re-exports ({mods}); require it directly and drop {req_name}", at)
                return
            }
            if (group_line?[key] ?? false) {
                if (group_reported?[key] ?? false) return
                group_reported[key] = true
                style_warning("STYLE030: require [group] is unused - no symbol from any member ({members?[key] ?? ""}) or a module it re-exports is referenced; drop it", at)
                return
            }
            style_warning("STYLE030: require {req_name} is unused - no symbol from it (or a module it re-exports) is referenced; drop it", at)
        }
    }

    def st030_decl_key(at : LineInfo) : string {
        return "{at.fileInfo.name}:{at.line}:{at.column}"
    }

    def st030_is_group_line(src : string) : bool {
        //! `require [group]` or `require ?guard [group]` - the bracket right after the keyword
        //! and the optional guard, not one anywhere on the line
        var group = false
        peek_data(src) $(d) {
            let req = find(d, "require")
            if (req < 0) {
                return
            }
            var i = req + length("require")
            let n = length(d)
            while (i < n && is_tab_or_space(int(d[i]))) {
                i++
            }
            if (i < n && int(d[i]) == '?') {
                while (i < n && !is_tab_or_space(int(d[i])) && int(d[i]) != '[') {
                    i++
                }
                while (i < n && is_tab_or_space(int(d[i]))) {
                    i++
                }
            }
            group = i < n && int(d[i]) == '['
        }
        return group
    }

    def note_decl_span(var file_first_decl : table<string; uint>; fname : string; s : uint) : void {
        if (file_first_decl |> key_exists(fname)) {
            if (s < file_first_decl[fname]) {
                file_first_decl[fname] = s
            }
        } else {
            file_first_decl |> insert(fname, s)
        }
    }

    def scan_long_comment_blocks(prog : ProgramPtr) : void {
        let mod = prog.getThisModule
        if (mod == null) return
        var file_first_decl : table<string; uint>
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
            note_decl_span(file_first_decl, fname, s)
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
            note_decl_span(file_first_decl, fname, st.at.line)
        }
        for_each_enumeration(mod) $(var en) {
            if (en.at.fileInfo == null || en.at.line == 0u) return
            let fname = string(en.at.fileInfo.name)
            note_decl_span(file_first_decl, fname, en.at.line)
        }
        for_each_global(mod) $(var v) {
            if (v.at.fileInfo == null || v.at.line == 0u || v.flags.generated) return
            let fname = string(v.at.fileInfo.name)
            note_decl_span(file_first_decl, fname, v.at.line)
        }
        for_each_typedef(mod) $(_name, value) {
            if (value.at.fileInfo == null || value.at.line == 0u) return
            let fname = string(value.at.fileInfo.name)
            note_decl_span(file_first_decl, fname, value.at.line)
        }
        scan_collected_files(mod, file_first_decl, fn_file, fn_starts, fn_ends, fn_private)
    }

    def scan_collected_files(mod : Module?;
                             var file_first_decl : table<string; uint>;
                             fn_file : array<string>; fn_starts, fn_ends : array<uint>;
                             fn_private : array<bool>) : void {
        var seen : table<string; bool>
        for_each_function(mod, "") $(var fn) {
            if (fn.at.fileInfo == null || fn._module != mod
                || fn.flags.generated || fn.fromGeneric != null || fn.moreFlags.isTemplate) return
            let fname = string(fn.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(fn.at, fname,
                file_first_decl[fname],
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
                file_first_decl[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_enumeration(mod) $(var en) {
            if (en.at.fileInfo == null) return
            let fname = string(en.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(en.at, fname,
                file_first_decl[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_global(mod) $(var v) {
            if (v.at.fileInfo == null || v.flags.generated) return
            let fname = string(v.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(v.at, fname,
                file_first_decl[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_typedef(mod) $(_name, value) {
            if (value.at.fileInfo == null) return
            let fname = string(value.at.fileInfo.name)
            if ((seen |> key_exists(fname)) || !file_first_decl |> key_exists(fname)) return
            seen |> insert(fname, true)
            scan_file_lines(value.at, fname,
                file_first_decl[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
    }

    def scan_file_lines(sample_at : LineInfo; file_name : string;
                        first_decl : uint;
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

    def st040_check_duplicates(prog : ProgramPtr) : void {
        return if (key_exists(disabled_codes, "STYLE040")
                   || (!empty(enabled_codes) && !key_exists(enabled_codes, "STYLE040"))
                   || !duplicate_regions_for(prog))
        var found <- find_extractable_dupes(prog)
        for (f in found) {
            if (f.anchor != null) {
                style_warning(f.message, f.anchor.at)
            }
        }
    }

    def has_nolint_directive(first_line : string) : bool {
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
        if (line_count <= 1
            || block_start < first_decl_line
            || has_nolint_directive(first_line_text)) return
        var enclosing_priv = false
        var inside_function = false
        var enclosing_size : uint = 0xFFFFFFFFu
        for (f_file, s, e, f_priv in fn_file, fn_starts, fn_ends, fn_private) {
            if (f_file != file_name) continue
            if (s <= block_start && block_start <= e) {
                let size = e - s
                if (size < enclosing_size) {
                    enclosing_size = size
                    enclosing_priv = f_priv
                    inside_function = true
                }
            }
        }
        let private_ctx = inside_function && enclosing_priv
        // a '//!' block is a contract doc wherever it sits, so a private one gets the public cap
        let contract_doc = starts_with(first_line_text, "//!")
        let threshold = (private_ctx && !contract_doc) ? 1 : 3
        if (line_count <= threshold) return
        var at = sample_at
        at.line = block_start
        at.last_line = block_start
        at.column = 1u
        at.last_column = 1u
        var msg = ""
        if (private_ctx) {
            msg = "STYLE015: {line_count}-line comment inside 'def private' exceeds {threshold}-line cap; a '//!' contract doc there caps at 3 lines, a plain '//' block at one WHY line - or suppress on the first line with '//!@nolint' (//! blocks) or '// nolint:STYLE015' (// blocks)"
        } else {
            msg = "STYLE014: comment block of {line_count} lines exceeds 3-line cap; trim multi-paragraph prose to a 1-line WHY (move design notes to .md), or suppress on the first line with '//!@nolint' (//! blocks) or '// nolint:STYLE014' (// blocks)"
        }
        style_warning(msg, at)
    }

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
        //! Flag the current top of `unsafe_stack` as containing a non-local let-ref binding. Propagates upward through `visitExpression`.
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
        //! The frame is the subtree's unsafe-requirement summary; mechanics in daslib/ARCHITECTURE_LINT.md.
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
        if (expr.genFlags.userSaidItsSafe && !expr.genFlags.generated
            && (current_function == null || current_function.fromGeneric == null)
            && !(skip_userSaidItsSafe |> has_value(expr))
            && frame.count == 0) {
            style_warning("STYLE024: redundant 'unsafe(...)' wrap; the inner expression has no operation that requires unsafe - drop the wrap", expr.at)
        }
        return expr
    }

    def override preVisitExprLet(expr : ExprLet?) : void {
        //! Mirror `isLocalOrGlobal` check from ast_infer_type.cpp:4989 — `let v & = E` requires unsafe at statement level when `E` is non-local-non-temporary. Mark the let's frame so the enclosing `unsafe { ... }` block can detect it (STYLE025 must stay silent when narrowing would leave the let-ref binding unsatisfied).
        if (expr.genFlags.generated) return
        if (declaration_owns_unsafe(expr)) mark_unsafe_in_stack()
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
            || func.moreFlags.unsafeWhenNotCloneArray
            || (func.moreFlags.unsafeOutsideOfFor && !is_for_loop_src))
    }

    def override preVisitExprDelete(expr : ExprDelete?) : void {
        if (expr.genFlags.generated) return
        if (expr.subexpr == null || expr.subexpr._type == null || !expr.subexpr._type.isSafeToDelete) {
            mark_unsafe_in_stack()
        }
    }

    def is_lowered_unsafe_delete(expr : ExpressionPtr) : bool {
        return expr is ExprCall && lowered_unsafe_delete(expr as ExprCall)
    }

    def variable_owns_unsafe(v : VariablePtr) : bool {
        let vt = v._type
        return false if (v.flags.generated || vt == null)
        return ((!vt.flags.ref && (vt.hasClasses
                || (vt.isStructure && vt.structType != null && vt.structType.flags.isClass)))
            || (!vt.isLocal && !vt.isGoodBlockType)
            || ((vt.flags.smartPtr || (strict_smart_pointers && vt.needInScope)) && !v.flags.inScope)
            || (v.flags.inScope && vt.isPointer && vt.firstType != null
                && vt.firstType.isStructure && vt.firstType.structType != null
                && vt.firstType.structType.flags.isClass))
    }

    def declaration_owns_unsafe(expr : ExpressionPtr) : bool {
        return false if (!(expr is ExprLet))
        for (v in (expr as ExprLet).variables) {
            return true if (variable_owns_unsafe(v))
        }
        return false
    }

    //! `unsafe(delete p)` does not parse, so a block holding a delete anywhere in it has no narrow
    //! form to offer, whichever of its statements the one unsafe operation turns out to be.
    def block_holds_delete(blk : ExprBlock?) : bool {
        for (st in blk.list) {
            return true if (st is ExprDelete || is_lowered_unsafe_delete(st))
        }
        return false
    }

    //! Neither `unsafe(a <- b)` nor `unsafe(a := b)` parses, so a block holding a move or a clone
    //! anywhere in it has no narrow form to offer, whichever statement the one unsafe operation is.
    def block_holds_move_or_clone(blk : ExprBlock?) : bool {
        for (st in blk.list) {
            return true if (st is ExprMove || st is ExprClone || is_lowered_unsafe_clone(st))
        }
        return false
    }

    //! `unsafe(a := b)` does not parse, and a container clone reaches lint as a generated call
    //! rather than an ExprClone, so a block holding one has no narrow form to offer.
    def is_lowered_unsafe_clone(expr : ExpressionPtr) : bool {
        return false if (!(expr is ExprCall))
        let call = expr as ExprCall
        return false if (call.func == null)
        let fname = string(call.func.name)
        return (call.func.flags.generated && fname == "clone") || starts_with(fname, "builtin`clone`")
    }

    //! Infer lowers `delete` to an unflagged finalizer call, and only isSafeToDelete - which this
    //! side cannot read - knows whether the wrap was needed, so every lowered delete counts as
    //! needing it: a redundant wrap is missed rather than a needed one reported.
    def lowered_unsafe_delete(expr : ExprCall?) : bool {
        return false if (expr.func == null || length(expr.arguments) != 1)
        let fname = string(expr.func.name)
        let is_finalize = ((expr.func.flags.generated && fname == "finalize")
            || starts_with(fname, "builtin`finalize`") || starts_with(fname, "builtin`finalize_dim`"))
        return false if (!is_finalize)
        let at = expr.arguments[0]._type
        return at == null || !at.isSafeToDelete
    }

    def override preVisitExprAddr(expr : ExprAddr?) : void {
        st029_record_func(expr.func)
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprCopy(expr : ExprCopy?) : void { st029_record_func(expr.func) }
    //! Mirrors infer's move rules: a class moves only under unsafe, and so does a value that needs
    //! inscope under strict_smart_pointers (a smart pointer moved by value when the option is off);
    //! an untyped operand counts too, so a needed wrap is never reported as redundant.
    def override preVisitExprMove(expr : ExprMove?) : void {
        st029_record_func(expr.func)
        return if (expr.genFlags.generated || expr.left == null || expr.right == null)
        let lt = expr.left._type
        let rt = expr.right._type
        if (lt == null || rt == null || lt.hasClasses) {
            mark_unsafe_in_stack()
        } elif (strict_smart_pointers ? rt.needInScope
                : (rt.isPointer && rt.flags.smartPtr && !rt.flags.ref && !(expr.right is ExprAscend))) {
            mark_unsafe_in_stack()
        }
    }
    def override preVisitExprClone(expr : ExprClone?) : void { st029_record_func(expr.func) }
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (expr.variable != null) st029_record_mod(expr.variable._module)
        if (!empty(st041_candidates) && expr.variable != null && key_exists(st041_candidates, expr.variable)) {
            st041_candidates[expr.variable].observed_refs++
        }
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
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprAsVariant(expr : ExprAsVariant?) : void {
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }
    def override preVisitExprSafeAsVariant(expr : ExprSafeAsVariant?) : void {
        if (!expr.genFlags.generated) mark_unsafe_in_stack()
    }

    def override preVisitExprCast(expr : ExprCast?) : void {
        if (!expr.genFlags.generated
                && (expr.castFlags.upcastCast || expr.castFlags.reinterpretCast)) {
            mark_unsafe_in_stack()
        }
        if (expr.castFlags.reinterpretCast && !expr.castFlags.fromAddrSugar
                && !expr.genFlags.generated
                && expr.castType != null && expr.castType.baseType == Type.tPointer
                && expr.subexpr != null && expr.subexpr is ExprRef2Ptr
                && !expr.subexpr.genFlags.generated) {
            style_warning("STYLE034: reinterpret<{describe(expr.castType)}>(addr(...)) collapses to addr<{describe(expr.castType)}>(...); one unsafe() covers both halves", expr.at)
        }
        if (!expr.genFlags.generated && expr.castType != null
                && (current_function == null || current_function.fromGeneric == null)) {
            let inert = style036_inert_contract(expr.castType)
            if (!empty(inert)) {
                let tail = inert == "-const" ? "; drop it, along with any 'const' it was meant to cancel" : "; drop it"
                style_warning("STYLE036: '{inert}' does nothing on the already-resolved cast type {describe(expr.castType)}{tail}", expr.at)
            }
        }
    }

    def style036_inert_contract(t : TypeDecl?) : string {
        if (t.isAutoOrAlias) return ""
        if (t.flags.removeConstant) return "-const"
        if (t.flags.removeRef) return "-&"
        if (t.flags.removeDim) return "-[]"
        if (t.flags.removeTemporary) return "-#"
        if (t.flags.explicitConst) return "==const"
        if (t.flags.explicitRef) return "==&"
        return ""
    }

    def override preVisitExprAt(expr : ExprAt?) : void {
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
        if (bt == Type.tPointer) {
            let inner = expr.subexpr._type.firstType
            if (inner == null || !(inner.isVectorType || inner.baseType == Type.tFixedArray)) {
                mark_unsafe_in_stack()
            }
        }
    }

    def override preVisitExprField(expr : ExprField?) : void {
        if (!expr.genFlags.generated && expr.value != null
                && expr.value._type != null
                && expr.value._type.baseType == Type.tVariant) {
            mark_unsafe_in_stack()
        }
    }

    def override preVisitExprReturn(expr : ExprReturn?) : void {
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
        if (!expr.genFlags.generated) st037_bump()
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
            style_warning("STYLE026: nested 'unsafe \{ ... }' block; the outer 'unsafe' already covers this scope - drop the inner wrap", expr.at)
        }
        if (n > 0) {
            unsafe_block_stack[n - 1]++
        }
    }

    def is_module_with(expr : ExpressionPtr) : bool {
        return expr is ExprWith && !empty((expr as ExprWith).moduleName)
    }

    def override preVisitExprWith(var expr : ExprWith?) : void {
        if (with_module_is_unsafe && !expr.genFlags.generated && !empty(expr.moduleName)) {
            mark_unsafe_in_stack()
        }
    }

    def override visitExprUnsafe(var expr : ExprUnsafe?) : ExpressionPtr {
        let n = length(unsafe_block_stack)
        if (n > 0) {
            unsafe_block_stack[n - 1]--
        }
        if (expr == null || expr.body == null || expr.genFlags.generated
            || (current_function != null && current_function.fromGeneric != null)
            || !(expr.body is ExprBlock)) return expr
        let blk = expr.body as ExprBlock
        let frame = unsafe_frame_for(blk)
        if (frame.count == 0 && !frame.has_non_local_let_ref) {
            style_warning("STYLE024: redundant 'unsafe \{ ... }' block; no statement requires unsafe - drop the wrap", expr.at)
        } elif (frame.count == 1 &&
                !(blk.list[0] is ExprYield ||
                  blk.list[0] is ExprDelete ||
                  blk.list[0] is ExprReturn ||
                  blk.list[0] is ExprNew ||
                  declaration_owns_unsafe(blk.list[0]) ||
                  is_module_with(blk.list[0]) ||
                  block_holds_delete(blk) ||
                  block_holds_move_or_clone(blk))
                && !frame.has_non_local_let_ref) {
            var narrowable = true
            if (blk.list[0] is ExprFor) {
                let fbody = (blk.list[0] as ExprFor).body
                if (fbody != null && fbody is ExprBlock) {
                    let fblk = fbody as ExprBlock
                    var all_deletes = !empty(fblk.list)
                    for (s in fblk.list) {
                        if (!(s is ExprDelete)) {
                            all_deletes = false
                        }
                    }
                    if (all_deletes) {
                        narrowable = false
                    }
                }
            }
            if (narrowable) {
                style_warning("STYLE025: 'unsafe \{ ... }' block scope is too broad; only one operation requires unsafe - narrow to 'unsafe(<sub-expr>)' wrapping that operation", expr.at)
            }
        }
        return expr
    }
}

def private require_analysis_enabled(disabled, enabled : table<string>) : bool {
    var any = false
    for (code in ["STYLE029", "STYLE030"]) {
        if (key_exists(disabled, code) || (!empty(enabled) && !key_exists(enabled, code))) continue
        any = true
    }
    return any
}

def private comment_hygiene_for(prog : ProgramPtr; requested : bool) : bool {
    //! Explicit callers (e.g. --comment-hygiene) force the checks on. Otherwise a module-local option overrides the path default, and daslib/ defaults on.
    if (requested) return true
    let configured = prog._options |> find_arg("_comment_hygiene")
    if (configured is tBool) return configured as tBool
    return is_daslib_source(prog)
}

def private ascii_strings_for(prog : ProgramPtr) : bool {
    let configured = prog._options |> find_arg("_ascii_strings")
    if (configured is tBool) return configured as tBool
    if (lint_config_forces_on("STYLE039")) return true
    return is_shipped_library_source(prog)
}

//! infer falls back to the host's policy, which lint cannot read; the policy header's default is on.
def private strict_smart_pointers_for(prog : ProgramPtr) : bool {
    return prog._options |> find_arg("strict_smart_pointers") ?as tBool ?? true
}

def private duplicate_regions_for(prog : ProgramPtr) : bool {
    let configured = prog._options |> find_arg("_duplicate_regions")
    if (configured is tBool) return configured as tBool
    if (lint_config_forces_on("STYLE040")) return true
    return is_core_library_source(prog)
}

def private max_complexity_for(prog : ProgramPtr) : int {
    let configured = prog._options |> find_arg("_cyclomatic_complexity") ?as tInt ?? STYLE037_DEFAULT_MAX
    return max(configured, 0)
}

def private max_function_length_for(prog : ProgramPtr) : int {
    let configured = prog._options |> find_arg("_function_length") ?as tInt ?? STYLE038_DEFAULT_MAX_LINES
    return max(configured, 0)
}

def public style_lint(prog : ProgramPtr; compile_time_errors : bool; comment_hygiene : bool = false) : int {
    //! Runs the style lint visitor on the compiled program.
    //! Returns the number of warnings found.
    //! STYLE014/STYLE015 default on for daslib sources; pass ``comment_hygiene = true`` to force them on elsewhere.
    let empty_set : table<string>
    return style_lint(prog, compile_time_errors, empty_set, [comment_hygiene = comment_hygiene])
}

def public style_lint(prog : ProgramPtr; compile_time_errors : bool; disabled_codes : table<string>; comment_hygiene : bool = false) : int {
    //! Filter-aware compile-time variant. ``disabled_codes`` suppresses
    //! the listed rule IDs (e.g. "STYLE005") alongside the usual
    //! ``// nolint:CODE`` directives.
    let hygiene = comment_hygiene_for(prog, comment_hygiene)
    var astVisitor = new StyleLintVisitor(compile_time_errors = compile_time_errors, comment_hygiene = hygiene)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.max_complexity = max_complexity_for(prog)
    astVisitor.max_function_length = max_function_length_for(prog)
    astVisitor.ascii_strings = ascii_strings_for(prog)
    astVisitor.strict_smart_pointers = strict_smart_pointers_for(prog)
    astVisitor.with_module_is_unsafe = (prog._options |> find_arg("with_module_is_unsafe")) ?as tBool ?? false
    astVisitor.this_module = prog.getThisModule
    let empty_enabled : table<string>
    astVisitor.analyze_requires = astVisitor.this_module != null && require_analysis_enabled(disabled_codes, empty_enabled)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    astVisitor->st029_check_requires(prog)
    astVisitor->st040_check_duplicates(prog)
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
    //! STYLE014/STYLE015 default on for daslib sources; pass ``comment_hygiene = true`` to force them on elsewhere.
    let empty_set : table<string>
    return style_lint_collect(prog, warnings, empty_set, empty_set, comment_hygiene)
}

def public style_lint_collect(prog : ProgramPtr; var warnings : array<string>;
                              disabled_codes, enabled_codes : table<string>;
                              comment_hygiene : bool = false) : int {
    //! Filter-aware overload. `disabled_codes` is a denylist; `enabled_codes`
    //! is a whitelist (empty == all). Codes use the bare form (e.g. "STYLE024").
    let hygiene = comment_hygiene_for(prog, comment_hygiene)
    var astVisitor = new StyleLintVisitor(compile_time_errors = false, collect_warnings = true, comment_hygiene = hygiene)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    astVisitor.max_complexity = max_complexity_for(prog)
    astVisitor.max_function_length = max_function_length_for(prog)
    astVisitor.ascii_strings = ascii_strings_for(prog)
    astVisitor.strict_smart_pointers = strict_smart_pointers_for(prog)
    astVisitor.with_module_is_unsafe = (prog._options |> find_arg("with_module_is_unsafe")) ?as tBool ?? false
    astVisitor.this_module = prog.getThisModule
    astVisitor.analyze_requires = astVisitor.this_module != null && require_analysis_enabled(disabled_codes, enabled_codes)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    astVisitor->st029_check_requires(prog)
    astVisitor->st040_check_duplicates(prog)
    if (!empty(astVisitor.unsafe_stack)) {
        panic("style_lint internal: unsafe_stack not balanced after walk (size={length(astVisitor.unsafe_stack)})")
    }
    let count = astVisitor.warning_count
    warnings |> push_from(astVisitor.warnings)
    unsafe { delete astVisitor; }
    return count
}

def public style_lint_collect_issues(prog : ProgramPtr; var issues : array<LintIssue>;
                                     disabled_codes, enabled_codes : table<string>;
                                     comment_hygiene : bool = false) : int {
    //! Structured twin of ``style_lint_collect`` — same filters, but emits
    //! ``LintIssue`` records (rule code + position) instead of display strings.
    let hygiene = comment_hygiene_for(prog, comment_hygiene)
    var astVisitor = new StyleLintVisitor(compile_time_errors = false, collect_warnings = true, comment_hygiene = hygiene)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    astVisitor.max_complexity = max_complexity_for(prog)
    astVisitor.max_function_length = max_function_length_for(prog)
    astVisitor.ascii_strings = ascii_strings_for(prog)
    astVisitor.strict_smart_pointers = strict_smart_pointers_for(prog)
    astVisitor.with_module_is_unsafe = (prog._options |> find_arg("with_module_is_unsafe")) ?as tBool ?? false
    astVisitor.this_module = prog.getThisModule
    astVisitor.analyze_requires = astVisitor.this_module != null && require_analysis_enabled(disabled_codes, enabled_codes)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    astVisitor->st029_check_requires(prog)
    astVisitor->st040_check_duplicates(prog)
    if (!empty(astVisitor.unsafe_stack)) {
        panic("style_lint internal: unsafe_stack not balanced after walk (size={length(astVisitor.unsafe_stack)})")
    }
    let count = astVisitor.warning_count
    issues |> push_from(astVisitor.issues)
    unsafe { delete astVisitor; }
    return count
}

[lint_macro]
class StyleLintMacro : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        let hygiene = prog._options |> find_arg("_comment_hygiene") ?as tBool ?? false
        let disabled <- build_lint_macro_disabled(prog)
        style_lint(prog, true, disabled, [comment_hygiene = hygiene])
        return true
    }
}
