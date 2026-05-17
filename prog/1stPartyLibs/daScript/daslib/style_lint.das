options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module style_lint shared private

//! Style lint module.
//!
//! Detects non-idiomatic patterns in daslang code at compile time.
//! When this module is required, a lint pass runs after compilation and reports
//! warnings as ``CompilationError::style_lint`` (error code 40218).
//!
//! Rules:
//!   STYLE001 — unnecessary <| pipe before block argument
//!   STYLE002 — <| pipe before parameterless block (remove pipe and $())
//!   STYLE003 — redundant $() on parameterless block argument
//!   STYLE005 — single-statement if can use postfix conditional (configurable, off by default)
//!   STYLE006 — string(__rtti) comparison should use `is` operator
//!   STYLE010 — if (true) should be a bare block
//!   STYLE011 — variable declaration followed by immediate assignment
//!   STYLE012 — array<T> var initialized via push/emplace; use an array literal
//!   STYLE013 — struct var with default/empty init followed by a run of field assignments
//!   STYLE014 — comment block exceeds 3 lines at module/public scope (opt-in via ``options _comment_hygiene = true``, suppress with '//!@nolint' or '// nolint:STYLE014')
//!   STYLE015 — comment block exceeds 1 line inside a 'def private' (opt-in via ``options _comment_hygiene = true``, suppress with '// nolint:STYLE015')

require daslib/ast_boost
require strings

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------

class StyleLintVisitor : AstVisitor {
    compile_time_errors : bool
    postfix_conditionals : bool = false
    comment_hygiene : bool = false
    // counter
    warning_count : int = 0
    // collection mode
    collect_warnings : bool = false
    warnings : array<string>
    // dedup: suppress duplicate warnings from generic instantiations
    reported_locations : array<uint64>
    // current function (for inferStack reporting)
    @do_not_delete current_function : Function?
    // STYLE011: track uninitialized variables from most recent ExprLet
    @do_not_delete pending_uninit_vars : array<Variable?>

    def StyleLintVisitor() {
        pass
    }

    def location_key(at : LineInfo) : uint64 {
        return uint64(at.line) | (uint64(at.column) << uint64(20))
    }

    def is_suppressed(text : string; at : LineInfo) : bool {
        if (at.fileInfo == null || at.line == 0u) {
            return false
        }
        let colon = find(text, ":")
        if (colon < 0) {
            return false
        }
        let code = slice(text, 0, colon)
        var found = false
        get_file_source_line(at.fileInfo, at.line) $(line) {
            let nolint_pos = find(line, "nolint")
            if (nolint_pos >= 0) {
                found = find(line, code, nolint_pos) >= 0
            }
        }
        return found
    }

    def style_warning(text : string; at : LineInfo) : void {
        let key = self->location_key(at)
        if (reported_locations |> has_value(key)) {
            return
        }
        reported_locations |> push(key)
        if (self->is_suppressed(text, at)) {
            return
        }
        warning_count++
        var msg = text
        if (current_function != null && current_function.fromGeneric != null && length(current_function.inferStack) > 0) {
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
        if (at_start.fileInfo == null) {
            return result
        }
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
        self->check_defer_pipe(fn)
    }

    // --- STYLE001/002 for defer <| (macro erases AST, scan source lines) ---

    def check_defer_pipe(fn : FunctionPtr) : void {
        if (fn.body == null || fn.at.fileInfo == null || fn.at.line == 0u) {
            return
        }
        let body_at = fn.body.at
        for (ln in range(int(body_at.line), int(body_at.last_line) + 1)) {
            var src = ""
            get_file_source_line(fn.at.fileInfo, uint(ln)) $(line) {
                src := line
            }
            let comment_pos = find(src, "//")
            let defer_pos = find(src, "defer")
            if (defer_pos < 0 || (comment_pos >= 0 && comment_pos < defer_pos)) {
                continue
            }
            if (defer_pos > 0) {
                var preceding_ok = false
                peek_data(src) $(data) {
                    let ch = int(data[defer_pos - 1])
                    preceding_ok = ch == ' ' || ch == '\t'
                }
                if (!preceding_ok) {
                    continue
                }
            }
            let pipe_pos = find(src, "<|", defer_pos + 5)
            if (pipe_pos < 0) {
                continue
            }
            let after_pipe = strip_left(slice(src, pipe_pos + 2))
            let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
            if (!pipes_block) {
                continue
            }
            var at = fn.at
            at.line = uint(ln)
            at.column = uint(defer_pos)
            at.last_line = uint(ln)
            at.last_column = uint(defer_pos + 5)
            if (starts_with(after_pipe, "$()")) {
                self->style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", at)
            } else {
                self->style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", at)
            }
        }
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        current_function = null
        return <- fn
    }

    // --- STYLE001/002/003/011: pipe and redundant $() on block arguments ---

    def check_block_pipe(call_at : LineInfo; var arguments : dasvector`ptr`Expression) : void {
        let nargs = length(arguments)
        if (nargs == 0) {
            return
        }
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
                        let src = self->source_line_between(call_at, eblk.at)
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
                                        self->style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", call_at)
                                    } else {
                                        self->style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", call_at)
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
        self->check_block_pipe(expr.at, expr.arguments)
    }

    def override preVisitExprInvoke(var expr : ExprInvoke?) : void {
        self->check_block_pipe(expr.at, expr.arguments)
    }


    def override preVisitExprMakeGenerator(var expr : ExprMakeGenerator?) : void {
        self->check_block_pipe(expr.at, expr.arguments)
    }

    // --- STYLE001/002 for desugared generators ---

    def override preVisitExprMakeStruct(var expr : ExprMakeStruct?) : void {
        // generator<T>() <| $ { ... } is lowered before lint runs — ExprMakeGenerator
        // is replaced by ExprMakeStruct. Detect via source line inspection.
        if (expr.at.fileInfo == null || expr.at.line == 0u) {
            return
        }
        var src = ""
        get_file_source_line(expr.at.fileInfo, expr.at.line) $(line) {
            src := line
        }
        let col = int(expr.at.column)
        if (col >= length(src) || !starts_with(slice(src, col), "generator<")) {
            return
        }
        let pipe_pos = find(src, "<|", col + 10)
        if (pipe_pos < 0) {
            return
        }
        let after_pipe = strip_left(slice(src, pipe_pos + 2))
        let pipes_block = starts_with(after_pipe, "$(") || starts_with(after_pipe, "$ ") || starts_with(after_pipe, "$\{") || starts_with(after_pipe, "@") || starts_with(after_pipe, "\{")
        if (!pipes_block) {
            return
        }
        let arrow_pos = find(after_pipe, "=>")
        let brace_pos = find(after_pipe, "\{")
        let is_lambda_shorthand = arrow_pos >= 0 && (brace_pos < 0 || arrow_pos < brace_pos)
        if (is_lambda_shorthand) {
            return
        }
        if (starts_with(after_pipe, "$()")) {
            self->style_warning("STYLE002: remove <| pipe and $() for parameterless block; use direct trailing block", expr.at)
        } else {
            self->style_warning("STYLE001: remove <| pipe before block argument; use direct trailing block syntax", expr.at)
        }
    }

    // --- STYLE003: redundant $() on parameterless blocks (all contexts) ---

    def override preVisitExprMakeBlock(var expr : ExprMakeBlock?) : void {
        var blk = expr._block
        if (!(blk is ExprBlock)) {
            return
        }
        var eblk = blk as ExprBlock
        if (!eblk.blockFlags.isClosure || eblk.blockFlags.isLambdaBlock || length(eblk.arguments) != 0) {
            return
        }
        let src = self->source_line_between(expr.at, eblk.at)
        let dollar_pos = find(src, "$()")
        if (dollar_pos < 0) {
            return
        }
        // Skip if <| pipe is present — STYLE002 already covers that case
        if (find(src, "<|") >= 0) {
            return
        }
        // Skip if explicit return type follows: $() : Type { ... }
        let after_dollar = strip_left(slice(src, dollar_pos + 3))
        if (starts_with(after_dollar, ":")) {
            return
        }
        self->style_warning("STYLE003: remove redundant $() for parameterless block; use $ \{", expr.at)
    }

    // --- STYLE006: string(__rtti) == "..." should use `is` ---

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (expr.op != "==") {
            return
        }
        if (self->is_string_rtti_comparison(expr.left) || self->is_string_rtti_comparison(expr.right)) {
            self->style_warning("STYLE006: string(__rtti) comparison should use `is` operator; e.g. expr is ExprFoo", expr.at)
        }
    }

    // --- STYLE005: postfix conditionals (configurable) ---
    // --- STYLE010: if (true) should be bare block ---

    def override preVisitExprIfThenElse(ifte : ExprIfThenElse?) : void {
        if (ifte.if_flags.isStatic) {
            return
        }
        // STYLE010: if (true) { ... }
        var cond = ifte.cond
        if (cond is ExprConstBool) {
            var cbool = cond as ExprConstBool
            if (cbool.value && ifte.if_false == null) {
                self->style_warning("STYLE010: if (true) is always taken; use a bare block instead", ifte.at)
            }
        }
        if (postfix_conditionals && ifte.if_false == null) {
            var then_expr = ifte.if_true
            if (then_expr is ExprBlock) {
                var blk = then_expr as ExprBlock
                if (length(blk.list) == 1 && length(blk.finalList) == 0) {
                    var stmt = blk.list[0]
                    if (stmt is ExprReturn || stmt is ExprBreak || stmt is ExprContinue) {
                        // Skip if already on one line (already postfix or one-liner)
                        if (ifte.at.line != stmt.at.line) {
                            self->style_warning("STYLE005: single-statement if can use postfix form; e.g. return val if (cond)", ifte.at)
                        }
                    }
                }
            }
        }
    }

    // --- STYLE011: variable declaration followed by immediate assignment ---

    def override preVisitExprLetVariable(expr : ExprLet?; var v : VariablePtr; last : bool) : void {
        if (v.init == null && !v.flags.generated && !v.flags.inScope && (current_function == null || current_function.fromGeneric == null)) {
            pending_uninit_vars |> push(v)
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

    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) : void {
        if (length(pending_uninit_vars) > 0) {
            if (expr is ExprCopy) {
                let v = self->find_assign_var((expr as ExprCopy).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    self->style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            } elif (expr is ExprClone) {
                let v = self->find_assign_var((expr as ExprClone).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    self->style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            } elif (expr is ExprMove) {
                let v = self->find_assign_var((expr as ExprMove).left)
                if (v != null && pending_uninit_vars |> has_value(v)) {
                    self->style_warning("STYLE011: variable declaration followed by immediate assignment; combine into single declaration with initialization", expr.at)
                }
            }
            pending_uninit_vars |> resize(0)
        }
        if (expr is ExprLet) {
            self->check_style012_array_init(blk, expr as ExprLet)
            self->check_style013_struct_field_init(blk, expr as ExprLet)
        }
    }

    // --- STYLE012: array<T> var then contiguous push/emplace instead of literal ---

    def check_style012_array_init(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) {
            return
        }
        // Find this ExprLet's index in the enclosing block's statement list.
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) {
            return
        }
        for (v in elet.variables) {
            if (v.init != null) {
                continue
            }
            if (v.flags.generated || v.flags.inScope) {
                continue
            }
            if (v._type == null || v._type.baseType != Type.tArray) {
                continue
            }
            if (length(v._type.dim) != 0) {
                continue
            }
            var count = 0
            for (j in range(idx + 1, n)) {
                let stmt = blk.list[j]
                if (!(stmt is ExprCall)) {
                    break
                }
                if (!self->is_push_or_emplace_of(stmt as ExprCall, v)) {
                    break
                }
                count++
            }
            if (count >= 2) {
                self->style_warning("STYLE012: array<T> initialized by a run of push/emplace calls; use an array literal `var a <- [x, <-y, z]` (or `var a <- array<T>(x, y, z)` when you need an explicit element type, e.g. for polymorphic upcasts)", v.at)
            }
        }
    }

    // --- STYLE013: struct var with default/empty init then a run of field assignments ---

    def check_style013_struct_field_init(blk : ExprBlock?; elet : ExprLet?) : void {
        if (current_function != null && current_function.fromGeneric != null) {
            return
        }
        var idx = -1
        let n = length(blk.list)
        for (k in range(n)) {
            if (blk.list[k] == elet) {
                idx = k
                break
            }
        }
        if (idx < 0 || idx + 1 >= n) {
            return
        }
        for (v in elet.variables) {
            if (v.flags.generated || v.flags.inScope) {
                continue
            }
            if (v._type == null || v._type.baseType != Type.tStructure) {
                continue
            }
            if (length(v._type.dim) != 0) {
                continue
            }
            if (!self->is_default_struct_init(v)) {
                continue
            }
            var count = 0
            for (j in range(idx + 1, n)) {
                if (!self->is_field_assign_of(blk.list[j], v)) {
                    break
                }
                count++
            }
            if (count >= 2) {
                self->style_warning("STYLE013: struct var (default-init or uninitialized) followed by a run of field assignments; use a named-argument constructor `var a = Foo(bar = ..., far = ...)`", v.at)
            }
        }
    }

    def is_default_struct_init(v : Variable?) : bool {
        //! True for `var a : Foo` (no init) or `var a = Foo()` (zero-arg MakeStruct).
        //! False for any non-empty constructor or non-MakeStruct init — those are
        //! "modify an existing value" cases, not the bug we want to flag.
        return true if (v.init == null)
        return false if (!(v.init is ExprMakeStruct))
        var ms = v.init as ExprMakeStruct
        if (ms._block != null) {
            return false
        }
        for (row in ms.structs) {
            for (_mf in *row) {
                // any field present → not the zero-arg constructor
                return false
            }
        }
        return true
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
        let src = (lhs as ExprField).value
        return false if (src == null || !(src is ExprVar))
        return (src as ExprVar).variable == target
    }

    def is_push_or_emplace_of(call : ExprCall?; target : Variable?) : bool {
        if (call.func == null || call.func.fromGeneric == null) {
            return false
        }
        if (call.func.fromGeneric.name != "push" && call.func.fromGeneric.name != "emplace") {
            return false
        }
        if (length(call.arguments) < 2) {
            return false
        }
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (!(arg is ExprVar)) {
            return false
        }
        return (arg as ExprVar).variable == target
    }

    // --- STYLE014/STYLE015: large comment blocks (post-visit pass) ---

    def scan_long_comment_blocks(prog : ProgramPtr) : void {
        let mod = prog.getThisModule
        if (mod == null) {
            return
        }
        var file_first_decl : table<string; uint>
        var file_max_line : table<string; uint>
        var fn_file : array<string>
        var fn_starts : array<uint>
        var fn_ends : array<uint>
        var fn_private : array<bool>
        for_each_function(mod, "") $(var fn) {
            if (fn.body == null || fn.at.fileInfo == null || fn.at.line == 0u) {
                return
            }
            if (fn.flags.generated || fn.fromGeneric != null || fn._module != mod) {
                return
            }
            if (fn.moreFlags.isTemplate) {
                return
            }
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
            if (st.at.fileInfo == null || st.at.line == 0u) {
                return
            }
            if (st.flags.generated || st._module != mod || st.flags.isTemplate) {
                return
            }
            var is_template_instance = false
            peek(st.name) $(name) {
                is_template_instance = find(name, "<") >= 0
            }
            if (is_template_instance) {
                return
            }
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
            if (en.at.fileInfo == null || en.at.line == 0u) {
                return
            }
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
            if (v.at.fileInfo == null || v.at.line == 0u) {
                return
            }
            if (v.flags.generated) {
                return
            }
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
        for_each_typedef(mod) $(name, value) {
            if (value.at.fileInfo == null || value.at.line == 0u) {
                return
            }
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
            if (fn.at.fileInfo == null || fn._module != mod) {
                return
            }
            if (fn.flags.generated || fn.fromGeneric != null || fn.moreFlags.isTemplate) {
                return
            }
            let fname = string(fn.at.fileInfo.name)
            if (seen |> key_exists(fname)) {
                return
            }
            if (!file_first_decl |> key_exists(fname)) {
                return
            }
            seen |> insert(fname, true)
            self->scan_file_lines(fn.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_structure(mod) $(var st) {
            if (st.at.fileInfo == null || st._module != mod) {
                return
            }
            if (st.flags.generated || st.flags.isTemplate) {
                return
            }
            var is_template_instance = false
            peek(st.name) $(name) {
                is_template_instance = find(name, "<") >= 0
            }
            if (is_template_instance) {
                return
            }
            let fname = string(st.at.fileInfo.name)
            if (seen |> key_exists(fname)) {
                return
            }
            if (!file_first_decl |> key_exists(fname)) {
                return
            }
            seen |> insert(fname, true)
            self->scan_file_lines(st.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_enumeration(mod) $(var en) {
            if (en.at.fileInfo == null) {
                return
            }
            let fname = string(en.at.fileInfo.name)
            if (seen |> key_exists(fname)) {
                return
            }
            if (!file_first_decl |> key_exists(fname)) {
                return
            }
            seen |> insert(fname, true)
            self->scan_file_lines(en.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_global(mod) $(var v) {
            if (v.at.fileInfo == null || v.flags.generated) {
                return
            }
            let fname = string(v.at.fileInfo.name)
            if (seen |> key_exists(fname)) {
                return
            }
            if (!file_first_decl |> key_exists(fname)) {
                return
            }
            seen |> insert(fname, true)
            self->scan_file_lines(v.at, fname,
                file_first_decl[fname], file_max_line[fname],
                fn_file, fn_starts, fn_ends, fn_private)
        }
        for_each_typedef(mod) $(name, value) {
            if (value.at.fileInfo == null) {
                return
            }
            let fname = string(value.at.fileInfo.name)
            if (seen |> key_exists(fname)) {
                return
            }
            if (!file_first_decl |> key_exists(fname)) {
                return
            }
            seen |> insert(fname, true)
            self->scan_file_lines(value.at, fname,
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
            if (!got) {
                break
            }
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
                    self->emit_long_comment(sample_at, file_name, first_decl, block_start, block_count, block_first_line,
                        fn_file, fn_starts, fn_ends, fn_private)
                    in_block = false
                }
            }
            ln++
        }
        if (in_block) {
            self->emit_long_comment(sample_at, file_name, first_decl, block_start, block_count, block_first_line,
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
        if (line_count <= 1) {
            return
        }
        if (block_start < first_decl_line) {
            return
        }
        if (self->has_nolint_directive(first_line_text)) {
            return
        }
        var enclosing_priv = false
        var inside_function = false
        var enclosing_size : uint = 0xFFFFFFFFu
        for (i in range(length(fn_file))) {
            if (fn_file[i] != file_name) {
                continue
            }
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
        if (line_count <= threshold) {
            return
        }
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
        self->style_warning(msg, at)
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
        if (call.name != "string" || length(call.arguments) < 1) {
            return false
        }
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
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

def public style_lint(prog : ProgramPtr; compile_time_errors : bool; postfix_conditionals : bool = false; comment_hygiene : bool = false) : int {
    //! Runs the style lint visitor on the compiled program.
    //! Returns the number of warnings found.
    //! Pass ``comment_hygiene = true`` to enable STYLE014/STYLE015 checks.
    var astVisitor = new StyleLintVisitor(compile_time_errors = compile_time_errors, postfix_conditionals = postfix_conditionals, comment_hygiene = comment_hygiene)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (comment_hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    let count = astVisitor.warning_count
    unsafe { delete astVisitor; }
    return count
}

def public style_lint_collect(prog : ProgramPtr; var warnings : array<string>; postfix_conditionals : bool = false; comment_hygiene : bool = false) : int {
    //! Runs the style lint visitor and collects warnings as strings.
    //! Returns the number of warnings found.
    //! Pass ``comment_hygiene = true`` to enable STYLE014/STYLE015 checks.
    var astVisitor = new StyleLintVisitor(compile_time_errors = false, collect_warnings = true, postfix_conditionals = postfix_conditionals, comment_hygiene = comment_hygiene)
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit_with_generics(prog, astVisitorAdapter)
    }
    if (comment_hygiene) {
        astVisitor->scan_long_comment_blocks(prog)
    }
    let count = astVisitor.warning_count
    warnings |> reserve(length(astVisitor.warnings))
    for (w in astVisitor.warnings) {
        warnings |> push(w)
    }
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
        style_lint(prog, true, [comment_hygiene = hygiene])
        return true
    }
}
