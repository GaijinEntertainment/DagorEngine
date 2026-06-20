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
//!   PERF018 — for (i in range(length(arr))) where i only indexes arr — use 'for (c in arr)'
//!   PERF019 — int(T.a) | int(T.b) on bitfield-or-enum-with-operator-| — collapse to int(T.a | T.b)
//!   PERF020 — T(x) where x is already T (workhorse type) — drop the redundant cast
//!   PERF021 — cond ? T(a) : T(b) on workhorse cast T — hoist to T(cond ? a : b)
//!   PERF022 — for (s in A) { B |> push(s) } / push_clone — use 'B |> push_from(A)' (bulk reserve+copy)
//!   PERF023 — var X = clone_expression(E); ... $e(X) ... — qmacro splice already clones; drop the pre-clone
//!   PERF024 — var X = clone_*(E); func(X) where func is [clone(...)]-annotated — callee clones; drop the pre-clone
//!   PERF025 — string(...) inside string interpolation is redundant — interpolation already converts to string

require daslib/ast_boost
require strings
require daslib/lint_config

// ---------------------------------------------------------------------------
// Visitor
// ---------------------------------------------------------------------------

struct VarStackEntry {
    @do_not_delete v : Variable?
    depth : int
    is_iter : bool
}

struct Perf018State {
    //! PERF018 per-for-loop candidate. Each enclosing `for` pushes one; `matched`
    //! flips true once we observe `for (i in range(length(V)))` with exactly one
    //! source and one iter var, where V is a bare Variable. We push a sentinel
    //! for non-matching loops too so visitExprFor can pop unconditionally and
    //! iter_var refs across nesting attribute to the right frame.
    //! `array<T>` requires T trivially-storable, so the owning ExprFor pointer
    //! lives in a parallel `perf018_for_exprs` array.
    @do_not_delete iter_var : Variable?
    @do_not_delete arr_var : Variable?
    qualified_count : int
    sources_seen : int
    vars_seen : int
    disqualified : bool
    matched : bool
}

struct Perf022State {
    //! PERF022 per-for-loop candidate. Detects 'for (s in A) { B |> push(s) }'
    //! (push and push_clone; all three pipe/dot/plain-call shapes compiler-fold
    //! to the same ExprCall). Pushed in preVisitExprFor in lockstep with PERF018.
    //! `matched` is stamped by preVisitExprForBody when body is a single
    //! ExprCall of the right shape against the frame's iter_var. We warn at the
    //! for-loop's `at` (looked up via the parallel `perf022_for_exprs` stack)
    //! rather than the inner push's `at` — the inner push location dedups
    //! against PERF006 which also fires at that exact line:col.
    //! `callee_id`: 1 = push, 2 = push_clone (kept trivial so the struct can live
    //! in an array). String name is rebuilt at warn time.
    @do_not_delete iter_var : Variable?
    callee_id : int
    sources_seen : int
    vars_seen : int
    disqualified : bool
    matched : bool
}

struct Perf024Candidate {
    //! PERF024 per-candidate state. One entry per `var X = clone_*(E)` decl
    //! seen in the current function (any of clone_expression / clone_type /
    //! clone_function / clone_variable / clone_structure). `safe_uses` counts
    //! ExprVar refs to X passed directly as an arg at a [clone(...)]-annotated
    //! position; `disqualified` is set on any other use. Reports the same way
    //! as PERF023: !disqualified && safe_uses >= 1.
    @do_not_delete v : Variable?
    safe_uses : int
    disqualified : bool
}

struct Perf023Candidate {
    //! PERF023 per-candidate state. One entry per `var X = clone_expression(E)`
    //! decl seen in the current function. `safe_uses` counts ExprVar refs to X
    //! whose ancestor chain (within this function) includes an `add_ptr_ref(...)`
    //! call — the splice-wrapper installed by `apply_qrules` for every `$e(...)`
    //! tag. `disqualified` is set on any other use (assignment, call arg passed
    //! to a non-splice consumer, etc.); the pre-clone is then load-bearing. The
    //! `var` carries the decl `at` directly (v.at), so the warning location is
    //! derived without storing LineInfo in the array.
    @do_not_delete v : Variable?
    safe_uses : int
    disqualified : bool
}

enum private Perf025Kind {
    // How a string()-cast value inside interpolation compares to direct
    // interpolation, deciding whether/how PERF025 fires.
    skip            // not flaggable: value-shape change, or not a string() cast
    equivalent      // "{x}" == string(x): drop the cast
    unsigned_hex    // "{x}" is hex but string(x) is decimal: drop + ':d' hint
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
    in_template : bool = false
    // PERF006: known-length loop tracking
    known_length_loop_depth : int = 0
    current_for_known_length : bool = true
    // PERF006: suppress when loop has break/continue (unpredictable iteration count)
    loop_has_early_exit : array<bool>
    // PERF009: track last move-initialized variable for redundant return detection
    @do_not_delete pending_move_var : Variable?
    @safe_when_uninitialized pending_move_at : LineInfo
    // PERF018: per-for-loop index-only candidates; arr_path (string) and the
    // owning ExprFor (for `.at` at warn time) live in parallel arrays kept in
    // lockstep with perf018_stack. at_qualifying_pushes records how many
    // increments each in-flight ExprAt added to in_qualifying_idx so visitExprAt
    // can undo them.
    perf018_stack : array<Perf018State>
    perf018_paths : array<string>
    @do_not_delete perf018_for_exprs : array<ExprFor?>
    in_qualifying_idx : int = 0
    at_qualifying_pushes : array<int>
    // PERF022: per-for-loop bulk-push candidates; same lockstep with for-loop
    // nesting. `array<T>` requires T trivially-storable, so the owning ExprFor
    // pointer lives in a parallel `perf022_for_exprs` array.
    perf022_stack : array<Perf022State>
    @do_not_delete perf022_for_exprs : array<ExprFor?>
    // PERF023: per-function candidates for `var X = clone_expression(E)`. Cleared
    // in preVisitFunction; finalized in visitFunction. `perf023_splice_depth` is
    // a counter incremented when visiting INTO an `add_ptr_ref(...)` call
    // (post-expansion form of `$e(...)` splice tags) and decremented on exit; an
    // ExprVar reference to a candidate that fires while the counter is >0 is a
    // "safe use" (apply_template will clone the substitution anyway), otherwise
    // the candidate is disqualified.
    perf023_candidates : array<Perf023Candidate>
    perf023_splice_depth : int = 0
    // PERF024: per-function candidates for `var X = clone_*(E)` whose only
    // uses are direct args at [clone(...)]-annotated positions. `perf024_skip_iptr`
    // holds the intptr identity of a Variable that's about to be visited via
    // a pre-classified annotated-arg position; cleared in preVisitExprVar so
    // that ExprVar visit doesn't re-classify (would otherwise disqualify).
    perf024_candidates : array<Perf024Candidate>
    perf024_skip_iptr : uint64 = 0ul
    warning_count : int = 0
    // collection mode — when true, warnings are appended to `warnings` instead of to_log
    collect_warnings : bool = false
    warnings : array<string>
    // dedup: reported (file_hash, line, column) to suppress duplicate warnings from generic instantiations
    reported_locations : table<uint64>
    // PERF019: cache of enum-types we've probed for an `operator |` overload.
    // Keys are intptr(Enumeration?); vals say "yes, has operator|".
    perf019_enum_or_probed : array<uint64>
    perf019_enum_or_yes : array<uint64>
    def PerfLintVisitor() {
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

    def perf_warning(text : string; at : LineInfo) : void {
        // Suppress in templates and inside macro-generated functions —
        // the user didn't write that code.
        if (in_template
            || (current_function != null && current_function.flags.generated)) return
        // Deduplicate warnings — same source location reported once, regardless of generic instantiations
        let key = location_key(at)
        if (key_exists(reported_locations, key)) return
        reported_locations |> insert(key)
        // Check inline suppression: // PERFxxx on the same line
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
            compiling_program() |> macro_performance_warning(at, msg)
        } elif (collect_warnings) {
            warnings |> push("performance warning: {msg} at {describe(at)}")
        } else {
            to_log(LOG_WARNING, "performance warning: {msg} at {describe(at)}\n")
        }
    }

    // --- variable scope helpers ---

    def is_loop_variable(v : Variable?) : bool {
        if (v == null) return false
        for (entry in var_stack) {
            if (entry.v == v && entry.is_iter) return true
        }
        return false
    }

    def is_defined_outside_loop(v : Variable?) : bool {
        if (v == null || loop_depth == 0 || is_loop_variable(v)) return false
        for (entry in var_stack) {
            if (entry.v == v) return entry.depth < loop_depth
        }
        // not on stack — function argument or global — outside any loop
        return true
    }

    // --- expression helpers ---

    def is_character_at_zero(expr : ExprCall?) : bool {
        if (length(expr.arguments) >= 2) {
            var idx = expr.arguments[1]
            if (idx is ExprConstInt && (idx as ExprConstInt).value == 0) return true
        }
        return false
    }

    def is_array_func(expr : ExprCall?; fname : string) : bool {
        // Builtin generics (push, reserve, etc.) have fromGeneric set.
        // Verify first argument is an array type.
        if (expr.func == null || expr.func.fromGeneric == null
            || expr.func.fromGeneric.name != fname || empty(expr.arguments)) return false
        let arg = expr.arguments[0]
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
            return evar.variable
        }
        if (expr is ExprRef2Value) return find_expr_path((expr as ExprRef2Value.subexpr), path)
        if (expr is ExprField) {
            var field = expr as ExprField
            path := ".{field.name}{path}"
            return find_expr_path(field.value, path)
        }
        if (expr is ExprSafeField) {
            var field = expr as ExprSafeField
            path := ".{field.name}{path}"
            return find_expr_path(field.value, path)
        }
        if (expr is ExprAt) {
            var at = expr as ExprAt
            path := "[*]{path}"
            return find_expr_path(at.subexpr, path)
        }
        if (expr is ExprSafeAt) {
            var at = expr as ExprSafeAt
            path := "[*]{path}"
            return find_expr_path(at.subexpr, path)
        }
        if (expr is ExprSwizzle) {
            var swiz = expr as ExprSwizzle
            return find_expr_path(swiz.value, path)
        }
        if (expr is ExprCast) {
            var ca = expr as ExprCast
            return find_expr_path(ca.subexpr, path)
        }
        if (expr is ExprRef2Ptr) {
            var rr = expr as ExprRef2Ptr
            return find_expr_path(rr.subexpr, path)
        }
        if (expr is ExprPtr2Ref) {
            var rr = expr as ExprPtr2Ref
            return find_expr_path(rr.subexpr, path)
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
            inner = (inner as ExprRef2Value.subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        var evar = inner as ExprVar
        return null if (evar.variable == null || evar.variable._type == null)
        if (evar.variable._type.baseType == Type.tString) return evar.variable
        return null
    }

    // --- PERF007/PERF008 helpers ---

    def is_string_of_das_string(expr : Expression?) : bool {
        //! Returns true if expr is a string(X) call where X is das_string.
        return false if (expr == null)
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return false if (inner == null || !(inner is ExprCall))
        var call = inner as ExprCall
        // After inference, string() calls may have FakeContext/FakeLineInfo extra args.
        // Check the function name and first argument type.
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

    // --- generic constant / structural helpers (PERF013-017) ---

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
        //! Structural equality via describe(). With `require_pure=true` (default),
        //! returns false when either side has side effects — protects rules that
        //! suggest collapsing duplicated subexpressions (PERF014/015/016) from
        //! silently changing evaluation count.
        if ((a == null || b == null)
            || (require_pure && (!a.flags.noSideEffects || !b.flags.noSideEffects))) return false
        return describe(a) == describe(b)
    }

    def is_workhorse_numeric(t : TypeDecl?) : bool {
        //! True if the type is one of the six numeric workhorse scalars (int, uint,
        //! int64, uint64, float, double). Vectors/bitfields/enums do NOT qualify.
        if (t == null) return false
        let bt = t.baseType
        return (bt == Type.tInt || bt == Type.tUInt
                || bt == Type.tInt64 || bt == Type.tUInt64
                || bt == Type.tFloat || bt == Type.tDouble)
    }

    def is_collection_length_call(expr : Expression? const) : bool {
        //! True for length(string) / length(das_string) / length(array) / length(table).
        //! Excludes math::length(float2/3/4) which returns vector magnitude.
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        if (call.func == null || call.func.name != "length") return false
        let modname = string(call.func._module.name)
        return modname == "strings" || modname == "$" || modname == "builtin"
    }

    // --- PERF019: int(T.a) | int(T.b) cast-collapse helpers ---

    def unwrap_int_cast(expr : Expression? const) : Expression? const {
        //! If `expr` is `int(x)` (single argument, no fake/context args), return x.
        //! `int()` after inference is a generic call — fromGeneric.name is "int".
        //! Peels one ExprRef2Value layer first.
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
        //! Returns true if any module in the compiling program defines `def operator |(a, b : T) : ...`
        //! for the enum represented by `t`. Result is cached on perf019_enum_or_probed / _yes by
        //! intptr(t.enumType) so repeated checks across a large file don't re-walk the function table.
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
        //! True if `T | T` resolves: bitfields always; enums only when an
        //! `operator |` overload exists. Vectors/ints/etc. don't get here —
        //! the caller restricts to int-cast inputs whose underlying type is
        //! either bitfield or enum.
        if (t == null) return false
        if (t.baseType == Type.tBitfield) return true
        if (t.baseType == Type.tEnumeration) return enum_has_or_operator(t)
        return false
    }

    // --- PERF020: redundant same-name workhorse cast helpers ---

    def perf020_target_basetype(fname : string) : Type {
        //! Map a workhorse cast-function name to the Type it produces.
        //! Returns Type.none for names that are not in the workhorse-cast set.
        //! Set: 11 numeric scalars + string + 4 bitfield flavours (15 total).
        //! Source of truth: ADD_NUMERIC_CASTS / ADD_BITFIELD_CASTS in
        //! src/builtin/module_builtin.cpp plus das_lexical_cast_* externs.
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

    // --- PERF024: redundant pre-clone before [clone(...)]-annotated function helpers ---

    def is_clone_init_call(expr : Expression const?) : bool {
        //! True when expr is a single-arg call to one of the AST clone primitives.
        //! Used to seed PERF024 candidates (`var X = clone_*(E)`) and to detect
        //! Type-A direct-splice form (`func(clone_*(X))`).
        if (expr == null || !(expr is ExprCall)) return false
        let c = expr as ExprCall
        if (length(c.arguments) != 1) return false
        let n = string(c.name)
        return (n == "clone_expression" || n == "clone_type"
            || n == "clone_function" || n == "clone_variable"
            || n == "clone_structure")
    }

    def get_clone_param_indices(callee : Function?; var out : array<int>&) : void {
        //! Fills `out` with the param-index list for any [clone(...)] annotations
        //! on callee. Caller-owned out array avoids array return ownership.
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

    // --- PERF025: redundant string(...) inside string interpolation helpers ---

    def perf025_value_kind(arg : Expression const?) : Perf025Kind {
        //! Classifies a string() cast's value argument by how `"{x}"` (the string
        //! builder's own conversion) compares to `string(x)`:
        //!   equivalent   — identical output (drop the cast): signed ints, float,
        //!                  double, string, das_string.
        //!   unsigned_hex — decimal vs hex difference (drop, but suggest the ':d'
        //!                  tag): the four unsigned int widths interpolate as hex.
        //!   skip         — not flaggable: value-shape-changing conversions
        //!                  (array<uint8>, uri, text_range, other handled types)
        //!                  and anything else.
        if (arg == null || arg._type == null) return Perf025Kind.skip
        let bt = arg._type.baseType
        if (bt == Type.tInt || bt == Type.tInt8 || bt == Type.tInt16 || bt == Type.tInt64
            || bt == Type.tFloat || bt == Type.tDouble || bt == Type.tString
            || (bt == Type.tHandle && arg._type.annotation != null
                && arg._type.annotation.name == "das_string")) return Perf025Kind.equivalent
        if (bt == Type.tUInt || bt == Type.tUInt8 || bt == Type.tUInt16 || bt == Type.tUInt64) return Perf025Kind.unsigned_hex
        return Perf025Kind.skip
    }

    def perf025_string_call_value(e : Expression const?) : Expression const? {
        //! If `e` is a `string(value)` cast call, return its value argument
        //! (arguments[0]). Skips an explicit `string(x, true)` hex request — the
        //! second argument being a literal `true` means the user wants hex, which
        //! no longer matches plain interpolation for signed types. The float /
        //! double / string / das_string overloads have no hex param, so their
        //! arguments[1] (a FakeContext) is not an ExprConstBool and passes through.
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
        //! Peels one ExprRef2Value off an interpolation element so a `string(...)`
        //! call read through a value-context wrapper is still exposed. A format
        //! spec (`"{x:fmt}"`) lowers to `_::fmt(":fmt", x)`, but `fmt` has no
        //! string-valued overload, so `string(x)` can never sit under a fmt
        //! wrapper (it would not compile) — no fmt unwrap is needed.
        var ex = elem
        if (ex != null && ex is ExprRef2Value) {
            ex = (ex as ExprRef2Value.subexpr)
        }
        return ex
    }

    // --- function tracking ---

    def override preVisitFunction(var fn : FunctionPtr) : void {
        current_function = fn
        in_template = fn.moreFlags.isTemplate
        // PERF023: reset per-function candidate set.
        perf023_candidates |> clear
        perf023_splice_depth = 0
        // PERF024: reset per-function candidate set.
        perf024_candidates |> clear
        perf024_skip_iptr = 0ul
    }

    def override visitFunction(var fn : FunctionPtr) : FunctionPtr {
        // PERF023: report any candidate whose uses are all splice-safe.
        for (c in perf023_candidates) {
            if (!c.disqualified && c.safe_uses >= 1 && c.v != null) {
                perf_warning(
                    "PERF023: var initialized via clone_expression and only spliced into qmacro/qmacro_block/qmacro_expr/qmacro_block_to_array; the splice already clones (apply_template), so drop the pre-clone and inline the source expression",
                    c.v.at)
            }
        }
        // PERF024: report any candidate whose only uses are annotated-arg-safe.
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
        current_function = null
        in_template = false
        return <- fn
    }

    // --- scope tracking ---

    def override preVisitExprBlock(blk : ExprBlock?) : void {
        if (blk.blockFlags.isClosure) {
            in_closure++
            string_builder_save_stack |> push(string_builder_target_var)
            string_builder_target_var = null
        }
        scope_stack |> push(length(var_stack))
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (blk.blockFlags.isClosure) {
            in_closure--
            if (!empty(string_builder_save_stack)) {
                string_builder_target_var = string_builder_save_stack |> back()
                string_builder_save_stack |> pop()
            }
        }
        if (!empty(scope_stack)) {
            var_stack |> resize(scope_stack |> back())
            scope_stack |> pop()
        }
        return <- blk
    }

    // --- variable declaration tracking ---

    def override preVisitExprLetVariable(expr : ExprLet?; var v : VariablePtr; last : bool) : void {
        if (in_closure == 0) {
            var_stack |> push(VarStackEntry(v = v, depth = loop_depth, is_iter = false))
        }
        // PERF023: seed candidate when init is `clone_expression(...)`.
        // Closure-scoped lets are skipped — `apply_template` rules block IS a
        // closure (the `(var rules : Template)` block argument), and pre-clones
        // declared there have different lifetime semantics than parent-scope
        // ones. We only fire on user-level decls.
        if (in_closure == 0 && v != null && v.init != null && v.init is ExprCall) {
            let initCall = v.init as ExprCall
            if (initCall.name == "clone_expression" && length(initCall.arguments) == 1) {
                perf023_candidates |> push(Perf023Candidate(v = v, safe_uses = 0, disqualified = false))
            }
        }
        // PERF024: seed candidate when init is any clone_* primitive. Broader
        // than PERF023's clone_expression-only seed because the [clone(...)]
        // contract covers all five AST clone primitives. Same closure-gating
        // rationale as PERF023.
        if (in_closure == 0 && v != null && is_clone_init_call(v.init)) {
            perf024_candidates |> push(Perf024Candidate(v = v, safe_uses = 0, disqualified = false))
        }
        // PERF009: track last move-initialized variable (only explicit `var x <- expr`)
        if (last && v.flags.init_via_move && v.init != null) {
            var dominated_by_clone = false
            // Exclude compiler-lowered `:=` (clone_to_move) on non-copyable types
            var ini = v.init
            if (ini is ExprCall) {
                let ecall = ini as ExprCall
                if (ecall.func != null && ecall.func.fromGeneric != null) {
                    dominated_by_clone = ecall.func.fromGeneric.name == "clone_to_move"
                }
            }
            if (!dominated_by_clone) {
                pending_move_var = v
                pending_move_at = expr.at
            }
        }
    }

    // --- PERF009: redundant move-init variable before return ---

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
                        perf_warning("PERF009: redundant move into variable immediately returned; use return <- expr directly", pending_move_at)
                    }
                }
            }
            pending_move_var = null
        }
    }

    // --- loop tracking ---

    def override preVisitExprFor(expr : ExprFor?) : void {
        if (in_closure == 0) {
            scope_stack |> push(length(var_stack))
            loop_depth++
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
        // any source whose length is determined at the start of the loop
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
        if (in_closure == 0 && current_for_known_length) {
            known_length_loop_depth++
        }
        if (in_closure == 0) {
            perf018_finalize_on_body()
            perf022_inspect_body(expr.body)
        }
    }

    def override preVisitExprForVariable(expr : ExprFor?; var v : VariablePtr; last : bool) : void {
        if (in_closure == 0) {
            var_stack |> push(VarStackEntry(v = v, depth = loop_depth - 1, is_iter = true))
            perf018_record_var(v)
            perf022_record_var(v)
        }
    }

    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        if (in_closure == 0) {
            perf018_pop_and_check()
            perf022_pop_and_check()
            loop_depth--
            if (current_for_known_length) {
                known_length_loop_depth--
            }
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
        return <- expr
    }

    // --- PERF018: for (i in range(length(arr))) where i only indexes arr ---

    def perf018_push_sentinel(expr : ExprFor?) : void {
        //! Always pushed in preVisitExprFor — keeps the stack in lockstep with
        //! for-loop nesting so visitExprFor can pop unconditionally. Matched/
        //! disqualified status is filled in by the per-source/per-variable hooks.
        var state = Perf018State(disqualified = true, matched = false)
        perf018_stack    |> push(state)
        unsafe {
            perf018_for_exprs |> push(reinterpret<ExprFor?>(expr))
        }
    }

    def perf018_pop_and_check() : void {
        if (empty(perf018_stack)) return
        let top_idx = length(perf018_stack) - 1
        let state = perf018_stack[top_idx]
        let owning = perf018_for_exprs[top_idx]
        perf018_stack    |> pop()
        perf018_for_exprs |> pop()
        if (state.matched && !state.disqualified && state.qualified_count > 0 && owning != null) {
            perf_warning("PERF018: for (i in range(length(X))) where i only indexes X; use 'for (c in X)' for direct iteration", owning.at)
        }
    }

    def perf018_bare_var_of(var e : ExpressionPtr) : Variable? {
        //! Returns the underlying Variable if `e` (after one optional Ref2Value
        //! peel) is a bare ExprVar. Null otherwise — field paths, calls, etc. all
        //! intentionally disqualify PERF018's arr resolution.
        var inner = e
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value.subexpr)
        }
        return null if (inner == null || !(inner is ExprVar))
        var evar = inner as ExprVar
        return evar.variable
    }

    def perf018_record_source(src : ExpressionPtr) : void {
        //! Called from preVisitExprForSource. Peels `range(length(<bareVar>))`
        //! (with one optional ExprCast on either layer) and, when matched, stamps
        //! the candidate's arr_var. Iter var arrives via perf018_record_var.
        if (empty(perf018_stack)) return
        let top_idx = length(perf018_stack) - 1
        var state = perf018_stack[top_idx]
        state.sources_seen++
        if (state.sources_seen > 1) {
            // Multi-source for loops can't be turned into a single for-each.
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
        if (rng.func == null || rng.func.name != "range"
            || length(rng.arguments) != 1) {
            perf018_stack[top_idx] = state
            return
        }
        var inner = rng.arguments[0]
        if (inner is ExprCast) {
            inner = (inner as ExprCast.subexpr)
        }
        if (inner == null || !(inner is ExprCall)) {
            perf018_stack[top_idx] = state
            return
        }
        var len = inner as ExprCall
        if (len.func == null || len.func.name != "length"
            || length(len.arguments) != 1) {
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
        //! When a single iter var pairs with a single recognized source, marks
        //! the candidate matched + un-disqualified.
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
        //! Called from preVisitExprForBody — fires once per for, after every
        //! preVisitExprForSource and preVisitExprForVariable. Flips matched=true
        //! iff the loop had exactly one source we recognized (arr_var set) and
        //! exactly one iter var (iter_var set).
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
        //! True when `at.subexpr[at.index]` accesses the frame's arr_var (bare)
        //! and indexes by the frame's iter_var with no arithmetic. Conservative
        //! by design — field paths, casts on the subexpr, etc. don't qualify.
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

    def perf018_on_at(var expr : ExprAt?) : void {
        var pushes = 0
        let n = length(perf018_stack)
        for (i in range(n)) {
            if (perf018_at_matches(expr, i)) {
                var st = perf018_stack[i]
                st.qualified_count++
                perf018_stack[i] = st
                pushes++
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
        //! Called from preVisitExprVar: a bare iter_var reference outside any
        //! qualifying ExprAt index slot disqualifies the matching frame.
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

    // --- PERF022: for (s in A) { B |> push(s) } — recommend bulk *_from ---

    def perf022_push_sentinel(expr : ExprFor?) : void {
        //! Always pushed in preVisitExprFor so the stack stays lock-step with
        //! for-loop nesting. preVisitExprForBody may flip `matched`; if a
        //! frame survives to perf022_pop_and_check matched-and-undisqualified,
        //! we fire at the for-loop's `at` (not the push's, to avoid colliding
        //! with PERF006 under the same-location dedup in perf_warning).
        var state : Perf022State
        perf022_stack |> push(state)
        unsafe {
            perf022_for_exprs |> push(reinterpret<ExprFor?>(expr))
        }
    }

    def perf022_record_source(src : Expression?) : void {
        if (empty(perf022_stack)) return
        let top_idx = length(perf022_stack) - 1
        var state = perf022_stack[top_idx]
        state.sources_seen++
        if (state.sources_seen > 1) {
            state.disqualified = true
        }
        // PERF022 only suggests push_from / push_clone_from, which take array<T>
        // or C-array sources. Disqualify range/string/iterator/generator sources
        // — there is no bulk overload to migrate to in those cases.
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
        //! Called from preVisitExprForBody. Flips `matched` iff the body is
        //! a single-statement ExprBlock containing exactly one ExprCall to
        //! push/push_clone with arg[1] = the frame's iter_var. Compiler folds
        //! 'B |> push(s)', 'B.push(s)', and 'push(B, s)' to the same shape,
        //! so this single check covers all three call forms.
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
        // Destination must be `array<T>` — the `*_from` overloads are array-only.
        // Generic `auto` parameters (e.g. `[expect_any_vector] def clone(var args;
        // ...)` in builtin.das) where the destination accepts dasvector etc. must
        // be skipped — no push_from overload exists for them.
        let dest = call.arguments[0]
        if (dest == null || dest._type == null || dest._type.baseType != Type.tArray) return
        var callee_id = 0
        if (call.func.fromGeneric != null) {
            if (call.func.fromGeneric.name == "push") {
                callee_id = 1
            } elif (call.func.fromGeneric.name == "push_clone") {
                callee_id = 2
            }
        } else {
            if (call.func.name == "push") {
                callee_id = 1
            } elif (call.func.name == "push_clone") {
                callee_id = 2
            }
        }
        if (callee_id == 0) return
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

    def perf022_pop_and_check() : void {
        if (empty(perf022_stack)) return
        let top_idx = length(perf022_stack) - 1
        let state = perf022_stack[top_idx]
        let owning = perf022_for_exprs[top_idx]
        perf022_stack    |> pop()
        perf022_for_exprs |> pop()
        if (state.matched && !state.disqualified && owning != null) {
            let name = state.callee_id == 1 ? "push" : "push_clone"
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
    }

    def override preVisitExprWhileBody(expr : ExprWhile?; body : ExpressionPtr) : void {
        in_while_cond = false
    }

    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        if (in_closure == 0) {
            loop_depth--
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
        in_while_cond = false
        return <- expr
    }

    // --- break/continue tracking (for PERF006 early-exit suppression) ---

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

    // --- if-depth tracking (for PERF006 conditional suppression) ---

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

    // --- PERF001: string += in loop ---
    // --- PERF007: unnecessary string(das_string) in comparison ---
    // --- PERF008: unnecessary get_ptr() for is/as ---

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        // No in_closure check needed — loop_depth doesn't increment inside
        // closure bodies (see preVisitExprFor/While), so PERF001's loop check
        // is already correctly scoped. The other rules below are syntactic
        // patterns that misbehave identically inside or outside closures.
        if (loop_depth > 0 && expr.op == "+=") {
            let v = find_string_var_from_expr(expr.left)
            if (v != null && is_defined_outside_loop(v)) {
                perf_warning("PERF001: string += in loop creates O(n^2) allocations; use build_string() instead", expr.at)
            }
        }
        // PERF007: string(das_string) == or != comparison
        if (expr.op == "==" || expr.op == "!=") {
            let left_is = is_string_of_das_string(expr.left)
            let right_is = is_string_of_das_string(expr.right)
            if (left_is || right_is) {
                perf_warning("PERF007: string(das_string) in comparison is unnecessary; das_string supports == and != directly", expr.at)
            }
        }
        // PERF008: x.__rtti == "Type" is expanded from `x is Type`
        if (expr.op == "==") {
            var left = expr.left
            // Unwrap ExprRef2Value — compiler inserts these for value-type reads
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
        // PERF010: x == null or x != null
        if (is_get_ptr_vs_null(expr.left, expr.right) || is_get_ptr_vs_null(expr.right, expr.left)) {
            perf_warning("PERF010: get_ptr() is unnecessary for null comparison; smart_ptr supports == and != null directly", expr.at)
        }
        // PERF013: a += ±1 / a -= ±1 → a++ / a--
        // Derive the actual delta from (op, RHS) so each branch can suggest the
        // correct postfix operator. RHS is one of {+1, -1}; op flips the sign.
        if (expr.op == "+=" || expr.op == "-=") {
            if (expr.left != null && is_workhorse_numeric(expr.left._type)) {
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
        }
        // PERF014: exact char-class checks. Digit range is a single bounded range
        // (closed joins with &&, negated complement with ||). Alpha is the both-case
        // compound (closed = || of two ranges, negated = && of two complements).
        if (expr.op == "&&") {
            check_perf014_digit(expr, false)
            check_perf014_alpha(expr, true)
        }
        if (expr.op == "||") {
            check_perf014_digit(expr, true)
            check_perf014_alpha(expr, false)
        }
        // PERF017: length(s) <op> 0 / 1 → empty(s) / !empty(s)
        if (expr.op == "==" || expr.op == "!=" || expr.op == "<" || expr.op == "<=" || expr.op == ">" || expr.op == ">=") {
            check_perf017_length_zero(expr)
        }
        // PERF019: int(T.a) | int(T.b) collapses to int(T.a | T.b) when T natively
        // supports `|` (bitfield always; enum only when an `operator |` overload exists).
        if (expr.op == "|") {
            check_perf019_int_cast_or(expr)
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

    // --- PERF020: T(x) where x is already T — redundant workhorse cast ---

    def check_perf020_redundant_cast(call : ExprCall?) : void {
        if (call == null || call.func == null || length(call.arguments) != 1) return
        let fname = string(call.func.fromGeneric != null ? call.func.fromGeneric.name : call.func.name)
        let target = perf020_target_basetype(fname)
        if (target == Type.none) return
        var arg = call.arguments[0]
        if (arg is ExprRef2Value) {
            arg = (arg as ExprRef2Value.subexpr)
        }
        if (arg == null || arg._type == null || arg._type.baseType != target) return
        perf_warning("PERF020: redundant {fname}(...) cast — argument is already {fname}", call.at)
    }

    // --- PERF021: cond ? T(a) : T(b) — hoist common workhorse cast out of ternary ---

    def cast_call_workhorse_target(e : Expression?; var fname_out : string&) : Type {
        //! If `e` is a workhorse cast call (after peeling ExprRef2Value), return
        //! its target Type and write the cast name to `fname_out`. Otherwise
        //! return Type.none. Accepts any argument count — `string(int)` is bound
        //! with `value,hex,context,at` (4 daslang args), other casts use 1.
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
        //! Peel ExprRef2Value, take call.arguments[0], peel ExprRef2Value, return its baseType
        //! (or Type.none if anything along the chain is null).
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
        //! Compare arguments[1..] structurally between two cast calls. Skip
        //! ExprFakeContext / ExprFakeLineInfo (auto-injected by the typer for
        //! Context*/LineInfoArg* parameters; differ at every call site by
        //! design). The remaining tail args — e.g. `string(int)`'s `hex` flag —
        //! must match in count and via expr_equal_struct, otherwise the hoist
        //! would silently lose semantically-significant arguments.
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
        // Tail-args must match so the hoisted form preserves semantically-significant
        // arguments (e.g. `string(int, hex)`'s `hex` flag). Auto-injected
        // ExprFakeContext / ExprFakeLineInfo are skipped — they differ at every site.
        if (lt == Type.none || rt == Type.none || lt != rt || lname != rname
            || la == Type.none || la != ra
            || !cast_call_tail_args_equal(expr.left, expr.right)) return
        perf_warning("PERF021: redundant per-branch {lname}(...) cast in ternary — hoist as {lname}(cond ? a : b)", expr.at)
    }

    // --- PERF014: exact char-class checks — is_number (digit range) / is_alpha (both-case compound) ---

    def parse_range_leg(leg : Expression?; upper_op, lower_op : string; var v_out : Expression?&; var bound_out : int&; var is_hi_out : bool&) : bool {
        //! Parse `var <op> const` or `const <op> var` where op is `upper_op` or
        //! `lower_op`. Returns true and fills outputs with: the variable
        //! expression, the int bound, and whether this leg expresses the upper
        //! bound of the range on var. `upper_op` is the var-on-left operator that
        //! marks the upper-bound leg ("<=" closed-in-range, ">" open-out-of-range);
        //! `lower_op` the lower-bound one (">=" / "<"). Yoda (const-on-left) forms
        //! flip, so the const-on-left arm tests against `lower_op`.
        if (leg == null || !(leg is ExprOp2)) return false
        var op2 = leg as ExprOp2
        let opname = string(op2.op)
        if (opname != upper_op && opname != lower_op) return false
        if (op2.right is ExprConstInt && op2.left != null) {
            v_out = op2.left
            bound_out = (op2.right as ExprConstInt).value
            is_hi_out = opname == upper_op
            return true
        }
        if (op2.left is ExprConstInt && op2.right != null) {
            v_out = op2.right
            bound_out = (op2.left as ExprConstInt).value
            is_hi_out = opname == lower_op
            return true
        }
        return false
    }

    def parse_bounded_range(expr : ExprOp2?; negated : bool; var v_out : Expression?&; var lo_out : int&; var hi_out : int&) : bool {
        //! Parse a single bounded range on one variable from a two-leg ExprOp2.
        //! Closed (`negated == false`): `c >= lo && c <= hi` (join `&&`, legs <=/>=).
        //! Open   (`negated == true`):  `c < lo || c > hi`   (join `||`, legs </>).
        //! Both legs must constrain the SAME (pure) expression — one upper, one
        //! lower bound. Fills v_out / lo_out / hi_out; returns true on clean parse.
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
        //! Exact: `c >= '0' && c <= '9'` → is_number(c); the negated De Morgan
        //! complement `c < '0' || c > '9'` → !is_number(c).
        var v : Expression? = null
        var lo = 0
        var hi = 0
        if (!parse_bounded_range(expr, negated, v, lo, hi) || lo != 48 || hi != 57) return
        let neg = negated ? "!" : ""
        perf_warning("PERF014: char-class range check{negated ? " (negated)" : ""} '0'..'9' — use '{neg}is_number({describe(v)})' from strings module", expr.at)
    }

    def check_perf014_alpha(expr : ExprOp2?; negated : bool) : void {
        //! Exact: `('a'..'z') || ('A'..'Z')` → is_alpha(c). Its De Morgan negation
        //! `(c < 'a' || c > 'z') && (c < 'A' || c > 'Z')` → !is_alpha(c). is_alpha
        //! is DEFINED as exactly this both-case union, so a single-case range is
        //! intentionally NOT flagged (no equivalent helper). The top-level join is
        //! `||` for the closed form / `&&` for the negated form; each child is a
        //! bounded range of the matching polarity.
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
        // One half must be 'a'..'z' (97..122), the other 'A'..'Z' (65..90).
        let lowerUpper = loL == 97 && hiL == 122 && loR == 65 && hiR == 90
        let upperLower = loL == 65 && hiL == 90 && loR == 97 && hiR == 122
        if (!lowerUpper && !upperLower) return
        let neg = negated ? "!" : ""
        perf_warning("PERF014: char-class range check{negated ? " (negated)" : ""} 'a'..'z' + 'A'..'Z' — use '{neg}is_alpha({describe(vL)})' from strings module", expr.at)
    }

    // --- PERF017: length(collection) <op> 0/1 → empty / !empty ---

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
        // Skip the canonical implementation of empty() in daslib/builtin.das —
        // empty(x) is literally `return length(x) == 0`, and the rule's whole job
        // is to suggest `empty(x)`. Self-flagging the suggestion's own body is
        // noise. Generic instantiations of empty<T> hit this path with
        // current_function.fromGeneric pointing at the source `empty`. Both arms
        // also gate on _module.name == "builtin" so unrelated user methods named
        // empty() (e.g. delegate.empty(), AOT cache empty()) keep getting linted.
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
        // Canonicalize so that length is on the left; flip op if needed.
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
        // Map (canon_op, k) → empty / !empty
        //   k=0: ==, <=             → empty
        //        !=, >              → !empty
        //   k=1: <                  → empty   (length < 1 means length == 0)
        //        >=                 → !empty  (length >= 1 means length > 0)
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
        // Diagnostic uses canon_op so Yoda forms (e.g. `1 <= length(x)`) print
        // the canonical equivalent (`length(x) >= 1`), matching what we matched.
        if (is_empty_pattern) {
            perf_warning("PERF017: 'length(x) {canon_op} {k}' — use 'empty(x)' (avoids unnecessary strlen on strings)", expr.at)
        } elif (is_nonempty_pattern) {
            perf_warning("PERF017: 'length(x) {canon_op} {k}' — use '!empty(x)' (avoids unnecessary strlen on strings)", expr.at)
        }
    }

    // --- PERF015 / PERF016: ternary min/max/abs ---

    def override preVisitExprOp3(expr : ExprOp3?) : void {
        // PERF021: fires anywhere, including in closures — a redundant per-branch cast
        // is redundant regardless of where the ternary lives.
        check_perf021_ternary_cast_hoist(expr)
        if (in_closure > 0 || expr.op != "?"
            || expr.subexpr == null || !(expr.subexpr is ExprOp2)) return
        var cmp = expr.subexpr as ExprOp2
        let cop = string(cmp.op)
        if ((cop != "<" && cop != "<=" && cop != ">" && cop != ">=")
            || expr.left == null || expr.right == null) return
        let t_branch = expr.left
        let f_branch = expr.right
        // PERF016 first — a ternary that matches abs may also superficially match
        // min/max if x and -x ever describe-equal, but they can't. So order is fine.
        if (try_perf016_abs(expr, cmp, t_branch, f_branch, cop)) return
        try_perf015_minmax(expr, cmp, t_branch, f_branch, cop)
    }

    def try_perf015_minmax(expr : ExprOp3?; cmp : ExprOp2?; t_branch, f_branch : Expression? const; cop : string) : bool {
        let cl = cmp.left
        let cr = cmp.right
        if (cl == null || cr == null) return false
        // ternary branches must be a permutation of (cl, cr)
        let tcl = expr_equal_struct(t_branch, cl)
        let tcr = expr_equal_struct(t_branch, cr)
        let fcl = expr_equal_struct(f_branch, cl)
        let fcr = expr_equal_struct(f_branch, cr)
        let same_order = tcl && fcr   // T==L, F==R
        let swap_order = tcr && fcl   // T==R, F==L
        if (!same_order && !swap_order) return false
        // Mapping
        //   <  / <=  : T==L,F==R → min;  T==R,F==L → max
        //   >  / >=  : T==L,F==R → max;  T==R,F==L → min
        var is_min = false
        if (cop == "<" || cop == "<=") {
            is_min = same_order
        } else {
            is_min = swap_order
        }
        let which = is_min ? "min" : "max"
        perf_warning("PERF015: ternary {which} — use '{which}(a, b)' from math module", expr.at)
        return true
    }

    def try_perf016_abs(expr : ExprOp3?; cmp : ExprOp2?; t_branch, f_branch : Expression? const; cop : string) : bool {
        // One side of the comparison must be const-zero
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
        // Identify negated branch (-x) and plain branch (x)
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
        // Determine canonical sign of x in comparison: x < 0 or x > 0 etc.
        // Reduce to: "is x's value-relation `negative` or `positive`" in the true branch
        // Forms that produce abs:
        //   x < 0 → -x  (T branch is -x, x is on left)        — neg_then=true , x_on_left=true , cop in {<, <=}
        //   x > 0 → x   (T branch is x,  x is on left)        — neg_then=false, x_on_left=true , cop in {>, >=}
        //   0 > x → -x                                         — neg_then=true , x_on_left=false, cop in {>, >=}
        //   0 < x → x                                          — neg_then=false, x_on_left=false, cop in {<, <=}
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
        perf_warning("PERF016: ternary abs — use 'abs(x)' from math module", expr.at)
        return true
    }

    def is_neg_of(maybe_neg : Expression? const; ref : Expression? const) : bool {
        //! True when `maybe_neg` is `-ref` structurally.
        if (maybe_neg == null || !(maybe_neg is ExprOp1)) return false
        let op1 = maybe_neg as ExprOp1
        if (op1.op != "-") return false
        return expr_equal_struct(op1.subexpr, ref)
    }

    // --- PERF011: unnecessary get_ptr() for field access ---

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

    // --- PERF024: detect args at [clone(...)]-annotated positions ---

    def override preVisitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; last : bool) : void {
        // Fires once per argument of an ExprCall, in declaration order, BEFORE
        // the visitor descends into the arg subtree. Used by PERF024:
        //   Arm A (direct splice): if arg is clone_*(X) at an annotated index → flag.
        //   Arm B (var-init-only-clone-use): if arg is a bare ExprVar matching a
        //     candidate, at an annotated index → mark as safe_use and set
        //     perf024_skip_var so the imminent preVisitExprVar doesn't
        //     disqualify the candidate.
        if (expr == null || expr.func == null) return
        var indices : array<int>
        get_clone_param_indices(expr.func, indices)
        if (empty(indices)) return
        // Compute this arg's index by identity-matching against expr.arguments.
        var arg_index = -1
        for (i in 0 .. length(expr.arguments)) {
            if (intptr(expr.arguments[i]) == intptr(arg)) {
                arg_index = i
                break
            }
        }
        if (arg_index < 0 || !has_value(indices, arg_index)) return
        // Peel typer-inserted ExprRef2Value wrapper — when the var has reference
        // type (e.g. `var X = clone_expression(...)`), the typer inserts a
        // dereference wrapper around uses passed to non-ref params.
        var inner = arg
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value).subexpr
        }
        // Arm A: inner is itself a clone_* call → redundant.
        if (is_clone_init_call(inner)) {
            perf_warning(
                "PERF024: argument to function annotated [clone({expr.func.arguments[arg_index].name})] is wrapped in clone_*; the callee clones internally, drop the outer clone wrap",
                arg.at)
            return
        }
        // Arm B: inner is a bare ExprVar matching a candidate.
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

    // --- PERF002 + PERF003: character_at (counter-based) ---

    def override preVisitExprCall(var expr : ExprCall?) : void {
        if (expr.func == null) return
        // PERF020: redundant same-type cast — fires anywhere, including in closures
        check_perf020_redundant_cast(expr)
        // PERF023: track splice-wrapper depth. apply_qrules emits `add_ptr_ref(...)`
        // around every `$e(...)` tag; matching the function NAME is sufficient
        // because the templates_boost wrapper is the only producer of that name
        // and it sits at every $e splice site post-expansion. Must fire BEFORE
        // the in_closure gate — the rules block is itself a closure.
        if (expr.name == "add_ptr_ref" && !empty(perf023_candidates)) {
            perf023_splice_depth++
        }
        // Loop/while-aware rules below skip closure bodies (they run later, not in the loop)
        if (in_closure > 0) return
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = expr
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

    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (expr.func == null) return <- expr
        // PERF023: pop splice-wrapper depth when leaving an add_ptr_ref call.
        // Symmetric with the preVisitExprCall push above; fires regardless of
        // closure depth.
        if (expr.name == "add_ptr_ref" && perf023_splice_depth > 0) {
            perf023_splice_depth--
        }
        // PERF003: character_at anywhere — fires even inside closures
        if (expr.func.name == "character_at" && expr.func._module.name == "strings") {
            var ecall = expr
            if (in_closure == 0 && loop_depth > 0 && in_character_at_call > 0) {
                in_character_at_call--
                current_character_at = null
            }
            // deferred so PERF002 can claim it first
            if (!reported_character_at |> has_value(ecall)) {
                if (is_character_at_zero(expr)) {
                    perf_warning("PERF003: character_at(s, 0) can be replaced with first_character(s)", expr.at)
                } else {
                    perf_warning("PERF003: character_at is O(n) due to strlen; consider peek_data() for hot paths", expr.at)
                }
            }
        }
        // PERF006: track reserve() calls and detect push/emplace without reserve
        if (is_array_func(expr, "reserve")) {
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
                if (rv != null && is_defined_outside_loop(rv) && !reserved_paths |> has_value(make_path_key(rv, path))) {
                    let fname = string(expr.func.fromGeneric.name)
                    perf_warning("PERF006: {fname} in loop without prior reserve() may cause repeated reallocations; consider reserve() before the loop", expr.at)
                }
            }
        }
        // PERF012: string(das_string) passed to strings module function
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

    // --- PERF004: string builder reassignment in loop (counter-based) ---

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
        if (in_closure > 0 || loop_depth == 0) return
        var v = find_string_var_from_expr(expr.left)
        if (v != null && is_defined_outside_loop(v) && expr.right is ExprStringBuilder) {
            string_builder_target_var = v
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

    // --- PERF025: redundant string(...) inside string interpolation ---

    def override preVisitExprStringBuilderElement(sb : ExprStringBuilder?; elem : ExpressionPtr; last : bool) {
        let inner = perf025_unwrap_element(elem)
        let value = perf025_string_call_value(inner)
        return if (value == null)
        let kind = perf025_value_kind(value)
        if (kind == Perf025Kind.equivalent) {
            perf_warning("PERF025: string(...) inside string interpolation is redundant — the interpolation already converts to string; drop the cast", inner.at)
        } elif (kind == Perf025Kind.unsigned_hex) {
            perf_warning("PERF025: string(...) inside string interpolation is redundant; drop the cast — unsigned values interpolate as hex by default, so use the '\{x:d\}' format tag to keep decimal output", inner.at)
        }
    }

    // --- Central ExprVar handler (PERF002, PERF004, PERF005) ---

    def override preVisitExprVar(expr : ExprVar?) : void {
        // PERF023: classify this var-ref as safe (inside add_ptr_ref) or
        // disqualifying. Runs before the in_closure gate because the splice
        // site lives inside the rules-block closure.
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
        // PERF024: classify this var-ref as safe (pre-classified in
        // preVisitExprCallArgument as a direct annotated-arg) or disqualifying.
        // perf024_skip_iptr is set just-in-time by the call-arg hook and
        // consumed here. Same closure-skip rationale as PERF023.
        if (!empty(perf024_candidates)) {
            let vv4 = expr.variable
            if (vv4 != null) {
                let vv4_iptr = intptr(vv4)
                if (perf024_skip_iptr == vv4_iptr && vv4_iptr != 0ul) {
                    perf024_skip_iptr = 0ul   // already classified safe above
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
        // PERF018: a bare iter_var reference outside a qualifying ExprAt index slot disqualifies the loop.
        perf018_on_var(v)
        // PERF002: loop variable inside character_at call
        if (in_character_at_call > 0 && is_loop_variable(v)) {
            if (current_character_at != null && !reported_character_at |> has_value(current_character_at)) {
                perf_warning("PERF002: character_at(s, i) in loop is O(n) per call (strlen + bounds check each time); use peek_data(s) to access characters as array<uint8>", expr.at)
                reported_character_at |> push(current_character_at)
            }
        }
        // PERF004: target var inside string builder
        if (in_string_builder_check > 0 && string_builder_target_var != null && v == string_builder_target_var) {
            perf_warning("PERF004: rebuilding string via interpolation in loop creates O(n^2) allocations; use build_string() instead", expr.at)
        }
        // PERF005: string var inside length() in while condition
        if (in_length_while_call > 0 && v != null) {
            if (expr.variable._type != null && expr.variable._type.baseType == Type.tString) {
                if (!expr.variable.access_flags.access_ref) {
                    perf_warning("PERF005: length(string) in while condition recomputes strlen each iteration; cache in a local variable", expr.at)
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
    let count = astVisitor.warning_count
    warnings |> push_from(astVisitor.warnings)
    unsafe { delete astVisitor; }
    return count
}

// ---------------------------------------------------------------------------
// Lint macro (auto-runs when module is required)
// ---------------------------------------------------------------------------

[lint_macro]
class PerfLintMacro : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        let disabled <- build_lint_macro_disabled(prog)
        perf_lint(prog, true, disabled)
        return true
    }
}
