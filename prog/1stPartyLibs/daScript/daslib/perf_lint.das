options gen2
options indenting = 4
options strict_smart_pointers = true

module perf_lint shared private

//! Performance lint module.
//!
//! Detects common performance anti-patterns in daslang code at compile time.
//! When this module is required, a lint pass runs after compilation and reports
//! warnings as ``CompilationError::runtime_macro_performance`` (error code 31208).
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
//!   PERF010 — unnecessary get_ptr() for null comparison
//!   PERF011 — unnecessary get_ptr() for field access
//!   PERF012 — string(das_string) passed to strings module function
//!   PERF013 — a += 1 / a -= 1 — use a++ / a--
//!   PERF014 — closed-interval char-class range — see is_alpha / is_alnum / is_number etc.
//!   PERF015 — ternary min/max — use min(a, b) / max(a, b)
//!   PERF016 — ternary abs — use abs(x)
//!   PERF017 — length(s) == 0 / != 0 — use empty(s) / !empty(s)
//!   PERF018 — for (i in range(length(arr))) where i only indexes arr — use 'for (c in arr)'; also urange/range64/urange64 + long_length
//!   PERF029 — for (i in range(length(X))) subscripting sibling arrays in lockstep — zip: for (a, b in X, Y); same range/length variants as PERF018
//!   PERF030 — move-assign onto a live heap-carrying variable drops old contents — delete first or force_inscope_pod
//!   PERF031 — slice/chop on a loop-invariant string inside a loop — each call strlens the whole source (O(n²)); slice the peek_data view instead
//!   PERF019 — int(T.a) | int(T.b) on bitfield-or-enum-with-operator-| — collapse to int(T.a | T.b)
//!   PERF020 — T(x) where x is already T (workhorse type) — drop the redundant cast (non-generic bodies only)
//!   PERF021 — cond ? T(a) : T(b) on workhorse cast T — hoist to T(cond ? a : b)
//!   PERF022 — for (s in A) { B |> push(s) } / push_clone — use 'B |> push_from(A)' (bulk reserve+copy)
//!   PERF023 — var X = clone_expression(E); ... $e(X) ... — qmacro splice already clones; drop the pre-clone
//!   PERF024 — var X = clone_*(E); func(X) where func is [clone(...)]-annotated — callee clones; drop the pre-clone
//!   PERF025 — string(...) inside string interpolation is redundant
//!   PERF026 — heap traffic on a hot path ([hot_path] / [no_alloc])
//!   PERF027 — environment lookup on a hot path ([hot_path] / [no_env])
//!   PERF028 — console / file I/O on a hot path ([hot_path] / [no_io])
//!
//! PERF026-028 are annotation-gated: nothing is checked until a function declares a contract.
//! [cold_path] prunes the walk; @scratch on a field (or on a by-ref helper parameter) declares a
//! reused buffer whose sizing is intentional. See skills/perf_lint.md.
//!
//! The five markers are registered compiler-side (src/builtin/module_builtin_runtime.cpp, beside
//! [clone]), so code under contract requires NOTHING and pays no build time. This module is the
//! checker, and is deliberately not required by the code it checks — it runs wherever lint runs.

require daslib/ast_boost
require strings
require daslib/lint_config

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------

struct Perf031Return {
    loop_depth_at_return : int
    barrier : int
}

struct VarStackEntry {
    @do_not_delete v : Variable?
    depth : int
    inline_aware_depth : int
    is_iter : bool
}

struct Perf018State {
    //! PERF018 per-for-loop candidate. Each enclosing `for` pushes one; `matched` flips true once we observe `for (i in range(length(V)))` with exactly one source and one iter var, where V is a bare Variable. We push a sentinel for non-matching loops too so visitExprFor can pop unconditionally and iter_var refs across nesting attribute to the right frame.
    @do_not_delete iter_var : Variable?
    @do_not_delete arr_var : Variable?
    qualified_count : int
    sources_seen : int
    vars_seen : int
    disqualified : bool
    matched : bool
    perf029_sibling_keys : string
    perf029_sibling_names : string
    perf029_sibling_count : int
}

struct Perf022State {
    //! PERF022 per-for-loop candidate. Detects 'for (s in A) { B |> push(s) }' (push and push_clone; all three pipe/dot/plain-call shapes compiler-fold to the same ExprCall). Pushed in preVisitExprFor in lockstep with PERF018.
    @do_not_delete iter_var : Variable?
    callee_id : Perf022Callee
    sources_seen : int
    vars_seen : int
    disqualified : bool
    matched : bool
}

struct Perf024Candidate {
    //! PERF024 per-candidate state. One entry per `var X = clone_*(E)` decl seen in the current function (any of clone_expression / clone_type / clone_function / clone_variable / clone_structure). `safe_uses` counts ExprVar refs to X passed directly as an arg at a [clone(...)]-annotated position; `disqualified` is set on any other use. Reports the same way as PERF023: !disqualified && safe_uses >= 1.
    @do_not_delete v : Variable?
    safe_uses : int
    disqualified : bool
}

struct Perf023Candidate {
    //! PERF023 per-candidate state. One entry per `var X = clone_expression(E)` decl seen in the current function. `safe_uses` counts ExprVar refs to X whose ancestor chain (within this function) includes an `add_ptr_ref(...)` call — the splice-wrapper installed by `apply_qrules` for every `$e(...)` tag. `disqualified` is set on any other use (assignment, call arg passed to a non-splice consumer, etc.); the pre-clone is then load-bearing. The `var` carries the decl `at` directly (v.at), so the warning location is derived without storing LineInfo in the array.
    @do_not_delete v : Variable?
    safe_uses : int
    disqualified : bool
}

enum private Perf025Kind {
    not_a_cast
    cast_is_noop
    cast_changes_radix
}

enum private BlockTailExit {
    none
    returns
    breaks
}

enum private Perf022Callee {
    neither
    push
    push_clone
}

bitfield private HotBits {
    perf026_alloc
    perf027_env
    perf028_io
}

struct private HotSink {
    bit : HotBits
    what : string
    @do_not_delete site : ExpressionPtr
}

struct private HotEdge {
    @do_not_delete fn : Function?
    @do_not_delete caller_site : ExpressionPtr
}

struct private HotCache {
    sinks_span_by_fn : table<uint64; int2>
    edges_span_by_fn : table<uint64; int2>
    sinks_all : array<HotSink>
    edges_all : array<HotEdge>
}

struct private HotFrame {
    @do_not_delete fn : Function?
    parent : int
    @do_not_delete site : ExpressionPtr
    @do_not_delete nearest_root_file_site : ExpressionPtr
}

def private hot_bits_of(func : Function?) : HotBits {
    var bits : HotBits
    for (a in func.annotations) {
        if (a.annotation.name == "hot_path") {
            bits = HotBits.perf026_alloc | HotBits.perf027_env | HotBits.perf028_io
        } elif (a.annotation.name == "no_alloc") {
            bits.perf026_alloc = true
        } elif (a.annotation.name == "no_env") {
            bits.perf027_env = true
        } elif (a.annotation.name == "no_io") {
            bits.perf028_io = true
        }
    }
    return bits
}

def private is_cold_path(func : Function?) : bool {
    for (a in func.annotations) {
        if (a.annotation.name == "cold_path") return true
    }
    return false
}

def private is_alloc_builtin(n : string) : bool {
    if (starts_with(n, "__builtin_array_") || starts_with(n, "__builtin_table_")) {
        return !(ends_with(n, "_lock") || ends_with(n, "_lock_mutable")
                 || ends_with(n, "_unlock") || ends_with(n, "_unlock_mutable")
                 || n == "__builtin_table_find" || n == "__builtin_table_key_exists"
                 || n == "__builtin_table_get_key" || n == "__builtin_table_keys"
                 || n == "__builtin_table_values")
    }
    return false
}

def private is_container_sizing(n : string) : bool {
    return (n == "resize" || n == "resize_no_init" || n == "reserve" || n == "clear"
            || n == "push" || n == "push_clone" || n == "emplace" || n == "push_from"
            || n == "push_clone_from" || n == "insert" || n == "erase" || n == "pop"
            || n == "ensure_capacity" || n == "scratch_resize" || n == "scratch_reserve"
            || n == "scratch_ensure_capacity")
}

def private perf030_needs_release(td : TypeDeclPtr) : bool {
    if (td == null) return false
    let bt = td.baseType
    if (bt == Type.tArray || bt == Type.tTable || bt == Type.tLambda) return true
    if (bt == Type.tFixedArray) return perf030_needs_release(td.firstType)
    if (bt == Type.tStructure && td.structType != null) {
        for (fld in td.structType.fields) {
            if (perf030_needs_release(fld._type)) return true
        }
        return false
    }
    if (bt == Type.tTuple || bt == Type.tVariant) {
        for (aft in td.argTypes) {
            if (perf030_needs_release(aft)) return true
        }
    }
    return false
}

def private has_annotation(annotations; name : string) : bool {
    for (a in annotations) {
        if (a.name == name) return true
    }
    return false
}

def private is_annotated_dest(var expr : Expression?; name : string) : bool {
    return false if (expr == null)
    if (expr is ExprField) {
        var f = expr as ExprField
        var fd = f.field
        if (fd != null && has_annotation(fd.annotation, name)) return true
        return is_annotated_dest(f.value, name)
    }
    if (expr is ExprVar) {
        var v = (expr as ExprVar).variable
        if (v != null) {
            if (has_annotation(v.annotation, name)) return true
            if (v._type != null && v._type.flags.ref && v.init != null) {
                return is_annotated_dest(v.init, name)
            }
        }
        return false
    }
    if (expr is ExprRef2Value) return is_annotated_dest((expr as ExprRef2Value.subexpr), name)
    if (expr is ExprAt) return is_annotated_dest((expr as ExprAt.subexpr), name)
    if (expr is ExprSafeAt) return is_annotated_dest((expr as ExprSafeAt.subexpr), name)
    if (expr is ExprCast) return is_annotated_dest((expr as ExprCast.subexpr), name)
    return false
}

def private is_scratch_dest(var expr : Expression?) : bool {
    return is_annotated_dest(expr, "scratch")
}

def private takes_scratch_arg(fn : Function?) : bool {
    return false if (fn == null)
    for (arg in fn.arguments) {
        if (has_annotation(arg.annotation, "scratch")) return true
    }
    return false
}

def private is_explicit_capacity_call(n : string) : bool {
    return (n == "reserve" || n == "ensure_capacity"
            || n == "scratch_reserve" || n == "scratch_ensure_capacity" || n == "scratch_resize")
}

def private is_env_builtin(n : string) : bool {
    return n == "get_env_variable" || n == "has_env_variable" || n == "set_env_variable"
}

def private is_io_builtin(n : string) : bool {
    return (n == "print" || n == "to_log" || n == "fprint" || n == "fopen" || n == "fclose"
            || n == "fread" || n == "fwrite" || n == "fflush" || n == "popen" || n == "popen_argv")
}

class HotBodyScan : AstVisitor {
    sinks : array<HotSink>
    edges : array<HotEdge>
    panic_depth : int = 0

    gen_depth : int = 0

    def override preVisitExpression(var expr : ExpressionPtr) : void {
        if (expr.genFlags.generated) {
            gen_depth++
        }
    }

    def override visitExpression(var expr : ExpressionPtr) : ExpressionPtr {
        if (expr.genFlags.generated) {
            gen_depth--
        }
        return expr
    }

    def add_sink(bit : HotBits; what : string; var site : ExpressionPtr) {
        return if (panic_depth > 0 || gen_depth > 0)
        sinks |> push(HotSink(bit = bit, what = what, site = site))
    }

    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (expr.func != null && expr.func.name == "panic") {
            panic_depth--
        }
        return expr
    }

    def override preVisitExprCall(var expr : ExprCall?) : void {
        if (expr.func == null) return
        let n = string(expr.func.name)
        if (n == "panic") {
            panic_depth++
            return
        }
        return if (panic_depth > 0)
        let origin = string(expr.func.fromGeneric != null ? expr.func.fromGeneric.name : expr.func.name)
        if (!empty(expr.arguments)
            && ((is_container_sizing(origin) && is_scratch_dest(expr.arguments[0]))
                || takes_scratch_arg(expr.func))) {
            return
        }
        if (is_env_builtin(n)) {
            add_sink(HotBits.perf027_env, "{n}()", expr)
        } elif (is_io_builtin(n)) {
            add_sink(HotBits.perf028_io, "{n}()", expr)
        } elif (is_alloc_builtin(n)) {
            add_sink(HotBits.perf026_alloc, "{n}()", expr)
        } elif (expr.func.flags.builtIn && expr.func.result != null
                && expr.func.result.baseType == Type.tString) {
            add_sink(HotBits.perf026_alloc, "{n}() returns a newly allocated string", expr)
        }
        edges |> push(HotEdge(fn = expr.func, caller_site = expr))
    }

    def override preVisitExprNew(var expr : ExprNew?) : void {
        add_sink(HotBits.perf026_alloc, "new", expr)
    }

    def override preVisitExprAscend(var expr : ExprAscend?) : void {
        add_sink(HotBits.perf026_alloc, "new", expr)
    }

    def override preVisitExprDelete(var expr : ExprDelete?) : void {
        add_sink(HotBits.perf026_alloc, "delete", expr)
    }

    def override preVisitExprStringBuilder(var expr : ExprStringBuilder?) : void {
        add_sink(HotBits.perf026_alloc, "string interpolation", expr)
    }

    def override preVisitExprAt(var expr : ExprAt?) : void {
        if (expr.subexpr == null || expr.subexpr._type == null) return
        if (expr.subexpr._type.baseType == Type.tTable && !is_scratch_dest(expr.subexpr)) {
            add_sink(HotBits.perf026_alloc, "table index (inserts a default entry when the key is missing)", expr)
        }
    }

    def override preVisitExprMakeBlock(var expr : ExprMakeBlock?) : void {
        if (expr.mmFlags.isLambda) {
            add_sink(HotBits.perf026_alloc, "lambda capture frame", expr)
        }
    }
}

class PerfLintVisitor : AstVisitor {
    compile_time_errors : bool
    perf032_reserved : table<string>
    loop_depth : int = 0
    in_closure : int = 0
    var_stack : array<VarStackEntry>
    scope_stack : array<int>
    @do_not_delete inline_blocks : array<ExprBlock?>
    inline_block_entry_depth : array<int>
    in_deferred : int = 0
    inline_aware_loop_depth : int = 0
    block_tail_exit : array<BlockTailExit>
    return_frames : array<Perf031Return>
    in_while_cond : bool = false
    in_character_at_call : int = 0
    @do_not_delete current_character_at : ExprCall?
    in_length_while_call : int = 0
    in_string_builder_check : int = 0
    @do_not_delete string_builder_target_var : Variable?
    @do_not_delete string_builder_save_stack : array<Variable?>
    @do_not_delete reported_character_at : array<ExprCall?>
    reserved_paths : array<string>
    if_depth_stack : array<int>
    @do_not_delete current_function : Function?
    in_template : bool = false
    @do_not_delete hot_roots : array<Function?>
    known_length_loop_depth : int = 0
    current_for_known_length : bool = true
    known_length_stack : array<bool>
    loop_has_early_exit : array<bool>
    @do_not_delete perf030_released : array<Variable?>
    @do_not_delete pending_move_var : Variable?
    @safe_when_uninitialized pending_move_at : LineInfo
    pending_move_is_clone : bool = false
    perf018_stack : array<Perf018State>
    @do_not_delete perf018_for_exprs : array<ExprFor?>
    in_qualifying_idx : int = 0
    at_qualifying_pushes : array<int>
    perf022_stack : array<Perf022State>
    @do_not_delete perf022_for_exprs : array<ExprFor?>
    perf023_candidates : array<Perf023Candidate>
    perf023_splice_depth : int = 0
    perf024_candidates : array<Perf024Candidate>
    perf024_skip_iptr : uint64 = 0ul
    warning_count : int = 0
    collect_warnings : bool = false
    warnings : array<string>
    issues : array<LintIssue>
    reported_locations : table<uint64>
    perf019_enum_or_probed : array<uint64>
    perf019_enum_or_yes : array<uint64>
    def report_key(at : LineInfo; code : string) : uint64 {
        //! fileInfo is part of the key — PERF026-028 report sinks in other files, where line|column alone collides.
        let pos = uint64(at.line) | (uint64(at.column) << uint64(20))
        return (pos ^ (intptr(at.fileInfo) * 1099511628211ul)) ^ hash(code)
    }

    disabled_codes : table<string>
    enabled_codes : table<string>

    def is_suppressed_code(code : string) : bool {
        return (key_exists(disabled_codes, code)
                || (!empty(enabled_codes) && !key_exists(enabled_codes, code)))
    }

    def is_suppressed(text : string; at : LineInfo) : bool {
        let code = extract_lint_code(text)
        if (!empty(code)) {
            return true if (key_exists(disabled_codes, code)
                            || (!empty(enabled_codes) && !key_exists(enabled_codes, code)))
        }
        return is_lint_suppressed(at, code)
    }

    def perf_warning(text : string; at : LineInfo) : void {
        if (in_template
            || !is_user_authored_body(current_function)
            || is_suppressed(text, at)) return
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
            compiling_program() |> macro_performance_warning(at, msg)
        } elif (collect_warnings) {
            warnings |> push("performance warning: {msg} at {describe(at)}")
            issues |> push(make_lint_issue(msg, at))
        } else {
            to_log(LOG_WARNING, "performance warning: {msg} at {describe(at)}\n")
        }
    }

    def is_loop_variable(v : Variable?) : bool {
        if (v == null) return false
        for (entry in var_stack) {
            if (entry.v == v && entry.is_iter) return true
        }
        return false
    }

    def outside_loop(v : Variable?; depth : int; use31 : bool) : bool {
        if (v == null || depth == 0 || is_loop_variable(v)) return false
        for (entry in var_stack) {
            if (entry.v == v) return (use31 ? entry.inline_aware_depth : entry.depth) < depth
        }
        return true
    }

    def is_defined_outside_loop(v : Variable?) : bool => outside_loop(v, loop_depth, false)

    def is_defined_outside_inline_aware_loop(v : Variable?) : bool => outside_loop(v, inline_aware_loop_depth, true)

    def is_character_at_zero(expr : ExprCall?) : bool {
        if (length(expr.arguments) >= 2) {
            var idx = expr.arguments[1]
            if (idx is ExprConstInt && (idx as ExprConstInt).value == 0) return true
        }
        return false
    }

    def is_array_func(expr : ExprCall?; fname : string) : bool {
        if (expr.func == null || expr.func.fromGeneric == null
            || expr.func.fromGeneric.name != fname || empty(expr.arguments)) return false
        let arg = expr.arguments[0]
        return arg._type != null && arg._type.baseType == Type.tArray
    }

    def find_expr_path(var expr : Expression?; var path : string&) : Variable? {
        //! Walks expression chains (field access, deref, index, etc.) to find the root variable, building a path string to distinguish foo.a from foo.b.
        return null if (expr == null)
        if (expr is ExprVar) {
            var evar = expr as ExprVar
            return null if (evar.variable == null)
            return evar.variable
        }
        if (expr is ExprRef2Value) return find_expr_path((expr as ExprRef2Value.subexpr), path)
        if (expr is ExprField) {
            var field = expr as ExprField
            path = ".{field.name}{path}"
            return find_expr_path(field.value, path)
        }
        if (expr is ExprSafeField) {
            var field = expr as ExprSafeField
            path = ".{field.name}{path}"
            return find_expr_path(field.value, path)
        }
        if (expr is ExprAt) {
            var at = expr as ExprAt
            path = "[*]{path}"
            return find_expr_path(at.subexpr, path)
        }
        if (expr is ExprSafeAt) {
            var at = expr as ExprSafeAt
            path = "[*]{path}"
            return find_expr_path(at.subexpr, path)
        }
        var through = path_passthrough_child(expr)
        if (through != null) {
            return find_expr_path(through, path)
        }
        return null
    }

    def path_passthrough_child(expr : Expression?) : Expression? {
        if (expr is ExprSwizzle) return (expr as ExprSwizzle).value
        if (expr is ExprCast) return (expr as ExprCast.subexpr)
        if (expr is ExprRef2Ptr) return (expr as ExprRef2Ptr.subexpr)
        if (expr is ExprPtr2Ref) return (expr as ExprPtr2Ref.subexpr)
        return null
    }

    def make_path_key(v : Variable?; path : string) : string {
        return "{intptr(v)}{path}"
    }

    def find_string_var_from_expr(expr : Expression?) : Variable? {
        return null if (expr == null)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        var evar = inner as ExprVar
        return null if (evar.variable == null || evar.variable._type == null)
        if (evar.variable._type.baseType == Type.tString) return evar.variable
        return null
    }

    def is_string_of_das_string(expr : Expression?) : bool {
        //! Returns true if expr is a string(X) call where X is das_string.
        return false if (expr == null)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return false if (inner == null || !(inner is ExprCall))
        var call = inner as ExprCall
        if (empty(call.arguments) || call.name != "string") return false
        var arg = call.arguments[0]
        if (arg._type == null || arg._type.baseType != Type.tHandle) return false
        return arg._type.annotation != null && arg._type.annotation.name == "das_string"
    }

    def is_get_ptr_of_smart_ptr(expr : Expression?) : bool {
        //! Returns true if expr is a X call where X is a smart_ptr.
        return false if (expr == null || !(expr is ExprCall))
        var call = expr as ExprCall
        if (call.func == null || call.func.fromGeneric == null) return false
        return call.func.fromGeneric.name == "get_ptr"
    }

    def is_get_ptr_vs_null(maybe_get_ptr : Expression?; maybe_null : Expression?) : bool {
        //! Returns true if maybe_get_ptr is get_ptr() and maybe_null is null.
        if (!is_get_ptr_of_smart_ptr(maybe_get_ptr) || maybe_null == null) return false
        if (maybe_null is ExprConstPtr) {
            var cptr = maybe_null as ExprConstPtr
            return cptr.value == null
        }
        return false
    }

    def is_const_zero(expr : Expression? const) : bool {
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt).value == 0
        if (expr is ExprConstUInt) return (expr as ExprConstUInt).value == 0u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64).value == 0l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64).value == 0ul
        if (expr is ExprConstFloat) return (expr as ExprConstFloat).value == 0.0f
        if (expr is ExprConstDouble) return (expr as ExprConstDouble).value == 0.0lf
        return false
    }

    def is_const_one(expr : Expression? const) : bool {
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt).value == 1
        if (expr is ExprConstUInt) return (expr as ExprConstUInt).value == 1u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64).value == 1l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64).value == 1ul
        if (expr is ExprConstFloat) return (expr as ExprConstFloat).value == 1.0f
        if (expr is ExprConstDouble) return (expr as ExprConstDouble).value == 1.0lf
        return false
    }

    def is_const_neg_one(expr : Expression? const) : bool {
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt).value == -1
        if (expr is ExprConstInt64) return (expr as ExprConstInt64).value == -1l
        if (expr is ExprConstFloat) return (expr as ExprConstFloat).value == -1.0f
        if (expr is ExprConstDouble) return (expr as ExprConstDouble).value == -1.0lf
        return false
    }

    def expr_equal_struct(a : Expression? const; b : Expression? const; require_pure : bool = true) : bool {
        //! Structural equality via describe(). With `require_pure=true` (default), returns false when either side has side effects — protects rules that suggest collapsing duplicated subexpressions (PERF014/015/016) from silently changing evaluation count.
        if ((a == null || b == null)
            || (require_pure && (!a.flags.noSideEffects || !b.flags.noSideEffects))) return false
        return describe(a) == describe(b)
    }

    def is_workhorse_numeric_scalar(t : TypeDecl?) : bool {
        //! True if the type is one of the six numeric workhorse scalars (int, uint, int64, uint64, float, double). Vectors/bitfields/enums do NOT qualify.
        if (t == null) return false
        let bt = t.baseType
        return (bt == Type.tInt || bt == Type.tUInt
                || bt == Type.tInt64 || bt == Type.tUInt64
                || bt == Type.tFloat || bt == Type.tDouble)
    }

    def is_collection_length_call(expr : Expression? const) : bool {
        //! True for length(string) / length(das_string) / length(array) / length(table).
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        if (call.func == null || call.func.name != "length") return false
        let modname = string(call.func._module.name)
        return modname == "strings" || modname == "$" || modname == "builtin"
    }

    def unwrap_int_cast(expr : Expression? const) : Expression? const {
        //! If `expr` is `int(x)` (single argument, no fake/context args), return x.
        if (expr == null) return null
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        if (inner == null || !(inner is ExprCall)) return null
        let call = inner as ExprCall
        if (call.func == null || empty(call.arguments)) return null
        let fname = string(call.func.fromGeneric != null ? call.func.fromGeneric.name : call.func.name)
        if (fname != "int") return null
        return call.arguments[0]
    }

    def enum_has_or_operator(t : TypeDecl?) : bool {
        //! Returns true if any module in the compiling program defines `def operator |(a, b : T) : ...` for the enum represented by `t`. Result is cached on perf019_enum_or_probed / _yes by intptr(t.enumType) so repeated checks across a large file don't re-walk the function table.
        if (t == null || t.enumType == null) return false
        let key = intptr(t.enumType)
        for (idx in range(length(perf019_enum_or_probed))) {
            return perf019_enum_or_yes |> has_value(key) if (perf019_enum_or_probed[idx] == key)
        }
        var found = false
        program_for_each_module(compiling_program()) $(mod) {
            return if (found)
            for_each_function(mod, "|") $(var fn) {
                if (length(fn.arguments) == 2
                    && fn.arguments[0]._type != null && fn.arguments[1]._type != null
                    && (fn.arguments[0]._type |> is_same_type(t, RefMatters.no, ConstMatters.no, TemporaryMatters.no))
                    && (fn.arguments[1]._type |> is_same_type(t, RefMatters.no, ConstMatters.no, TemporaryMatters.no))) {
                    found = true
                }
            }
        }
        perf019_enum_or_probed |> push(key)
        if (found) {
            perf019_enum_or_yes |> push(key)
        }
        return found
    }

    def inner_type_supports_or(t : TypeDecl?) : bool {
        //! True if `T | T` resolves: bitfields always; enums only when an `operator |` overload exists. Vectors/ints/etc. don't get here — the caller restricts to int-cast inputs whose underlying type is either bitfield or enum.
        if (t == null) return false
        if (t.baseType == Type.tBitfield) return true
        if (t.baseType == Type.tEnumeration) return enum_has_or_operator(t)
        return false
    }

    def perf020_target_basetype(fname : string) : Type {
        if (fname == "int") return Type.tInt
        if (fname == "int8") return Type.tInt8
        if (fname == "int16") return Type.tInt16
        if (fname == "int64") return Type.tInt64
        if (fname == "uint") return Type.tUInt
        if (fname == "uint8") return Type.tUInt8
        if (fname == "uint16") return Type.tUInt16
        if (fname == "uint64") return Type.tUInt64
        if (fname == "float") return Type.tFloat
        if (fname == "double") return Type.tDouble
        if (fname == "string") return Type.tString
        if (fname == "bitfield") return Type.tBitfield
        if (fname == "bitfield8") return Type.tBitfield8
        if (fname == "bitfield16") return Type.tBitfield16
        if (fname == "bitfield64") return Type.tBitfield64
        return Type.none
    }

    def is_clone_init_call(expr : Expression const?) : bool {
        //! True when expr is a single-arg call to one of the AST clone primitives.
        if (expr == null || !(expr is ExprCall)) return false
        let c = expr as ExprCall
        if (length(c.arguments) != 1) return false
        let n = string(c.name)
        return (n == "clone_expression" || n == "clone_type"
            || n == "clone_function" || n == "clone_variable"
            || n == "clone_structure")
    }

    def get_clone_param_indices(callee : Function?; var out : array<int>&) : void {
        //! Fills `out` with the param-index list for any [clone(...)] annotations on callee. Caller-owned out array avoids array return ownership.
        out |> clear
        if (callee == null) return
        for (ann in callee.annotations) {
            if (ann == null || ann.annotation == null || ann.annotation.name != "clone") continue
            for (a in ann.arguments) {
                for (i in 0 .. length(callee.arguments)) {
                    if (callee.arguments[i].name == a.name) {
                        out |> push(i)
                        break
                    }
                }
            }
        }
    }

    def perf025_value_kind(arg : Expression const?) : Perf025Kind {
        //! How `"{x}"` (the builder's own conversion) compares to the `string(x)` cast PERF025 flags.
        if (arg == null || arg._type == null) return Perf025Kind.not_a_cast
        let bt = arg._type.baseType
        if (bt == Type.tInt || bt == Type.tInt8 || bt == Type.tInt16 || bt == Type.tInt64
            || bt == Type.tFloat || bt == Type.tDouble || bt == Type.tString
            || (bt == Type.tHandle && arg._type.annotation != null
                && arg._type.annotation.name == "das_string")) return Perf025Kind.cast_is_noop
        if (bt == Type.tUInt || bt == Type.tUInt8 || bt == Type.tUInt16 || bt == Type.tUInt64) return Perf025Kind.cast_changes_radix
        return Perf025Kind.not_a_cast
    }

    def perf025_string_call_value(e : Expression const?) : Expression const? {
        //! If `e` is a `string(value)` cast call, return its value argument (arguments[0]). Skips an explicit `string(x, true)` hex request — the second argument being a literal `true` means the user wants hex, which no longer matches plain interpolation for signed types. The float / double / string / das_string overloads have no hex param, so their arguments[1] (a FakeContext) is not an ExprConstBool and passes through.
        if (e == null || !(e is ExprCall)) return null
        let c = e as ExprCall
        if (c.func == null || empty(c.arguments)) return null
        let fname = string(c.func.fromGeneric != null ? c.func.fromGeneric.name : c.func.name)
        if (fname != "string") return null
        if (length(c.arguments) >= 2) {
            let a1 = c.arguments[1]
            if (a1 != null && a1 is ExprConstBool && (a1 as ExprConstBool).value) return null
        }
        return c.arguments[0]
    }

    def perf025_unwrap_element(elem : Expression const?) : Expression const? {
        //! Peels one ExprRef2Value off an interpolation element so a `string(...)` call read through a value-context wrapper is still exposed. A format spec (`"{x:fmt}"`) lowers to `_::fmt(":fmt", x)`, but `fmt` has no string-valued overload, so `string(x)` can never sit under a fmt wrapper (it would not compile) — no fmt unwrap is needed.
        var ex = elem
        if (ex != null && ex is ExprRef2Value) {
            ex = (ex as ExprRef2Value.subexpr)
        }
        return ex
    }

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = fn
        in_template = fn.moreFlags.isTemplate
        if (!in_template && uint(hot_bits_of(fn)) != 0u) {
            hot_roots |> push(fn)
        }
        perf023_candidates |> clear
        perf023_splice_depth = 0
        perf024_candidates |> clear
        perf024_skip_iptr = 0ul
        perf030_released |> clear
        perf032_reserved |> clear
        pending_move_var = null
        for (arg in fn.arguments) {
            check_perf032_declaration(arg._type, arg.annotation, arg.at)
        }
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        for (c in perf023_candidates) {
            if (!c.disqualified && c.safe_uses >= 1 && c.v != null) {
                perf_warning(
                    "PERF023: var initialized via clone_expression and only spliced into qmacro/qmacro_block/qmacro_expr/qmacro_block_to_array; the splice already clones (apply_template), so drop the pre-clone and inline the source expression",
                    c.v.at)
            }
        }
        for (c in perf024_candidates) {
            if (!c.disqualified && c.safe_uses >= 1 && c.v != null) {
                perf_warning(
                    "PERF024: var initialized via clone_* and only passed to a function annotated [clone(...)] at the matching arg position; the callee clones internally, so drop the pre-clone and inline the source",
                    c.v.at)
            }
        }
        perf023_candidates |> clear
        perf023_splice_depth = 0
        perf024_candidates |> clear
        perf024_skip_iptr = 0ul
        perf030_released |> clear
        current_function = null
        in_template = false
        return <- fn
    }

    def override preVisitExprMakeBlock(var expr : ExprMakeBlock?) : void {
        if (expr._block != null && expr._block is ExprBlock && expr._type != null && expr._type.baseType == Type.tBlock
                && !expr.mmFlags.isLambda && !expr.mmFlags.isLocalFunction && !expr.genFlags.generated
                && !(expr._block as ExprBlock).genFlags.generated) {
            inline_blocks |> push(expr._block as ExprBlock)
            inline_block_entry_depth |> push(inline_aware_loop_depth)
        }
    }

    def override visitExprMakeBlock(var expr : ExprMakeBlock?) : ExpressionPtr {
        if (!empty(inline_blocks) && expr._block != null && expr._block is ExprBlock
                && inline_blocks |> back() == expr._block as ExprBlock) {
            inline_blocks |> pop()
            inline_block_entry_depth |> pop()
        }
        return <- expr
    }

    def inline_barrier() : int => empty(inline_block_entry_depth) ? 0 : inline_block_entry_depth |> back()

    def tail_exit_kind(blk : ExprBlock?) : BlockTailExit {
        if (blk == null || empty(blk.list)) return BlockTailExit.none
        let lastE = blk.list[length(blk.list) - 1]
        if (lastE is ExprReturn) return BlockTailExit.returns
        if (lastE is ExprBreak) return BlockTailExit.breaks
        return BlockTailExit.none
    }

    def override preVisitExprBlock(blk : ExprBlock?) : void {
        if (blk.blockFlags.isClosure) {
            in_closure++
            string_builder_save_stack |> push(string_builder_target_var)
            string_builder_target_var = null
            if (!(inline_blocks |> has_value(blk))) {
                in_deferred++
            }
        }
        scope_stack |> push(length(var_stack))
        block_tail_exit |> push(tail_exit_kind(blk))
    }

    def override preVisitExprBlockArgument(blk : ExprBlock?; var arg : VariablePtr; lastArg : bool) : void {
        if (in_deferred == 0) {
            var_stack |> push(VarStackEntry(v = arg, depth = loop_depth, inline_aware_depth = inline_aware_loop_depth, is_iter = false))
        }
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            in_closure--
            if (!(inline_blocks |> has_value(blk))) {
                in_deferred--
            }
            if (!empty(string_builder_save_stack)) {
                string_builder_target_var = string_builder_save_stack |> back()
                string_builder_save_stack |> pop()
            }
        }
        if (!empty(scope_stack)) {
            var_stack |> resize(scope_stack |> back())
            scope_stack |> pop()
        }
        if (!empty(block_tail_exit)) {
            block_tail_exit |> pop()
        }
        return <- blk
    }

    def override preVisitExprReturn(expr : ExprReturn?) : void {
        return_frames |> push(Perf031Return(loop_depth_at_return = inline_aware_loop_depth, barrier = inline_barrier()))
    }

    def override visitExprReturn(var expr : ExprReturn?) : ExpressionPtr {
        if (!empty(return_frames)) {
            return_frames |> pop()
        }
        return <- expr
    }

    def override preVisitGlobalLetVariable(v : VariablePtr; _last : bool) : void {
        check_perf032_declaration(v._type, v.annotation, v.at)
    }

    def override preVisitStructureField(_st : StructurePtr; decl : FieldDeclaration; _last : bool) : void {
        check_perf032_declaration(decl._type, decl.annotation, decl.at)
    }

    def override preVisitExprLetVariable(expr : ExprLet?; var v : VariablePtr; last : bool) : void {
        check_perf032_declaration(v._type, v.annotation, v.at)
        if (in_deferred == 0) {
            var_stack |> push(VarStackEntry(v = v, depth = loop_depth, inline_aware_depth = inline_aware_loop_depth, is_iter = false))
        }
        if (in_closure == 0 && v != null && v.init != null && v.init is ExprCall) {
            let initCall = v.init as ExprCall
            if (initCall.name == "clone_expression" && length(initCall.arguments) == 1) {
                perf023_candidates |> push(Perf023Candidate(v = v, safe_uses = 0, disqualified = false))
            }
        }
        if (in_closure == 0 && v != null && is_clone_init_call(v.init)) {
            perf024_candidates |> push(Perf024Candidate(v = v, safe_uses = 0, disqualified = false))
        }
        if (last && v.flags.init_via_move && v.init != null) {
            var dominated_by_clone = false
            var ini = v.init
            if (ini is ExprCall) {
                let ecall = ini as ExprCall
                if (ecall.func != null) {
                    let fname = string(ecall.func.fromGeneric != null ? ecall.func.fromGeneric.name : ecall.func.name)
                    dominated_by_clone = fname == "clone_to_move"
                }
            }
            pending_move_var = v
            pending_move_at = expr.at
            pending_move_is_clone = dominated_by_clone
        }
    }

    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) : void {
        if (pending_move_var != null) {
            var e = expr
            if (e is ExprReturn) {
                var ret = e as ExprReturn
                if (ret.returnFlags.moveSemantics && ret.subexpr != null) {
                    var sub = ret.subexpr
                    if (sub is ExprRef2Value) {
                        sub = (sub as ExprRef2Value.subexpr)
                    }
                    if (sub is ExprVar && (sub as ExprVar.variable) == pending_move_var) {
                        if (pending_move_is_clone) {
                            perf_warning("PERF009: redundant clone into variable immediately returned; use return clone_to_move(expr) directly", pending_move_at)
                        } else {
                            perf_warning("PERF009: redundant move into variable immediately returned; use return <- expr directly", pending_move_at)
                        }
                    }
                }
            }
            pending_move_var = null
        }
    }

    def override preVisitExprFor(expr : ExprFor?) : void {
        if (in_closure == 0) {
            scope_stack |> push(length(var_stack))
            if_depth_stack |> push(0)
            current_for_known_length = true
            loop_has_early_exit |> push(false)
            perf018_push_sentinel(expr)
            perf022_push_sentinel(expr)
        }
    }

    def is_known_length_source(src : Expression?) : bool {
        if (src == null || src._type == null) return false
        let bt = src._type.baseType
        return (bt == Type.tArray || bt == Type.tString
                || bt == Type.tRange || bt == Type.tRange64
                || bt == Type.tURange || bt == Type.tURange64
                || bt == Type.tFixedArray)
    }

    def override preVisitExprForSource(expr : ExprFor?; src : ExpressionPtr; last : bool) : void {
        if (in_closure == 0 && !is_known_length_source(src)) {
            current_for_known_length = false
        }
        if (in_closure == 0) {
            perf018_record_source(src)
            perf022_record_source(src)
        }
    }

    def override preVisitExprForBody(expr : ExprFor?) : void {
        if (in_closure == 0) {
            known_length_stack |> push(current_for_known_length)
            if (current_for_known_length) {
                known_length_loop_depth++
            }
        }
        if (in_closure == 0) {
            loop_depth++
            perf018_finalize_on_body()
            perf022_inspect_body(expr.body)
        }
        if (in_deferred == 0) {
            inline_aware_loop_depth++
        }
    }

    def override preVisitExprForVariable(expr : ExprFor?; var v : VariablePtr; last : bool) : void {
        if (in_deferred == 0) {
            var_stack |> push(VarStackEntry(v = v, depth = loop_depth, inline_aware_depth = inline_aware_loop_depth, is_iter = true))
        }
        if (in_closure == 0) {
            perf018_record_var(v)
            perf022_record_var(v)
        }
    }

    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        if (in_deferred == 0) {
            inline_aware_loop_depth--
        }
        if (in_closure == 0) {
            perf018_pop_and_check()
            perf022_pop_and_check()
            loop_depth--
            if (!empty(known_length_stack)) {
                if (known_length_stack |> back()) {
                    known_length_loop_depth--
                }
                known_length_stack |> pop()
            }
            pop_loop_scope()
        }
        return <- expr
    }

    def perf018_push_sentinel(expr : ExprFor?) : void {
        //! Always pushed in preVisitExprFor — keeps the stack in lockstep with for-loop nesting so visitExprFor can pop unconditionally. Matched/ disqualified status is filled in by the per-source/per-variable hooks.
        var state = Perf018State(disqualified = true, matched = false)
        perf018_stack    |> push(state)
        perf018_for_exprs |> push(unsafe(reinterpret<ExprFor?>(expr)))
    }

    def perf018_pop_and_check() : void {
        if (empty(perf018_stack)) return
        let top_idx = length(perf018_stack) - 1
        let state = perf018_stack[top_idx]
        let owning = perf018_for_exprs[top_idx]
        perf018_stack    |> pop()
        perf018_for_exprs |> pop()
        if (state.matched && !state.disqualified && state.qualified_count > 0 && owning != null) {
            if (state.perf029_sibling_count > 0 && state.arr_var != null) {
                perf_warning("PERF029: parallel-array indexing - i subscripts {state.arr_var.name}{state.perf029_sibling_names} in lockstep; zip them: 'for (a, b, ... in {state.arr_var.name}{state.perf029_sibling_names})'", owning.at)
            } else {
                perf_warning("PERF018: for (i in range(length(X))) where i only indexes X; use 'for (c in X)' for direct iteration", owning.at)
            }
        }
    }

    def perf018_bare_var_of(var e : ExpressionPtr) : Variable? {
        //! Returns the underlying Variable if `e` (after one optional Ref2Value peel) is a bare ExprVar. Null otherwise — field paths, calls, etc. all intentionally disqualify PERF018's arr resolution.
        var inner = e
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        var evar = inner as ExprVar
        return evar.variable
    }

    def perf018_peel_index_cast(var e : ExpressionPtr) : ExpressionPtr {
        //! One optional cast layer around range's argument: ExprCast, or a 1-arg workhorse int-cast call (int/uint/int64/uint64). Casts don't change what the loop iterates, so urange(uint(length(x))) reduces to the same length shape.
        if (e == null) return e
        if (e is ExprCast) return (e as ExprCast.subexpr)
        if (e is ExprCall) {
            var c = e as ExprCall
            if (c.func != null && length(c.arguments) == 1
                && (c.func.name == "int" || c.func.name == "uint"
                    || c.func.name == "int64" || c.func.name == "uint64")) {
                return c.arguments[0]
            }
        }
        return e
    }

    def perf018_record_source(src : ExpressionPtr) : void {
        //! Called from preVisitExprForSource. Peels `range/urange/range64/urange64(length/long_length(<bareVar>))` (with one optional cast on either layer) and, when matched, stamps the candidate's arr_var. Iter var arrives via perf018_record_var.
        if (empty(perf018_stack)) return
        let top_idx = length(perf018_stack) - 1
        var state = perf018_stack[top_idx]
        state.sources_seen++
        if (state.sources_seen > 1) {
            state.disqualified = true
            perf018_stack[top_idx] = state
            return
        }
        var s = src
        if (s is ExprCast) {
            s = (s as ExprCast.subexpr)
        }
        if (s == null || !(s is ExprCall)) {
            perf018_stack[top_idx] = state
            return
        }
        var rng = s as ExprCall
        if (rng.func == null || length(rng.arguments) != 1
            || (rng.func.name != "range" && rng.func.name != "urange"
                && rng.func.name != "range64" && rng.func.name != "urange64")) {
            perf018_stack[top_idx] = state
            return
        }
        var inner = perf018_peel_index_cast(rng.arguments[0])
        if (inner == null || !(inner is ExprCall)) {
            perf018_stack[top_idx] = state
            return
        }
        var len = inner as ExprCall
        if (len.func == null || length(len.arguments) != 1
            || (len.func.name != "length" && len.func.name != "long_length")) {
            perf018_stack[top_idx] = state
            return
        }
        var av = perf018_bare_var_of(len.arguments[0])
        if (av == null) {
            perf018_stack[top_idx] = state
            return
        }
        state.arr_var = av
        perf018_stack[top_idx] = state
    }

    def perf018_record_var(var v : Variable?) : void {
        //! Called from preVisitExprForVariable. Stamps iter_var on the top frame.
        if (empty(perf018_stack) || v == null) return
        let top_idx = length(perf018_stack) - 1
        var state = perf018_stack[top_idx]
        state.vars_seen++
        if (state.vars_seen > 1) {
            state.disqualified = true
            perf018_stack[top_idx] = state
            return
        }
        state.iter_var = v
        perf018_stack[top_idx] = state
    }

    def perf018_finalize_on_body() : void {
        //! Called from preVisitExprForBody — fires once per for, after every preVisitExprForSource and preVisitExprForVariable. Flips matched=true iff the loop had exactly one source we recognized (arr_var set) and exactly one iter var (iter_var set).
        if (empty(perf018_stack)) return
        let top_idx = length(perf018_stack) - 1
        var state = perf018_stack[top_idx]
        if (state.sources_seen == 1 && state.vars_seen == 1
            && state.arr_var != null && state.iter_var != null) {
            state.matched = true
            state.disqualified = false
        }
        perf018_stack[top_idx] = state
    }

    def perf018_at_matches(var at : ExprAt?; frame_idx : int) : bool {
        //! True when `at.subexpr[at.index]` accesses the frame's arr_var (bare) and indexes by the frame's iter_var with no arithmetic. Conservative by design — field paths, casts on the subexpr, etc. don't qualify.
        if (frame_idx < 0 || frame_idx >= length(perf018_stack)) return false
        let state = perf018_stack[frame_idx]
        if (state.disqualified || !state.matched
            || state.arr_var == null || state.iter_var == null) return false
        let av = perf018_bare_var_of(at.subexpr)
        if (av != state.arr_var) return false
        var idx = at.index
        if (idx is ExprRef2Value) {
            idx = (idx as ExprRef2Value.subexpr)
        }
        return idx != null && idx is ExprVar && (idx as ExprVar).variable == state.iter_var
    }

    def perf029_other_array_of(var at : ExprAt?; frame_idx : int) : Variable? {
        if (frame_idx < 0 || frame_idx >= length(perf018_stack)) return null
        let state = perf018_stack[frame_idx]
        if (state.disqualified || !state.matched
            || state.arr_var == null || state.iter_var == null) return null
        var av = perf018_bare_var_of(at.subexpr)
        if (av == null || av == state.arr_var || av._type == null) return null
        let bt = av._type.baseType
        if (bt != Type.tArray && bt != Type.tFixedArray) return null
        var idx = at.index
        if (idx is ExprRef2Value) {
            idx = (idx as ExprRef2Value.subexpr)
        }
        if (idx == null || !(idx is ExprVar) || (idx as ExprVar).variable != state.iter_var) return null
        return av
    }

    def perf018_on_at(var expr : ExprAt?) : void {
        var pushes = 0
        let n = length(perf018_stack)
        for (i in range(n)) {
            if (perf018_at_matches(expr, i)) {
                var st = perf018_stack[i]
                st.qualified_count++
                perf018_stack[i] = st
                pushes++
            } else {
                var av = perf029_other_array_of(expr, i)
                if (av != null) {
                    var st = perf018_stack[i]
                    st.perf029_sibling_count++
                    let akey = "|{intptr(av)}|"
                    if (find(st.perf029_sibling_keys, akey) < 0) {
                        st.perf029_sibling_keys += akey
                        st.perf029_sibling_names += ", {av.name}"
                    }
                    perf018_stack[i] = st
                    pushes++
                }
            }
        }
        if (pushes > 0) {
            in_qualifying_idx += pushes
        }
        at_qualifying_pushes |> push(pushes)
    }

    def perf018_off_at() : void {
        if (empty(at_qualifying_pushes)) return
        let n = at_qualifying_pushes |> back()
        at_qualifying_pushes |> pop()
        in_qualifying_idx -= n
    }

    def perf018_on_var(v : Variable?) : void {
        //! Called from preVisitExprVar: a bare iter_var reference outside any qualifying ExprAt index slot disqualifies the matching frame.
        if (v == null || empty(perf018_stack) || in_qualifying_idx > 0) return
        let n = length(perf018_stack)
        for (i in range(n)) {
            var st = perf018_stack[i]
            if (st.matched && !st.disqualified && st.iter_var == v) {
                st.disqualified = true
                perf018_stack[i] = st
            }
        }
    }

    def perf022_push_sentinel(expr : ExprFor?) : void {
        //! The warning fires at the for-loop's `at`, not the push site — the three-function protocol is in daslib/ARCHITECTURE_LINT.md.
        var state : Perf022State
        perf022_stack |> push(state)
        perf022_for_exprs |> push(unsafe(reinterpret<ExprFor?>(expr)))
    }

    def perf022_record_source(src : Expression?) : void {
        if (empty(perf022_stack)) return
        let top_idx = length(perf022_stack) - 1
        var state = perf022_stack[top_idx]
        state.sources_seen++
        if (state.sources_seen > 1) {
            state.disqualified = true
        }
        if (src == null || src._type == null) {
            state.disqualified = true
        } else {
            let bt = src._type.baseType
            let is_array_src = bt == Type.tArray || bt == Type.tFixedArray
            if (!is_array_src) {
                state.disqualified = true
            }
        }
        perf022_stack[top_idx] = state
    }

    def perf022_record_var(var v : Variable?) : void {
        if (empty(perf022_stack) || v == null) return
        let top_idx = length(perf022_stack) - 1
        var state = perf022_stack[top_idx]
        state.vars_seen++
        if (state.vars_seen > 1) {
            state.disqualified = true
            perf022_stack[top_idx] = state
            return
        }
        state.iter_var = v
        perf022_stack[top_idx] = state
    }

    def perf022_inspect_body(body : Expression?) : void {
        //! Called from preVisitExprForBody. Flips `matched` iff the body is a single-statement ExprBlock containing exactly one ExprCall to push/push_clone with arg[1] = the frame's iter_var. Compiler folds 'B |> push(s)', 'B.push(s)', and 'push(B, s)' to the same shape, so this single check covers all three call forms.
        if (empty(perf022_stack)) return
        let top_idx = length(perf022_stack) - 1
        var state = perf022_stack[top_idx]
        if (state.disqualified || state.iter_var == null
            || state.sources_seen != 1 || state.vars_seen != 1
            || body == null || !(body is ExprBlock)) return
        var blk = body as ExprBlock
        if (length(blk.list) != 1) return
        var stmt = blk.list[0]
        if (stmt == null || !(stmt is ExprCall) || (stmt as ExprCall).func == null
            || length((stmt as ExprCall).arguments) != 2) return
        var call = stmt as ExprCall
        let dest = call.arguments[0]
        if (dest == null || dest._type == null || dest._type.baseType != Type.tArray) return
        let callee_id = perf022_callee_id(call)
        if (callee_id == Perf022Callee.neither) return
        var arg1 = call.arguments[1]
        if (arg1 is ExprRef2Value) {
            arg1 = (arg1 as ExprRef2Value.subexpr)
        }
        if (arg1 == null || !(arg1 is ExprVar)) return
        var ev = arg1 as ExprVar
        if (ev.variable != state.iter_var) return
        state.matched = true
        state.callee_id = callee_id
        perf022_stack[top_idx] = state
    }

    def perf022_callee_id(var call : ExprCall?) : Perf022Callee {
        if (call.func.fromGeneric != null) {
            if (call.func.fromGeneric.name == "push") return Perf022Callee.push
            return call.func.fromGeneric.name == "push_clone" ? Perf022Callee.push_clone : Perf022Callee.neither
        }
        if (call.func.name == "push") return Perf022Callee.push
        return call.func.name == "push_clone" ? Perf022Callee.push_clone : Perf022Callee.neither
    }

    def perf022_pop_and_check() : void {
        if (empty(perf022_stack)) return
        let top_idx = length(perf022_stack) - 1
        let state = perf022_stack[top_idx]
        let owning = perf022_for_exprs[top_idx]
        perf022_stack    |> pop()
        perf022_for_exprs |> pop()
        if (state.matched && !state.disqualified && owning != null) {
            let name = state.callee_id == Perf022Callee.push ? "push" : "push_clone"
            perf_warning("PERF022: for-loop body 'B |> {name}(x)' pushes one element per iteration; use 'B |> {name}_from(A)' for a bulk reserve+copy", owning.at)
        }
    }

    def override preVisitExprAt(var expr : ExprAt?) : void {
        if (in_closure == 0 && expr != null) {
            perf018_on_at(expr)
        }
    }

    def override visitExprAt(var expr : ExprAt?) : ExpressionPtr {
        if (in_closure == 0) {
            perf018_off_at()
        }
        return <- expr
    }

    def override preVisitExprWhile(expr : ExprWhile?) : void {
        if (in_closure == 0) {
            scope_stack |> push(length(var_stack))
            loop_depth++
            if_depth_stack |> push(0)
            loop_has_early_exit |> push(false)
            in_while_cond = true
        }
        if (in_deferred == 0) {
            inline_aware_loop_depth++
        }
    }

    def override preVisitExprWhileBody(expr : ExprWhile?; body : ExpressionPtr) : void {
        in_while_cond = false
    }

    def pop_loop_scope() : void {
        if (!empty(scope_stack)) {
            var_stack |> resize(scope_stack |> back())
            scope_stack |> pop()
        }
        if (!empty(if_depth_stack)) {
            if_depth_stack |> pop()
        }
        if (!empty(loop_has_early_exit)) {
            loop_has_early_exit |> pop()
        }
    }

    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        if (in_deferred == 0) {
            inline_aware_loop_depth--
        }
        if (in_closure == 0) {
            loop_depth--
            pop_loop_scope()
        }
        in_while_cond = false
        return <- expr
    }

    def override preVisitExprBreak(expr : ExprBreak?) : void {
        if (in_closure == 0 && !empty(loop_has_early_exit)) {
            loop_has_early_exit[length(loop_has_early_exit) - 1] = true
        }
    }

    def override preVisitExprContinue(expr : ExprContinue?) : void {
        if (in_closure == 0 && !empty(loop_has_early_exit)) {
            loop_has_early_exit[length(loop_has_early_exit) - 1] = true
        }
    }

    def override preVisitExprIfThenElse(ifte : ExprIfThenElse?) : void {
        if (!empty(if_depth_stack)) {
            if_depth_stack[length(if_depth_stack) - 1]++
        }
    }

    def override visitExprIfThenElse(var ifte : ExprIfThenElse?) : ExpressionPtr {
        if (!empty(if_depth_stack)) {
            if_depth_stack[length(if_depth_stack) - 1]--
        }
        return <- ifte
    }

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (loop_depth > 0 && expr.op == "+=") {
            let v = find_string_var_from_expr(expr.left)
            if (v != null && is_defined_outside_loop(v)) {
                perf_warning("PERF001: string += in loop creates O(n^2) allocations; use build_string() instead", expr.at)
            }
        }
        if (expr.op == "==" || expr.op == "!=") {
            let left_is = is_string_of_das_string(expr.left)
            let right_is = is_string_of_das_string(expr.right)
            if (left_is || right_is) {
                perf_warning("PERF007: string(das_string) in comparison is unnecessary; das_string supports == and != directly", expr.at)
            }
        }
        if (expr.op == "==") {
            var left = expr.left
            if (left is ExprRef2Value) {
                left = (left as ExprRef2Value.subexpr)
            }
            if (left is ExprField) {
                var field = left as ExprField
                if (field.name == "__rtti" && is_get_ptr_of_smart_ptr(field.value)) {
                    perf_warning("PERF008: get_ptr() is unnecessary for is/as; smart_ptr supports type checks directly", expr.at)
                }
            }
        }
        if (is_get_ptr_vs_null(expr.left, expr.right) || is_get_ptr_vs_null(expr.right, expr.left)) {
            perf_warning("PERF010: get_ptr() is unnecessary for null comparison; smart_ptr supports == and != null directly", expr.at)
        }
        if (expr.op == "+=" || expr.op == "-=") {
            check_perf013_incdec(expr)
        }
        if (expr.op == "&&") {
            check_perf014_digit(expr, false)
            check_perf014_alpha(expr, true)
        }
        if (expr.op == "||") {
            check_perf014_digit(expr, true)
            check_perf014_alpha(expr, false)
        }
        if (expr.op == "==" || expr.op == "!=" || expr.op == "<" || expr.op == "<=" || expr.op == ">" || expr.op == ">=") {
            check_perf017_length_zero(expr)
        }
        if (expr.op == "|") {
            check_perf019_int_cast_or(expr)
        }
    }

    def check_perf013_incdec(expr : ExprOp2?) : void {
        if (expr.left == null || !is_workhorse_numeric_scalar(expr.left._type)) return
        let rhs_one = is_const_one(expr.right)
        let rhs_neg = is_const_neg_one(expr.right)
        var delta = 0
        if (rhs_one) {
            delta = expr.op == "+=" ? 1 : -1
        } elif (rhs_neg) {
            delta = expr.op == "+=" ? -1 : 1
        }
        if (delta == 1) {
            perf_warning("PERF013: '{expr.op} {rhs_one ? 1 : -1}' on a numeric scalar; use postfix '++' for a faster, idiomatic increment", expr.at)
        } elif (delta == -1) {
            perf_warning("PERF013: '{expr.op} {rhs_one ? 1 : -1}' on a numeric scalar; use postfix '--' for a faster, idiomatic decrement", expr.at)
        }
    }

    def check_perf019_int_cast_or(expr : ExprOp2?) : void {
        let l = unwrap_int_cast(expr.left)
        let r = unwrap_int_cast(expr.right)
        if (l == null || r == null
            || l._type == null || r._type == null
            || !inner_type_supports_or(l._type)
            || !(l._type |> is_same_type(r._type, RefMatters.no, ConstMatters.no, TemporaryMatters.no))) return
        let bt = l._type.baseType
        let kind = bt == Type.tBitfield ? "bitfield" : "enum"
        perf_warning("PERF019: int(...) | int(...) on the same {kind} type can collapse to int(L | R) (one cast instead of two)", expr.at)
    }

    def check_perf020_redundant_cast(call : ExprCall?) : void {
        if (call == null || call.func == null || empty(call.arguments)
            || (current_function != null && current_function.fromGeneric != null)) return
        let fname = string(call.func.fromGeneric != null ? call.func.fromGeneric.name : call.func.name)
        let target = perf020_target_basetype(fname)
        if (target == Type.none) return
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (arg == null || arg._type == null || arg._type.baseType != target
                || (target == Type.tString && arg._type.flags.temporary)) return
        perf_warning("PERF020: redundant {fname}(...) cast - argument is already {fname}", call.at)
    }

    def cast_call_workhorse_target(e : Expression?; var fname_out : string&) : Type {
        //! If `e` is a workhorse cast call (after peeling ExprRef2Value), return its target Type and write the cast name to `fname_out`. Otherwise return Type.none. Accepts any argument count — `string(int)` is bound with `value,hex,context,at` (4 daslang args), other casts use 1.
        var ee = e
        if (ee != null && ee is ExprRef2Value) {
            ee = (ee as ExprRef2Value.subexpr)
        }
        if (ee == null || !(ee is ExprCall)) return Type.none
        let c = ee as ExprCall
        if (c.func == null || empty(c.arguments)) return Type.none
        let n = string(c.func.fromGeneric != null ? c.func.fromGeneric.name : c.func.name)
        let t = perf020_target_basetype(n)
        if (t == Type.none) return Type.none
        fname_out = n
        return t
    }

    def cast_call_arg_basetype(e : Expression?) : Type {
        //! Peel ExprRef2Value, take call.arguments[0], peel ExprRef2Value, return its baseType (or Type.none if anything along the chain is null).
        var ee = e
        if (ee != null && ee is ExprRef2Value) {
            ee = (ee as ExprRef2Value.subexpr)
        }
        if (ee == null || !(ee is ExprCall)) return Type.none
        let c = ee as ExprCall
        if (empty(c.arguments)) return Type.none
        var arg = c.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (arg == null || arg._type == null) return Type.none
        return arg._type.baseType
    }

    def cast_call_tail_args_equal(le, re : Expression?) : bool {
        //! Compare arguments[1..] structurally between two cast calls. Skip ExprFakeContext / ExprFakeLineInfo (auto-injected by the typer for Context*/LineInfoArg* parameters; differ at every call site by design). The remaining tail args — e.g. `string(int)`'s `hex` flag — must match in count and via expr_equal_struct, otherwise the hoist would silently lose semantically-significant arguments.
        var lee = le
        var ree = re
        if (lee != null && lee is ExprRef2Value) {
            lee = (lee as ExprRef2Value.subexpr)
        }
        if (ree != null && ree is ExprRef2Value) {
            ree = (ree as ExprRef2Value.subexpr)
        }
        if (lee == null || ree == null || !(lee is ExprCall) || !(ree is ExprCall)) return false
        let lc = lee as ExprCall
        let rc = ree as ExprCall
        var li = 1
        var ri = 1
        let ln = length(lc.arguments)
        let rn = length(rc.arguments)
        while (true) {
            while (li < ln && (lc.arguments[li] is ExprFakeContext || lc.arguments[li] is ExprFakeLineInfo)) {
                li++
            }
            while (ri < rn && (rc.arguments[ri] is ExprFakeContext || rc.arguments[ri] is ExprFakeLineInfo)) {
                ri++
            }
            if (li >= ln || ri >= rn) return (li >= ln && ri >= rn)
            if (!expr_equal_struct(lc.arguments[li], rc.arguments[ri], false)) return false
            li++
            ri++
        }
        return true
    }

    def check_perf021_ternary_cast_hoist(expr : ExprOp3?) : void {
        if (expr == null || expr.op != "?"
            || expr.left == null || expr.right == null) return
        var lname = ""
        var rname = ""
        let lt = cast_call_workhorse_target(expr.left, lname)
        let rt = cast_call_workhorse_target(expr.right, rname)
        let la = cast_call_arg_basetype(expr.left)
        let ra = cast_call_arg_basetype(expr.right)
        if (lt == Type.none || rt == Type.none || lt != rt || lname != rname
            || la == Type.none || la != ra
            || !cast_call_tail_args_equal(expr.left, expr.right)) return
        perf_warning("PERF021: redundant per-branch {lname}(...) cast in ternary - hoist as {lname}(cond ? a : b)", expr.at)
    }

    def parse_range_leg(leg : Expression?; upper_op, lower_op : string; var v_out : Expression?&; var bound_out : int&; var bounds_above : bool&) : bool {
        //! Parse one comparison leg of a char-class range check.
        if (leg == null || !(leg is ExprOp2)) return false
        var op2 = leg as ExprOp2
        let opname = string(op2.op)
        if (opname != upper_op && opname != lower_op) return false
        if (op2.right is ExprConstInt && op2.left != null) {
            v_out = op2.left
            bound_out = (op2.right as ExprConstInt).value
            bounds_above = opname == upper_op
            return true
        }
        if (op2.left is ExprConstInt && op2.right != null) {
            v_out = op2.right
            bound_out = (op2.left as ExprConstInt).value
            bounds_above = opname == lower_op
            return true
        }
        return false
    }

    def parse_bounded_range(expr : ExprOp2?; negated : bool; var v_out : Expression?&; var lo_out : int&; var hi_out : int&) : bool {
        //! Parse a single bounded range on one variable from a two-leg ExprOp2.
        if (expr == null || expr.op != (negated ? "||" : "&&")) return false
        let upper_op = negated ? ">" : "<="
        let lower_op = negated ? "<" : ">="
        var va : Expression? = null
        var vb : Expression? = null
        var bound_a = 0
        var bound_b = 0
        var hi_a = false
        var hi_b = false
        if (!parse_range_leg(expr.left, upper_op, lower_op, va, bound_a, hi_a)
            || !parse_range_leg(expr.right, upper_op, lower_op, vb, bound_b, hi_b)
            || !expr_equal_struct(va, vb)
            || hi_a == hi_b) return false
        let lo = hi_a ? bound_b : bound_a
        let hi = hi_a ? bound_a : bound_b
        if (lo >= hi) return false
        v_out = va
        lo_out = lo
        hi_out = hi
        return true
    }

    def check_perf014_digit(expr : ExprOp2?; negated : bool) : void {
        //! Exact: `c >= '0' && c <= '9'` → is_number(c); the negated De Morgan complement `c < '0' || c > '9'` → !is_number(c).
        var v : Expression? = null
        var lo = 0
        var hi = 0
        if (!parse_bounded_range(expr, negated, v, lo, hi) || lo != 48 || hi != 57) return
        let neg = negated ? "!" : ""
        perf_warning("PERF014: char-class range check{negated ? " (negated)" : ""} '0'..'9' - use '{neg}is_number({describe(v)})' from strings module", expr.at)
    }

    def check_perf014_alpha(expr : ExprOp2?; negated : bool) : void {
        //! Exact: `('a'..'z') || ('A'..'Z')` → is_alpha(c). Its De Morgan negation `(c < 'a' || c > 'z') && (c < 'A' || c > 'Z')` → !is_alpha(c). is_alpha is DEFINED as exactly this both-case union, so a single-case range is intentionally NOT flagged (no equivalent helper). The top-level join is `||` for the closed form / `&&` for the negated form; each child is a bounded range of the matching polarity.
        if (expr == null || expr.op != (negated ? "&&" : "||")
            || expr.left == null || expr.right == null
            || !(expr.left is ExprOp2) || !(expr.right is ExprOp2)) return
        var vL : Expression? = null
        var vR : Expression? = null
        var loL = 0
        var hiL = 0
        var loR = 0
        var hiR = 0
        if (!parse_bounded_range(expr.left as ExprOp2, negated, vL, loL, hiL)
            || !parse_bounded_range(expr.right as ExprOp2, negated, vR, loR, hiR)
            || !expr_equal_struct(vL, vR)) return
        let lowerUpper = loL == 97 && hiL == 122 && loR == 65 && hiR == 90
        let upperLower = loL == 65 && hiL == 90 && loR == 97 && hiR == 122
        if (!lowerUpper && !upperLower) return
        let neg = negated ? "!" : ""
        perf_warning("PERF014: char-class range check{negated ? " (negated)" : ""} 'a'..'z' + 'A'..'Z' - use '{neg}is_alpha({describe(vL)})' from strings module", expr.at)
    }

    def peel_ref2value(e : Expression? const) : Expression? const {
        if (e != null && e is ExprRef2Value) return (e as ExprRef2Value.subexpr)
        return e
    }

    def const_int_value(e : Expression? const; var v : int&) : bool {
        let inner = peel_ref2value(e)
        if (inner == null) return false
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

    def check_perf017_length_zero(expr : ExprOp2?) : void {
        if (current_function != null) {
            let direct_match = (current_function.name == "empty"
                && current_function._module != null
                && current_function._module.name == "builtin")
            let generic_match = (current_function.fromGeneric != null
                && current_function.fromGeneric.name == "empty"
                && current_function.fromGeneric._module != null
                && current_function.fromGeneric._module.name == "builtin")
            if (direct_match || generic_match) return
        }
        let lhs = peel_ref2value(expr.left)
        let rhs = peel_ref2value(expr.right)
        var len_on_left = true
        if (is_collection_length_call(lhs)) {
            len_on_left = true
        } elif (is_collection_length_call(rhs)) {
            len_on_left = false
        } else {
            return
        }
        let const_side = len_on_left ? rhs : lhs
        var k = 0
        if (!const_int_value(const_side, k) || (k != 0 && k != 1)) return
        let opname = string(expr.op)
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
        if (is_empty_pattern) {
            perf_warning("PERF017: 'length(x) {canon_op} {k}' - use 'empty(x)' (avoids unnecessary strlen on strings)", expr.at)
        } elif (is_nonempty_pattern) {
            perf_warning("PERF017: 'length(x) {canon_op} {k}' - use '!empty(x)' (avoids unnecessary strlen on strings)", expr.at)
        }
    }

    def override preVisitExprOp3(expr : ExprOp3?) : void {
        check_perf021_ternary_cast_hoist(expr)
        if (in_closure > 0 || expr.op != "?"
            || expr.subexpr == null || !(expr.subexpr is ExprOp2)) return
        var cmp = expr.subexpr as ExprOp2
        let cop = string(cmp.op)
        if ((cop != "<" && cop != "<=" && cop != ">" && cop != ">=")
            || expr.left == null || expr.right == null) return
        let t_branch = expr.left
        let f_branch = expr.right
        if (try_perf016_abs(expr, cmp, t_branch, f_branch, cop)) return
        try_perf015_minmax(expr, cmp, t_branch, f_branch, cop)
    }

    def try_perf015_minmax(expr : ExprOp3?; cmp : ExprOp2?; t_branch, f_branch : Expression? const; cop : string) : bool {
        let cl = cmp.left
        let cr = cmp.right
        if (cl == null || cr == null) return false
        let tcl = expr_equal_struct(t_branch, cl)
        let tcr = expr_equal_struct(t_branch, cr)
        let fcl = expr_equal_struct(f_branch, cl)
        let fcr = expr_equal_struct(f_branch, cr)
        let same_order = tcl && fcr
        let swap_order = tcr && fcl
        if (!same_order && !swap_order) return false
        var is_min = false
        if (cop == "<" || cop == "<=") {
            is_min = same_order
        } else {
            is_min = swap_order
        }
        let which = is_min ? "min" : "max"
        perf_warning("PERF015: ternary {which} - use '{which}(a, b)' from math module", expr.at)
        return true
    }

    def try_perf016_abs(expr : ExprOp3?; cmp : ExprOp2?; t_branch, f_branch : Expression? const; cop : string) : bool {
        var x_on_left = false
        if (is_const_zero(cmp.right)) {
            x_on_left = true
        } elif (is_const_zero(cmp.left)) {
            x_on_left = false
        } else {
            return false
        }
        let x_expr = x_on_left ? cmp.left : cmp.right
        if (x_expr == null) return false
        var neg_then = false
        var ok = false
        if (is_neg_of(t_branch, x_expr) && expr_equal_struct(f_branch, x_expr)) {
            neg_then = true
            ok = true
        } elif (expr_equal_struct(t_branch, x_expr) && is_neg_of(f_branch, x_expr)) {
            neg_then = false
            ok = true
        }
        if (!ok) return false
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
        if (!is_abs) return false
        perf_warning("PERF016: ternary abs - use 'abs(x)' from math module", expr.at)
        return true
    }

    def is_neg_of(maybe_neg : Expression? const; ref : Expression? const) : bool {
        //! True when `maybe_neg` is `-ref` structurally.
        if (maybe_neg == null || !(maybe_neg is ExprOp1)) return false
        let op1 = maybe_neg as ExprOp1
        if (op1.op != "-") return false
        return expr_equal_struct(op1.subexpr, ref)
    }

    def override preVisitExprField(expr : ExprField?) : void {
        if (in_closure > 0) return
        if (is_get_ptr_of_smart_ptr(expr.value)) {
            perf_warning("PERF011: get_ptr() is unnecessary for field access; smart_ptr auto-dereferences", expr.at)
        }
    }

    def override preVisitExprSafeField(expr : ExprSafeField?) : void {
        if (in_closure > 0) return
        if (is_get_ptr_of_smart_ptr(expr.value)) {
            perf_warning("PERF011: get_ptr() is unnecessary for field access; smart_ptr auto-dereferences", expr.at)
        }
    }

    def override preVisitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; last : bool) : void {
        if (expr == null || expr.func == null) return
        var indices : array<int>
        get_clone_param_indices(expr.func, indices)
        if (empty(indices)) return
        var arg_index = -1
        for (i in 0 .. length(expr.arguments)) {
            if (intptr(expr.arguments[i]) == intptr(arg)) {
                arg_index = i
                break
            }
        }
        if (arg_index < 0 || !has_value(indices, arg_index)) return
        var inner = arg
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value).subexpr
        }
        if (is_clone_init_call(inner)) {
            perf_warning(
                "PERF024: argument to function annotated [clone({expr.func.arguments[arg_index].name})] is wrapped in clone_*; the callee clones internally, drop the outer clone wrap",
                arg.at)
            return
        }
        if (inner is ExprVar) {
            let vv = (inner as ExprVar).variable
            if (vv != null) {
                let vv_iptr = intptr(vv)
                for (i in 0 .. length(perf024_candidates)) {
                    if (intptr(perf024_candidates[i].v) == vv_iptr) {
                        perf024_candidates[i].safe_uses++
                        perf024_skip_iptr = vv_iptr
                        break
                    }
                }
            }
        }
    }

    def check_perf032_declaration(td : TypeDeclPtr; annotations : AnnotationArgumentList; at : LineInfo) : void {
        if (td == null || !has_annotation(annotations, "exact_size")) return
        if (td.baseType != Type.tArray) {
            perf_warning("PERF032: @exact_size does not apply to `{describe(td, false, false, false)}` - the annotation is an array contract (every resize must follow a reserve/ensure_capacity)", at)
        }
    }

    def check_perf032_sizing_call(var expr : ExprCall?) : void {
        if (empty(expr.arguments)) return
        var dest = expr.arguments[0]
        if (dest._type == null || dest._type.baseType != Type.tArray) return
        let origin = string(expr.func.fromGeneric != null ? expr.func.fromGeneric.name : expr.func.name)
        if (is_explicit_capacity_call(origin)) {
            perf032_reserved |> insert(describe(dest))
        } elif ((origin == "resize" || origin == "resize_no_init") && is_annotated_dest(dest, "exact_size")) {
            if (!key_exists(perf032_reserved, describe(dest))) {
                perf_warning("PERF032: {origin} on the @exact_size array `{describe(dest)}` with no reserve/ensure_capacity earlier in this function - an unreserved resize rounds capacity up to the next power of two, and past max_unreserved_size it panics, only when the big input arrives; reserve(need) or ensure_capacity(need) first, or size it through a helper that does", expr.at)
            }
        }
    }

    def override preVisitExprCall(var expr : ExprCall?) : void {
        if (expr.func == null) return
        check_perf020_redundant_cast(expr)
        check_perf032_sizing_call(expr)
        if (expr.name == "add_ptr_ref" && !empty(perf023_candidates)) {
            perf023_splice_depth++
        }
        if (in_deferred == 0 && length(expr.arguments) == 1
                && (expr.name == "_::finalize" || expr.func.name == "finalize"
                    || expr.func.name |> starts_with("builtin`finalize`")
                    || expr.func.name |> starts_with("builtin`finalize_dim`"))) {
            perf030_mark_released(expr.arguments[0])
        }
        if (in_closure > 0) return
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = expr
            if (loop_depth > 0) {
                in_character_at_call++
                current_character_at = ecall
            }
        }
        if (in_while_cond && expr.func.name == "length" && expr.func._module.name == "strings") {
            in_length_while_call++
        }
    }

    def check_perf031_slice_in_loop(var expr : ExprCall?) {
        if (in_deferred > 0 || inline_aware_loop_depth <= 0 || !(expr.func.name == "slice" || expr.func.name == "chop") || expr.func._module.name != "strings"
                || empty(expr.arguments) || expr.arguments[0]._type == null || expr.arguments[0]._type.baseType != Type.tString) {
            return
        }
        if (!empty(return_frames)) {
            let rf = return_frames |> back()
            if (rf.barrier == 0 && rf.loop_depth_at_return == inline_aware_loop_depth) return
        }
        if (!empty(block_tail_exit) && inline_barrier() == 0) {
            let tail = block_tail_exit |> back()
            if (tail == BlockTailExit.returns || (tail == BlockTailExit.breaks && inline_aware_loop_depth == 1)) return
        }
        var spath = ""
        let srv = find_expr_path(expr.arguments[0], spath)
        if (srv != null && !srv.flags.generated && is_defined_outside_inline_aware_loop(srv)) {
            perf_warning("PERF031: {expr.func.name} re-strlens its source string on every call; in a loop over the same string this is O(n^2) - wrap the loop in peek_data(s) $(d) and {expr.func.name}(d, ...) the view, or hoist it", expr.at)
        }
    }

    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (expr.func == null) return <- expr
        if (expr.name == "add_ptr_ref" && perf023_splice_depth > 0) {
            perf023_splice_depth--
        }
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = expr
            if (in_closure == 0 && loop_depth > 0 && in_character_at_call > 0) {
                in_character_at_call--
                current_character_at = null
            }
            if (!reported_character_at |> has_value(ecall)) {
                if (is_character_at_zero(expr)) {
                    perf_warning("PERF003: character_at(s, 0) can be replaced with first_character(s)", expr.at)
                } else {
                    perf_warning("PERF003: character_at is O(n) due to strlen; consider peek_data() for hot paths", expr.at)
                }
            }
        }
        check_perf031_slice_in_loop(expr)
        if (is_array_func(expr, "reserve") || is_array_func(expr, "ensure_capacity")) {
            var path = ""
            let rv = find_expr_path(expr.arguments[0], path)
            if (rv != null) {
                let key = make_path_key(rv, path)
                if (!reserved_paths |> has_value(key)) {
                    reserved_paths |> push(key)
                }
            }
        }
        let in_conditional = !empty(if_depth_stack) && if_depth_stack[length(if_depth_stack) - 1] > 0
        let has_early_exit = !empty(loop_has_early_exit) && loop_has_early_exit[length(loop_has_early_exit) - 1]
        if (in_closure == 0 && known_length_loop_depth > 0 && !in_conditional && !has_early_exit) {
            if (is_array_func(expr, "push") || is_array_func(expr, "push_clone") || is_array_func(expr, "emplace")) {
                var path = ""
                let rv = find_expr_path(expr.arguments[0], path)
                if (rv != null && !rv.flags.generated && is_defined_outside_loop(rv)
                    && !reserved_paths |> has_value(make_path_key(rv, path))) {
                    let fname = string(expr.func.fromGeneric.name)
                    perf_warning("PERF006: {fname} in loop without prior reserve() may cause repeated reallocations; consider reserve() before the loop", expr.at)
                }
            }
        }
        if (expr.func._module.name == "strings" && expr.func.name != "string") {
            for (arg in expr.arguments) {
                if (is_string_of_das_string(arg)) {
                    perf_warning("PERF012: string(das_string) passed to strings function; use peek(das_string) instead", expr.at)
                    break
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

    def override preVisitExprCopy(expr : ExprCopy?) : void {
        if (in_closure > 0 || loop_depth == 0) return
        var v = find_string_var_from_expr(expr.left)
        if (v != null && is_defined_outside_loop(v) && expr.right is ExprStringBuilder) {
            string_builder_target_var = v
        }
    }

    def override visitExprCopy(var expr : ExprCopy?) : ExpressionPtr {
        string_builder_target_var = null
        return <- expr
    }

    def override preVisitExprMove(expr : ExprMove?) : void {
        check_perf030_move_leak(expr)
        if (in_closure > 0 || loop_depth == 0) return
        var v = find_string_var_from_expr(expr.left)
        if (v != null && is_defined_outside_loop(v) && expr.right is ExprStringBuilder) {
            string_builder_target_var = v
        }
    }

    def check_perf030_move_leak(expr : ExprMove?) : void {
        if (expr == null || expr.move_flags.podDelete || expr.genFlags.generated) return
        var lhs = expr.left
        if (lhs is ExprRef2Value) {
            lhs = (lhs as ExprRef2Value.subexpr)
        }
        if (lhs == null || !(lhs is ExprVar)) return
        let lv = (lhs as ExprVar).variable
        if (lv == null || lv.flags.aliasCMRES || lv.flags.generated
                || (lv._type != null && lv._type.flags.ref)
                || !perf030_needs_release(expr.left._type)) return
        let released = perf030_released |> find_index(lv)
        if (released >= 0) {
            perf030_released |> erase(released)
            return
        }
        perf_warning("PERF030: move-assign drops the target's old contents without release (a leak on a persistent heap); when the target can be live, delete it first; when it is fresh, fold the move into the declaration (a `cond ? a : b` init covers conditional fills); or enable force_inscope_pod so the compiler collects it", expr.at)
    }

    def perf030_mark_released(sub : ExpressionPtr) : void {
        var e = sub
        if (e is ExprRef2Value) {
            e = (e as ExprRef2Value.subexpr)
        }
        while (e != null && e is ExprField) {
            e = (e as ExprField).value
            if (e is ExprRef2Value) {
                e = (e as ExprRef2Value.subexpr)
            }
        }
        if (e == null || !(e is ExprVar)) return
        var v = (e as ExprVar).variable
        if (v != null && perf030_released |> find_index(v) < 0) {
            perf030_released |> push(v)
        }
    }

    def override preVisitExprDelete(var expr : ExprDelete?) : void {
        if (in_deferred == 0) {
            perf030_mark_released(expr.subexpr)
        }
    }

    def override visitExprMove(var expr : ExprMove?) : ExpressionPtr {
        string_builder_target_var = null
        return <- expr
    }

    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) : void {
        if (string_builder_target_var != null) {
            in_string_builder_check++
        }
    }

    def override visitExprStringBuilder(var expr : ExprStringBuilder?) : ExpressionPtr {
        if (in_string_builder_check > 0) {
            in_string_builder_check--
        }
        return <- expr
    }

    def override preVisitExprStringBuilderElement(sb : ExprStringBuilder?; elem : ExpressionPtr; last : bool) {
        let inner = perf025_unwrap_element(elem)
        let value = perf025_string_call_value(inner)
        return if (value == null)
        let kind = perf025_value_kind(value)
        if (kind == Perf025Kind.cast_is_noop) {
            perf_warning("PERF025: string(...) inside string interpolation is redundant - the interpolation already converts to string; drop the cast", inner.at)
        } elif (kind == Perf025Kind.cast_changes_radix) {
            perf_warning("PERF025: string(...) inside string interpolation is redundant; drop the cast - unsigned values interpolate as hex by default, so use the '\{x:d\}' format tag to keep decimal output", inner.at)
        }
    }

    def override preVisitExprVar(expr : ExprVar?) : void {
        if (!empty(perf023_candidates)) {
            let vv = expr.variable
            if (vv != null) {
                for (i in 0 .. length(perf023_candidates)) {
                    if (perf023_candidates[i].v == vv) {
                        if (perf023_splice_depth > 0) {
                            perf023_candidates[i].safe_uses++
                        } else {
                            perf023_candidates[i].disqualified = true
                        }
                        break
                    }
                }
            }
        }
        if (!empty(perf024_candidates)) {
            let vv4 = expr.variable
            if (vv4 != null) {
                let vv4_iptr = intptr(vv4)
                if (perf024_skip_iptr == vv4_iptr && vv4_iptr != 0ul) {
                    perf024_skip_iptr = 0ul
                } else {
                    for (i in 0 .. length(perf024_candidates)) {
                        if (intptr(perf024_candidates[i].v) == vv4_iptr) {
                            perf024_candidates[i].disqualified = true
                            break
                        }
                    }
                }
            }
        }
        if (in_closure > 0) return
        let v = expr.variable
        perf018_on_var(v)
        if (in_character_at_call > 0 && is_loop_variable(v)) {
            if (current_character_at != null && !reported_character_at |> has_value(current_character_at)) {
                perf_warning("PERF002: character_at(s, i) in loop is O(n) per call (strlen + bounds check each time); use peek_data(s) to access characters as array<uint8>", expr.at)
                reported_character_at |> push(current_character_at)
            }
        }
        if (in_string_builder_check > 0 && string_builder_target_var != null && v == string_builder_target_var) {
            perf_warning("PERF004: rebuilding string via interpolation in loop creates O(n^2) allocations; use build_string() instead", expr.at)
        }
        if (in_length_while_call > 0 && v != null) {
            if (expr.variable._type != null && expr.variable._type.baseType == Type.tString) {
                if (!expr.variable.access_flags.access_ref) {
                    perf_warning("PERF005: length(string) in while condition recomputes strlen each iteration; cache in a local variable", expr.at)
                }
            }
        }
    }

    def hot_file_of(at : LineInfo) : string {
        return at.fileInfo != null ? "{at.fileInfo.name}" : ""
    }

    def hot_visit_key(fn_hash : uint64; anchor : ExpressionPtr) : uint64 {
        return fn_hash * 1099511628211ul + intptr(anchor)
    }

    def hot_ensure_summary(var fn : Function?; var hc : HotCache) {
        let h = fn.getMangledNameHash
        return if (key_exists(hc.sinks_span_by_fn, h))
        var scan = new HotBodyScan()
        make_visitor(*scan) $(adapter) {
            visit(fn, adapter)
        }
        hc.sinks_span_by_fn |> insert(h, int2(length(hc.sinks_all), length(scan.sinks)))
        hc.sinks_all |> push_from(scan.sinks)
        hc.edges_span_by_fn |> insert(h, int2(length(hc.edges_all), length(scan.edges)))
        hc.edges_all |> push_from(scan.edges)
        unsafe { delete scan; }
    }

    def hot_report(frames : array<HotFrame>; fi : int; sink : HotSink; want : HotBits; root_file : string) {
        return if ((uint(sink.bit) & uint(want)) == 0u)
        let code = sink.bit.perf026_alloc ? "PERF026" : (sink.bit.perf027_env ? "PERF027" : "PERF028")
        return if (is_lint_suppressed(sink.site.at, code))
        var si = fi
        while (si >= 0) {
            if (frames[si].site != null && is_lint_suppressed(frames[si].site.at, code)) {
                return
            }
            si = frames[si].parent
        }
        var at = sink.site.at
        let shared_node = frames[fi].fn.flags.generated || frames[fi].fn.fromGeneric != null
        if ((hot_file_of(at) != root_file || shared_node) && frames[fi].nearest_root_file_site != null) {
            at = frames[fi].nearest_root_file_site.at
        }
        var sink_pos = ""
        if (sink.site.at.line != at.line || hot_file_of(sink.site.at) != hot_file_of(at)) {
            sink_pos = " [at {hot_file_of(sink.site.at)}:{int(sink.site.at.line)}]"
        }
        var parts : array<string>
        var pi = fi
        while (pi >= 0) {
            var pf = frames[pi].fn
            parts |> push(string(pf.fromGeneric != null ? pf.fromGeneric.name : pf.name))
            pi = frames[pi].parent
        }
        let chain = build_string() $(var w) {
            var k = length(parts) - 1
            while (k >= 0) {
                w |> write(parts[k])
                if (k > 0) {
                    w |> write(" -> ")
                }
                k--
            }
        }
        delete parts
        let esc = "[cold_path] on the callee, // nolint on the line, or DAS_LINT_DISABLE for a whole run"
        let esc_alloc = "@scratch on the destination field if it is a reused buffer, {esc}"
        if (sink.bit.perf026_alloc) {
            perf_warning("PERF026: {sink.what} allocates or frees on a hot path, via {chain}{sink_pos}; hoist it out of the hot path. To allow it: {esc_alloc}", at)
        } elif (sink.bit.perf027_env) {
            perf_warning("PERF027: {sink.what} reads the environment on a hot path, via {chain}{sink_pos}; getenv allocates a string in the context heap on every call - read it once at init and cache the value. To allow it: {esc}", at)
        } else {
            perf_warning("PERF028: {sink.what} performs I/O on a hot path, via {chain}{sink_pos}. To allow it: {esc}", at)
        }
    }

    def scan_hot_root(var root : Function?; var hc : HotCache) {
        let want = hot_bits_of(root)
        let root_file = hot_file_of(root.at)
        var frames : array<HotFrame>
        var visited : table<uint64>
        frames |> push(HotFrame(fn = root, parent = -1))
        visited |> insert(hot_visit_key(root.getMangledNameHash, null))
        var qi = 0
        while (qi < length(frames)) {
            var cur = frames[qi]
            let ci = qi
            qi++
            hot_ensure_summary(cur.fn, hc)
            let h = cur.fn.getMangledNameHash
            let ss = hc.sinks_span_by_fn[h]
            for (si in range(ss.x, ss.x + ss.y)) {
                hot_report(frames, ci, hc.sinks_all[si], want, root_file)
            }
            let es = hc.edges_span_by_fn[h]
            for (ei in range(es.x, es.x + es.y)) {
                var callee = hc.edges_all[ei].fn
                continue if (callee == null || callee.body == null || callee.flags.builtIn
                             || callee.moreFlags.isTemplate || callee.moreFlags.stub
                             || is_cold_path(callee))
                var site = hc.edges_all[ei].caller_site
                var anchor = hot_file_of(site.at) == root_file ? site : cur.nearest_root_file_site
                let key = hot_visit_key(callee.getMangledNameHash, anchor)
                continue if (key_exists(visited, key))
                visited |> insert(key)
                frames |> push(HotFrame(fn = callee, parent = ci, site = site, nearest_root_file_site = anchor))
            }
        }
        delete frames
        delete visited
    }

    def hot_path_scan() {
        return if (empty(hot_roots))
        if (is_suppressed_code("PERF026") && is_suppressed_code("PERF027")
            && is_suppressed_code("PERF028")) {
            hot_roots |> clear()
            return
        }
        var hc : HotCache
        while (!empty(hot_roots)) {
            var root = hot_roots[length(hot_roots) - 1]
            hot_roots |> erase(length(hot_roots) - 1)
            scan_hot_root(root, hc)
        }
        delete hc
    }
}

def public perf_lint(prog : ProgramPtr; compile_time_errors : bool) : int {
    //! Runs the performance lint visitor on the compiled program.
    //! Returns the number of warnings found.
    let empty_set : table<string>
    return perf_lint(prog, compile_time_errors, empty_set)
}

def public perf_lint(prog : ProgramPtr; compile_time_errors : bool; disabled_codes : table<string>) : int {
    //! Filter-aware compile-time variant. ``disabled_codes`` suppresses
    //! the listed rule IDs (e.g. "PERF007") alongside the usual
    //! ``// nolint:CODE`` directives.
    var astVisitor = new PerfLintVisitor(compile_time_errors = compile_time_errors)
    astVisitor.disabled_codes := disabled_codes
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit(prog, astVisitorAdapter)
    }
    astVisitor->hot_path_scan()
    let count = astVisitor.warning_count
    unsafe { delete astVisitor; }
    return count
}

def public perf_lint_collect(prog : ProgramPtr; var warnings : array<string>) : int {
    //! Runs the performance lint visitor and collects warnings as strings.
    //! Returns the number of warnings found.
    let empty_set : table<string>
    return perf_lint_collect(prog, warnings, empty_set, empty_set)
}

def public perf_lint_collect(prog : ProgramPtr; var warnings : array<string>;
                             disabled_codes, enabled_codes : table<string>) : int {
    //! Filter-aware overload. `disabled_codes` is a denylist; `enabled_codes`
    //! is a whitelist (empty == all). Codes use the bare form (e.g. "PERF001").
    var astVisitor = new PerfLintVisitor(compile_time_errors = false, collect_warnings = true)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit(prog, astVisitorAdapter)
    }
    astVisitor->hot_path_scan()
    let count = astVisitor.warning_count
    warnings |> push_from(astVisitor.warnings)
    unsafe { delete astVisitor; }
    return count
}

def public perf_lint_collect_issues(prog : ProgramPtr; var issues : array<LintIssue>;
                                    disabled_codes, enabled_codes : table<string>) : int {
    //! Structured twin of ``perf_lint_collect`` — same filters, but emits
    //! ``LintIssue`` records (rule code + position) instead of display strings.
    var astVisitor = new PerfLintVisitor(compile_time_errors = false, collect_warnings = true)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(astVisitorAdapter) {
        visit(prog, astVisitorAdapter)
    }
    astVisitor->hot_path_scan()
    let count = astVisitor.warning_count
    issues |> push_from(astVisitor.issues)
    unsafe { delete astVisitor; }
    return count
}

[lint_macro]
class PerfLintMacro : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        let disabled <- build_lint_macro_disabled(prog)
        perf_lint(prog, true, disabled)
        return true
    }
}

