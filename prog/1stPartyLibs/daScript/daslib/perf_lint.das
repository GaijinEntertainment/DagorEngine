options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module perf_lint shared private

//! Performance lint module.
//!
//! Detects common performance anti-patterns in daslang code at compile time.
//! When this module is required, a lint pass runs after compilation and reports
//! warnings as ``CompilationError::performance_lint`` (error code 40217).
//!
//! Rules:
//!   PERF001 — string += in loop (O(n²))
//!   PERF002 — character_at in loop with loop variable index (O(n) per call)
//!   PERF003 — character_at anywhere (info: O(n) per call)
//!   PERF004 — string interpolation reassignment in loop (O(n²))
//!   PERF005 — length(string) in while condition (strlen each iteration)
//!   PERF006 — push/emplace in loop without prior reserve()
//!   PERF007 — unnecessary string(das_string) in comparison
//!   PERF008 — unnecessary get_ptr() for is/as type checks
//!   PERF009 — redundant move-init variable immediately returned

require daslib/ast_boost
require strings

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------

struct VarStackEntry {
    @do_not_delete v : Variable?
    depth : int
    is_iter : bool
}

class PerfLintVisitor : AstVisitor {
    compile_time_errors : bool
    // variable + loop tracking
    loop_depth : int = 0
    in_closure : int = 0
    var_stack : array<VarStackEntry>
    scope_stack : array<int>
    // while tracking
    in_while_cond : bool = false
    // counter-based detection state
    in_character_at_call : int = 0
    @do_not_delete current_character_at : ExprCall?
    in_length_while_call : int = 0
    in_string_builder_check : int = 0
    @do_not_delete string_builder_target_var : Variable?
    @do_not_delete string_builder_save_stack : array<Variable?>
    // reported character_at locations (to avoid duplicate PERF002+PERF003)
    @do_not_delete reported_character_at : array<ExprCall?>
    // PERF006: path keys for arrays that had reserve() called
    reserved_paths : array<string>
    // PERF006: if-depth per scope — push(0) on scope entry, pop on exit
    if_depth_stack : array<int>
    // current function being visited (for inferStack reporting)
    @do_not_delete current_function : Function?
    // PERF006: known-length loop tracking
    known_length_loop_depth : int = 0
    current_for_known_length : bool = true
    // PERF006: suppress when loop has break/continue (unpredictable iteration count)
    loop_has_early_exit : array<bool>
    // PERF009: track last move-initialized variable for redundant return detection
    @do_not_delete pending_move_var : Variable?
    @safe_when_uninitialized pending_move_at : LineInfo
    warning_count : int = 0
    // collection mode — when true, warnings are appended to `warnings` instead of to_log
    collect_warnings : bool = false
    warnings : array<string>
    // dedup: reported (file_hash, line, column) to suppress duplicate warnings from generic instantiations
    reported_locations : array<uint64>
    def PerfLintVisitor() {
        pass
    }

    def location_key(at : LineInfo) : uint64 {
        return uint64(at.line) | (uint64(at.column) << uint64(20))
    }

    def is_suppressed(text : string; at : LineInfo) : bool {
        if (at.fileInfo == null || at.line == 0u) {
            return false
        }
        // extract PERFxxx code from warning text (e.g. "PERF003: ...")
        let colon = find(text, ":")
        if (colon < 0) {
            return false
        }
        let code = slice(text, 0, colon)
        var found = false
        get_file_source_line(at.fileInfo, at.line) <| $(line) {
            // look for "// nolint:PERFxxx" or "// nolint(PERFxxx)" in a comment
            let nolint_pos = find(line, "nolint")
            if (nolint_pos >= 0) {
                found = find(line, code, nolint_pos) >= 0
            }
        }
        return found
    }

    def perf_warning(text : string; at : LineInfo) : void {
        // Deduplicate warnings — same source location reported once, regardless of generic instantiations
        let key = self->location_key(at)
        if (reported_locations |> has_value(key)) {
            return
        }
        reported_locations |> push(key)
        // Check inline suppression: // PERFxxx on the same line
        if (self->is_suppressed(text, at)) {
            return
        }
        warning_count++
        var msg = text
        if (current_function != null && current_function.fromGeneric != null && length(current_function.inferStack) > 0) {
            msg = build_string() <| $(var w) {
                w |> write(text)
                w |> write("\n  while compiling {current_function.name}")
                for (ih in current_function.inferStack) {
                    w |> write("\n  instanced from {ih.func.name} at {describe(ih.at)}")
                }
            }
        }
        if (compile_time_errors) {
            compiling_program() |> macro_performance_warning(at, msg)
        } elif (collect_warnings) {
            warnings |> push("performance warning: {msg} at {describe(at)}")
        } else {
            to_log(LOG_WARNING, "performance warning: {msg} at {describe(at)}\n")
        }
    }

    // --- variable scope helpers ---

    def is_loop_variable(v : Variable?) : bool {
        if (v == null) {
            return false
        }
        for (entry in var_stack) {
            if (entry.v == v && entry.is_iter) {
                return true
            }
        }
        return false
    }

    def is_defined_outside_loop(v : Variable?) : bool {
        if (v == null || loop_depth == 0) {
            return false
        }
        if (self->is_loop_variable(v)) {
            return false
        }
        for (entry in var_stack) {
            if (entry.v == v) {
                return entry.depth < loop_depth
            }
        }
        // not on stack — function argument or global — outside any loop
        return true
    }

    // --- expression helpers ---

    def is_character_at_zero(expr : smart_ptr<ExprCall>) : bool {
        if (length(expr.arguments) >= 2) {
            var idx = get_ptr(expr.arguments[1])
            if (idx is ExprConstInt && (idx as ExprConstInt).value == 0) {
                return true
            }
        }
        return false
    }

    def is_array_func(expr : smart_ptr<ExprCall>; fname : string) : bool {
        // Builtin generics (push, reserve, etc.) have fromGeneric set.
        // Verify first argument is an array type.
        if (expr.func.fromGeneric == null || expr.func.fromGeneric.name != fname) {
            return false
        }
        if (length(expr.arguments) < 1) {
            return false
        }
        let arg = get_ptr(expr.arguments[0])
        return arg._type != null && arg._type.baseType == Type.tArray
    }

    def find_expr_path(var expr : Expression?; var path : string&) : Variable? {
        //! Walks expression chains (field access, deref, index, etc.) to find the
        //! root variable, building a path string to distinguish foo.a from foo.b.
        //! Returns null if the chain can't be resolved (call results, Op3, etc.).
        return null if (expr == null)
        if (expr is ExprVar) {
            var evar = expr as ExprVar
            return null if (evar.variable == null)
            return get_ptr(evar.variable)
        }
        if (expr is ExprRef2Value) {
            return self->find_expr_path(get_ptr((expr as ExprRef2Value).subexpr), path)
        }
        if (expr is ExprField) {
            var field = expr as ExprField
            path := ".{field.name}{path}"
            return self->find_expr_path(get_ptr(field.value), path)
        }
        if (expr is ExprSafeField) {
            var field = expr as ExprSafeField
            path := ".{field.name}{path}"
            return self->find_expr_path(get_ptr(field.value), path)
        }
        if (expr is ExprAt) {
            var at = expr as ExprAt
            path := "[*]{path}"
            return self->find_expr_path(get_ptr(at.subexpr), path)
        }
        if (expr is ExprSafeAt) {
            var at = expr as ExprSafeAt
            path := "[*]{path}"
            return self->find_expr_path(get_ptr(at.subexpr), path)
        }
        if (expr is ExprSwizzle) {
            var swiz = expr as ExprSwizzle
            return self->find_expr_path(get_ptr(swiz.value), path)
        }
        if (expr is ExprCast) {
            var ca = expr as ExprCast
            return self->find_expr_path(get_ptr(ca.subexpr), path)
        }
        if (expr is ExprRef2Ptr) {
            var rr = expr as ExprRef2Ptr
            return self->find_expr_path(get_ptr(rr.subexpr), path)
        }
        if (expr is ExprPtr2Ref) {
            var rr = expr as ExprPtr2Ref
            return self->find_expr_path(get_ptr(rr.subexpr), path)
        }
        // Bail on everything else (Op3, NullCoalescing, calls, etc.)
        return null
    }

    def make_path_key(v : Variable?; path : string) : string {
        return "{intptr(v)}{path}"
    }

    def find_string_var_from_expr(expr : Expression?) : Variable? {
        return null if (expr == null)
        // Unwrap ExprRef2Value (compiler inserts these for value-type reads)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = get_ptr((inner as ExprRef2Value).subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        var evar = inner as ExprVar
        return null if (evar.variable == null || evar.variable._type == null)
        if (evar.variable._type.baseType == Type.tString) {
            return get_ptr(evar.variable)
        }
        return null
    }

    // --- PERF007/PERF008 helpers ---

    def is_string_of_das_string(expr : Expression?) : bool {
        //! Returns true if expr is a string(X) call where X is das_string.
        return false if (expr == null)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = get_ptr((inner as ExprRef2Value).subexpr)
        }
        return false if (inner == null || !(inner is ExprCall))
        var call = inner as ExprCall
        if (length(call.arguments) < 1) {
            return false
        }
        // After inference, string() calls may have FakeContext/FakeLineInfo extra args.
        // Check the function name and first argument type.
        if (call.name != "string") {
            return false
        }
        var arg = get_ptr(call.arguments[0])
        if (arg._type == null || arg._type.baseType != Type.tHandle) {
            return false
        }
        return arg._type.annotation != null && arg._type.annotation.name == "das_string"
    }

    def is_get_ptr_of_smart_ptr(expr : Expression?) : bool {
        //! Returns true if expr is a get_ptr(X) call where X is a smart_ptr.
        return false if (expr == null || !(expr is ExprCall))
        var call = expr as ExprCall
        if (call.func == null || call.func.fromGeneric == null) {
            return false
        }
        return call.func.fromGeneric.name == "get_ptr"
    }

    // --- function tracking ---

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = get_ptr(fn)
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        current_function = null
        return <- fn
    }

    // --- scope tracking ---

    def override preVisitExprBlock(blk : smart_ptr<ExprBlock>) : void {
        if (blk.blockFlags.isClosure) {
            in_closure++
            string_builder_save_stack |> push(string_builder_target_var)
            string_builder_target_var = null
        }
        scope_stack |> push(length(var_stack))
    }

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            in_closure--
            if (length(string_builder_save_stack) > 0) {
                string_builder_target_var = string_builder_save_stack |> back()
                string_builder_save_stack |> pop()
            }
        }
        if (length(scope_stack) > 0) {
            var_stack |> resize(scope_stack |> back())
            scope_stack |> pop()
        }
        return <- blk
    }

    // --- variable declaration tracking ---

    def override preVisitExprLetVariable(expr : smart_ptr<ExprLet>; var v : VariablePtr; last : bool) : void {
        if (in_closure == 0) {
            var_stack |> push(VarStackEntry(v = get_ptr(v), depth = loop_depth, is_iter = false))
        }
        // PERF009: track last move-initialized variable (only explicit `var x <- expr`)
        if (last && v.flags.init_via_move && v.init != null) {
            var dominated_by_clone = false
            // Exclude compiler-lowered `:=` (clone_to_move) on non-copyable types
            var ini = get_ptr(v.init)
            if (ini is ExprCall) {
                let ecall = ini as ExprCall
                if (ecall.func != null && ecall.func.fromGeneric != null) {
                    dominated_by_clone = ecall.func.fromGeneric.name == "clone_to_move"
                }
            }
            if (!dominated_by_clone) {
                pending_move_var = get_ptr(v)
                pending_move_at = expr.at
            }
        }
    }

    // --- PERF009: redundant move-init variable before return ---

    def override preVisitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) : void {
        if (pending_move_var != null) {
            var e = get_ptr(expr)
            if (e is ExprReturn) {
                var ret = e as ExprReturn
                if (ret.returnFlags.moveSemantics && ret.subexpr != null) {
                    var sub = get_ptr(ret.subexpr)
                    if (sub is ExprRef2Value) {
                        sub = get_ptr((sub as ExprRef2Value).subexpr)
                    }
                    if (sub is ExprVar && get_ptr((sub as ExprVar).variable) == pending_move_var) {
                        self->perf_warning("PERF009: redundant move into variable immediately returned; use return <- expr directly", pending_move_at)
                    }
                }
            }
            pending_move_var = null
        }
    }

    // --- loop tracking ---

    def override preVisitExprFor(expr : smart_ptr<ExprFor>) : void {
        if (in_closure == 0) {
            scope_stack |> push(length(var_stack))
            loop_depth++
            if_depth_stack |> push(0)
            current_for_known_length = true
            loop_has_early_exit |> push(false)
        }
    }

    def is_known_length_source(src : Expression?) : bool {
        if (src == null || src._type == null) {
            return false
        }
        let bt = src._type.baseType
        if (bt == Type.tArray || bt == Type.tString) {
            return true
        }
        if (bt == Type.tRange || bt == Type.tRange64 || bt == Type.tURange || bt == Type.tURange64) {
            return true
        }
        // fixed arrays have dim > 0
        if (src._type.dim |> length > 0) {
            return true
        }
        return false
    }

    def override preVisitExprForSource(expr : smart_ptr<ExprFor>; src : ExpressionPtr; last : bool) : void {
        if (in_closure == 0 && !self->is_known_length_source(get_ptr(src))) {
            current_for_known_length = false
        }
    }

    def override preVisitExprForBody(expr : smart_ptr<ExprFor>) : void {
        if (in_closure == 0 && current_for_known_length) {
            known_length_loop_depth++
        }
    }

    def override preVisitExprForVariable(expr : smart_ptr<ExprFor>; var v : VariablePtr; last : bool) : void {
        if (in_closure == 0) {
            var_stack |> push(VarStackEntry(v = get_ptr(v), depth = loop_depth - 1, is_iter = true))
        }
    }

    def override visitExprFor(var expr : smart_ptr<ExprFor>) : ExpressionPtr {
        if (in_closure == 0) {
            loop_depth--
            if (current_for_known_length) {
                known_length_loop_depth--
            }
            if (length(scope_stack) > 0) {
                var_stack |> resize(scope_stack |> back())
                scope_stack |> pop()
            }
            if (length(if_depth_stack) > 0) {
                if_depth_stack |> pop()
            }
            if (length(loop_has_early_exit) > 0) {
                loop_has_early_exit |> pop()
            }
        }
        return <- expr
    }

    def override preVisitExprWhile(expr : smart_ptr<ExprWhile>) : void {
        if (in_closure == 0) {
            scope_stack |> push(length(var_stack))
            loop_depth++
            if_depth_stack |> push(0)
            loop_has_early_exit |> push(false)
            in_while_cond = true
        }
    }

    def override preVisitExprWhileBody(expr : smart_ptr<ExprWhile>; body : ExpressionPtr) : void {
        in_while_cond = false
    }

    def override visitExprWhile(var expr : smart_ptr<ExprWhile>) : ExpressionPtr {
        if (in_closure == 0) {
            loop_depth--
            if (length(scope_stack) > 0) {
                var_stack |> resize(scope_stack |> back())
                scope_stack |> pop()
            }
            if (length(if_depth_stack) > 0) {
                if_depth_stack |> pop()
            }
            if (length(loop_has_early_exit) > 0) {
                loop_has_early_exit |> pop()
            }
        }
        in_while_cond = false
        return <- expr
    }

    // --- break/continue tracking (for PERF006 early-exit suppression) ---

    def override preVisitExprBreak(expr : smart_ptr<ExprBreak>) : void {
        if (in_closure == 0 && length(loop_has_early_exit) > 0) {
            loop_has_early_exit[length(loop_has_early_exit) - 1] = true
        }
    }

    def override preVisitExprContinue(expr : smart_ptr<ExprContinue>) : void {
        if (in_closure == 0 && length(loop_has_early_exit) > 0) {
            loop_has_early_exit[length(loop_has_early_exit) - 1] = true
        }
    }

    // --- if-depth tracking (for PERF006 conditional suppression) ---

    def override preVisitExprIfThenElse(ifte : smart_ptr<ExprIfThenElse>) : void {
        if (length(if_depth_stack) > 0) {
            if_depth_stack[length(if_depth_stack) - 1]++
        }
    }

    def override visitExprIfThenElse(var ifte : smart_ptr<ExprIfThenElse>) : ExpressionPtr {
        if (length(if_depth_stack) > 0) {
            if_depth_stack[length(if_depth_stack) - 1]--
        }
        return <- ifte
    }

    // --- PERF001: string += in loop ---
    // --- PERF007: unnecessary string(das_string) in comparison ---
    // --- PERF008: unnecessary get_ptr() for is/as ---

    def override preVisitExprOp2(expr : smart_ptr<ExprOp2>) : void {
        if (in_closure > 0) {
            return
        }
        if (loop_depth > 0 && expr.op == "+=") {
            let v = self->find_string_var_from_expr(get_ptr(expr.left))
            if (v != null && self->is_defined_outside_loop(v)) {
                self->perf_warning("PERF001: string += in loop creates O(n^2) allocations; use build_string() instead", expr.at)
            }
        }
        // PERF007: string(das_string) == or != comparison
        if (expr.op == "==" || expr.op == "!=") {
            let left_is = self->is_string_of_das_string(get_ptr(expr.left))
            let right_is = self->is_string_of_das_string(get_ptr(expr.right))
            if (left_is || right_is) {
                self->perf_warning("PERF007: string(das_string) in comparison is unnecessary; das_string supports == and != directly", expr.at)
            }
        }
        // PERF008: get_ptr(x).__rtti == "Type" is expanded from `get_ptr(x) is Type`
        if (expr.op == "==") {
            var left = get_ptr(expr.left)
            // Unwrap ExprRef2Value — compiler inserts these for value-type reads
            if (left is ExprRef2Value) {
                left = get_ptr((left as ExprRef2Value).subexpr)
            }
            if (left is ExprField) {
                var field = left as ExprField
                if (field.name == "__rtti" && self->is_get_ptr_of_smart_ptr(get_ptr(field.value))) {
                    self->perf_warning("PERF008: get_ptr() is unnecessary for is/as; smart_ptr supports type checks directly", expr.at)
                }
            }
        }
    }

    // --- PERF002 + PERF003: character_at (counter-based) ---

    def override preVisitExprCall(var expr : smart_ptr<ExprCall>) : void {
        if (in_closure > 0) {
            return
        }
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = get_ptr(expr)
            // PERF002: start tracking — any loop var inside this call triggers warning
            if (loop_depth > 0) {
                in_character_at_call++
                current_character_at = ecall
            }
        }
        // PERF005: length(string) in while condition — start tracking
        if (in_while_cond && expr.func.name == "length" && expr.func._module.name == "strings") {
            in_length_while_call++
        }
    }

    def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr {
        // PERF003: character_at anywhere — fires even inside closures
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = get_ptr(expr)
            if (in_closure == 0 && loop_depth > 0 && in_character_at_call > 0) {
                in_character_at_call--
                current_character_at = null
            }
            // deferred so PERF002 can claim it first
            if (!reported_character_at |> has_value(ecall)) {
                if (self->is_character_at_zero(expr)) {
                    self->perf_warning("PERF003: character_at(s, 0) can be replaced with first_character(s)", expr.at)
                } else {
                    self->perf_warning("PERF003: character_at is O(n) due to strlen; consider peek_data() for hot paths", expr.at)
                }
            }
        }
        // PERF006: track reserve() calls and detect push/emplace without reserve
        if (self->is_array_func(expr, "reserve")) {
            var path = ""
            let rv = self->find_expr_path(get_ptr(expr.arguments[0]), path)
            if (rv != null) {
                let key = self->make_path_key(rv, path)
                if (!reserved_paths |> has_value(key)) {
                    reserved_paths |> push(key)
                }
            }
        }
        let in_conditional = length(if_depth_stack) > 0 && if_depth_stack[length(if_depth_stack) - 1] > 0
        let has_early_exit = length(loop_has_early_exit) > 0 && loop_has_early_exit[length(loop_has_early_exit) - 1]
        if (in_closure == 0 && known_length_loop_depth > 0 && !in_conditional && !has_early_exit) {
            if (self->is_array_func(expr, "push") || self->is_array_func(expr, "push_clone") || self->is_array_func(expr, "emplace")) {
                var path = ""
                let rv = self->find_expr_path(get_ptr(expr.arguments[0]), path)
                if (rv != null && self->is_defined_outside_loop(rv) && !reserved_paths |> has_value(self->make_path_key(rv, path))) {
                    let fname = string(expr.func.fromGeneric.name)
                    self->perf_warning("PERF006: {fname} in loop without prior reserve() may cause repeated reallocations; consider reserve() before the loop", expr.at)
                }
            }
        }
        return <- expr if (in_closure > 0)
        if (expr.func.name == "length" && expr.func._module.name == "strings") {
            if (in_length_while_call > 0) {
                in_length_while_call--
            }
        }
        return <- expr
    }

    // --- PERF004: string builder reassignment in loop (counter-based) ---

    def override preVisitExprCopy(expr : smart_ptr<ExprCopy>) : void {
        if (in_closure > 0 || loop_depth == 0) {
            return
        }
        var v = self->find_string_var_from_expr(get_ptr(expr.left))
        if (v != null && self->is_defined_outside_loop(v) && expr.right is ExprStringBuilder) {
            string_builder_target_var = v
        }
    }

    def override visitExprCopy(var expr : smart_ptr<ExprCopy>) : ExpressionPtr {
        string_builder_target_var = null
        return <- expr
    }

    def override preVisitExprMove(expr : smart_ptr<ExprMove>) : void {
        if (in_closure > 0 || loop_depth == 0) {
            return
        }
        var v = self->find_string_var_from_expr(get_ptr(expr.left))
        if (v != null && self->is_defined_outside_loop(v) && expr.right is ExprStringBuilder) {
            string_builder_target_var = v
        }
    }

    def override visitExprMove(var expr : smart_ptr<ExprMove>) : ExpressionPtr {
        string_builder_target_var = null
        return <- expr
    }

    def override preVisitExprStringBuilder(expr : smart_ptr<ExprStringBuilder>) : void {
        if (string_builder_target_var != null) {
            in_string_builder_check++
        }
    }

    def override visitExprStringBuilder(var expr : smart_ptr<ExprStringBuilder>) : ExpressionPtr {
        if (in_string_builder_check > 0) {
            in_string_builder_check--
        }
        return <- expr
    }

    // --- Central ExprVar handler (PERF002, PERF004, PERF005) ---

    def override preVisitExprVar(expr : smart_ptr<ExprVar>) : void {
        if (in_closure > 0) {
            return
        }
        let v = get_ptr(expr.variable)
        // PERF002: loop variable inside character_at call
        if (in_character_at_call > 0 && self->is_loop_variable(v)) {
            if (current_character_at != null && !reported_character_at |> has_value(current_character_at)) {
                self->perf_warning("PERF002: character_at(s, i) in loop is O(n) per call (strlen + bounds check each time); use peek_data(s) to access characters as array<uint8>", expr.at)
                reported_character_at |> push(current_character_at)
            }
        }
        // PERF004: target var inside string builder
        if (in_string_builder_check > 0 && string_builder_target_var != null && v == string_builder_target_var) {
            self->perf_warning("PERF004: rebuilding string via interpolation in loop creates O(n^2) allocations; use build_string() instead", expr.at)
        }
        // PERF005: string var inside length() in while condition
        if (in_length_while_call > 0 && v != null) {
            if (expr.variable._type != null && expr.variable._type.baseType == Type.tString) {
                if (!expr.variable.access_flags.access_ref) {
                    self->perf_warning("PERF005: length(string) in while condition recomputes strlen each iteration; cache in a local variable", expr.at)
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

def public perf_lint(prog : ProgramPtr; compile_time_errors : bool) : int {
    //! Runs the performance lint visitor on the compiled program.
    //! Returns the number of warnings found.
    var astVisitor = new PerfLintVisitor(compile_time_errors = compile_time_errors)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(prog, astVisitorAdapter)
    let count = astVisitor.warning_count
    unsafe { delete astVisitor; }
    return count
}

def public perf_lint_collect(prog : ProgramPtr; var warnings : array<string>) : int {
    //! Runs the performance lint visitor and collects warnings as strings.
    //! Returns the number of warnings found.
    var astVisitor = new PerfLintVisitor(compile_time_errors = false, collect_warnings = true)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(prog, astVisitorAdapter)
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
class PerfLintMacro : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        perf_lint(prog, true)
        return true
    }
}
