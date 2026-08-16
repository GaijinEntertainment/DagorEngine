options gen2
options indenting = 4
options strict_smart_pointers = true

module lint shared private

//! Lint pass that enables paranoid compilation checks.
//!
//! When this module is required, the ``paranoid`` lint flag is turned on
//! for the current module, enabling stricter compiler diagnostics.
//! See also ``lint_everything`` for applying lint to all modules.

require daslib/strings_boost
require daslib/ast_boost
require daslib/strings_boost
require daslib/lint_config

[lint_macro]
class LintEverything : AstPassMacro {
    //! Pass macro that enables lint checking for modules.
    def override apply(prog : ProgramPtr; _mod : Module?) : bool {
        let disabled <- build_lint_macro_disabled(prog)
        paranoid(prog, true, disabled)
        return true
    }
}

struct private DeadStoreEntry {
    @do_not_delete v : Variable const?
    @do_not_delete pending_expr : Expression const?
    skip : bool
    has_pending : bool
    pending_pure : bool
}

struct private Lint021Candidate {
    @do_not_delete v : Variable const?
    safe_uses : int
    disqualified : bool
}

def private is_block_terminator(expr : ExpressionPtr) : bool {
    // A direct-child block statement that always exits the enclosing function.
    if (expr is ExprReturn) return true
    if (expr is ExprCall) {
        let call = expr as ExprCall
        if (call.func == null) return false
        return call.name |> eq <| "panic" && call.func._module.name |> eq <| "builtin"
    }
    return false
}

class LintVisitor : AstVisitor {
    // LINT014 is decided after the body: a `var` argument that is returned needs its mutability
    // for the move (a non-copyable result cannot move from a const argument). Arguments are
    // collected before the body, so a `return <arg>` just removes its candidate
    lint014_candidates : array<Variable const?>
    //! AST visitor that performs lint checks.
    astVisitorAdapter : VisitorAdapter?
    exprForTerminator : array<uint64>
    compile_time_errors : bool
    noLint : bool = false
    genericDepth : int = 0
    multiple_contexts : bool = false
    // collection mode — when true, errors are appended to `errors` instead of error()
    collect_errors : bool = false
    errors : array<string>
    issues : array<LintIssue>       // structured twin of `errors`, same entries in the same order
    // Filters: disabled (denylist) and enabled (whitelist; empty == all).
    // Caller-populated via collect overload; applied alongside // nolint suppression.
    disabled_codes : table<string>
    enabled_codes : table<string>
    // LINT010 state — dead-store detection. Per-local tracking; one entry per
    // analyzed local in the current function. Reset on function entry.
    branch_depth : int = 0
    ds_entries : array<DeadStoreEntry>
    funcFile : string       // current function's source file, anchors lint_error's foreign-lineinfo gate
    // LINT021 state — 64-bit locals never consumed as 64-bit. Per-ExprCall kind stack keeps
    // narrow/poison depths correct across nesting; closure depth disqualifies captured uses.
    lint021_candidates : array<Lint021Candidate>
    lint021_call_kind : array<int>      // 0 = plain, 1 = 64-bit narrowing cast, 2 = poison
    lint021_narrow_depth : int = 0
    lint021_poison_depth : int = 0
    lint021_closure_depth : int = 0
    def LintVisitor() {
        pass
    }
    def is_suppressed(text : string; at : LineInfo) : bool {
        let code = extract_lint_code(text)
        if (!empty(code)) {
            return true if (key_exists(disabled_codes, code)
                            || (!empty(enabled_codes) && !key_exists(enabled_codes, code)))
        }
        return is_lint_suppressed(at, code)
    }
    def lint_error(text : string; at : LineInfo) : void {
        // a node carrying another file's lineinfo (an inliner splice from a required module, macro-manufactured code) gets its diagnostics from that file's own compile — reporting it here would fail modules on idioms they cannot edit
        if (noLint || is_suppressed(text, at)
                || (at.fileInfo != null && !empty(funcFile) && at.fileInfo.name != funcFile)) return
        if (compile_time_errors) {
            compiling_program() |> macro_error(at, text)
        } elif (collect_errors) {
            errors |> push("{text} at {describe(at)}")
            issues |> push(make_lint_issue(text, at))
        } else {
            error("{text} at {describe(at)}\n")
        }
    }
    def private source_operator_is_clone(at : LineInfo) : bool {
        //! Inference lowers a string ExprClone to ExprCopy in ordinary mode, but preserves the clone operator's LineInfo. Check that source token so a regular '=' copy stays silent.
        if (at.fileInfo == null || at.line == 0u || at.line != at.last_line) return false
        var found = false
        get_file_source_line(at.fileInfo, at.line) $(line) {
            var start = int(at.column) - 1
            if (start < 0) {
                start = 0
            }
            var finish = int(at.last_column) + 1
            if (finish < start + 3) {
                finish = start + 3
            }
            if (finish > length(line)) {
                finish = length(line)
            }
            if (finish > start) {
                found = find(slice(line, start, finish), ":=") >= 0
            }
        }
        return found
    }
    def private root_generic(fn : Function const?) : Function const? {
        var root = fn
        while (root != null && root.fromGeneric != null) {
            root = root.fromGeneric
        }
        return root
    }
    def private is_temporary_string_source(expr : Expression?) : bool {
        if (expr == null) return false
        if (expr._type != null && expr._type.flags.temporary) return true
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value).subexpr
        }
        if (inner is ExprVar) {
            let variable = (inner as ExprVar).variable
            return variable != null && variable._type != null && variable._type.flags.temporary
        }
        return false
    }
    def private is_inferred_temp_string_clone(expr : Expression?; clone_at : LineInfo) : bool {
        //! InferTypes rewrites `dst := string#_expr` to `dst = clone_string(string#_expr)` and gives the injected call the original := token location. A source-written clone_string call keeps its own location.
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        return (call.func != null && call.func.name == "clone_string"
            && call.at.line == clone_at.line && call.at.column == clone_at.column)
    }
    def private is_inferred_temp_string_init(expr : Expression?; init_at : LineInfo) : bool {
        //! InferTypes rewrites `var dst := string#_expr` through clone_string. The wrapper result is a regular string, so inspect its source argument to preserve the temporary-string exception.
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        let root = root_generic(call.func)
        return (root != null && root.name == "clone_string"
            && root._module != null && root._module.name == "builtin"
            && call.at.line == init_at.line && call.at.column == init_at.column
            && !empty(call.arguments) && is_temporary_string_source(call.arguments[0]))
    }
    def private source_variable_init_is_clone(v : VariablePtr) : bool {
        //! String clone initialization is lowered before lint runs. Search only between the source variable name and its initializer, so another declaration or := inside the RHS cannot produce a false positive.
        if (v == null || v.init == null || v.at.fileInfo == null || v.at.line == 0u) return false
        var found = false
        get_file_source_line(v.at.fileInfo, v.at.line) $(line) {
            var start = int(v.at.column) - 1
            if (start < 0) {
                start = 0
            }
            var finish = length(line)
            if (v.init.at.line == v.at.line && int(v.init.at.column) > start) {
                finish = int(v.init.at.column) + 1
                if (finish > length(line)) {
                    finish = length(line)
                }
            }
            if (finish > start) {
                found = find(slice(line, start, finish), ":=") >= 0
            }
        }
        return found
    }
    def private check_string_clone_init(v : VariablePtr) : void {
        if (v == null || multiple_contexts || noLint || genericDepth > 0
                || v.flags.generated || v._type == null || v.init == null
                || v._type.baseType != Type.tString
                || is_temporary_string_source(v.init)
                || is_inferred_temp_string_init(v.init, v.at)
                || !source_variable_init_is_clone(v)) return
        lint_error("LINT016: ':=' does not clone a string variable without 'options multiple_contexts'; use '=' for same-context storage, or did you mean '= clone_string(value)' for a cross-context copy", v.at)
    }
    def private check_string_clone_assign(expr : ExprCopy?) : void {
        if (expr == null || multiple_contexts || noLint || genericDepth > 0 || expr.genFlags.generated
                || expr.left == null || expr.right == null
                || expr.left._type == null || expr.right._type == null
                || expr.left._type.baseType != Type.tString
                || expr.right._type.baseType != Type.tString
                || is_temporary_string_source(expr.right)
                || is_inferred_temp_string_clone(expr.right, expr.at)
                || !source_operator_is_clone(expr.at)) return
        lint_error("LINT016: ':=' does not clone a string without 'options multiple_contexts'; use '=' for same-context storage, or did you mean 'clone_string(value)' for a cross-context copy", expr.at)
    }
    def private check_string_push_clone(expr : ExprCall?) : void {
        if (expr == null || multiple_contexts || noLint || genericDepth > 0 || expr.genFlags.generated
                || expr.func == null || length(expr.arguments) < 2) return
        let root = root_generic(expr.func)
        if (root == null || root.name != "push_clone"
                || root._module == null || root._module.name != "builtin") return
        let dst = expr.arguments[0]
        let value = expr.arguments[1]
        if (dst == null || value == null || dst._type == null || value._type == null
                || dst._type.baseType != Type.tArray || dst._type.firstType == null
                || dst._type.firstType.baseType != Type.tString
                || value._type.baseType != Type.tString
                || is_temporary_string_source(value)) return
        lint_error("LINT016: 'push_clone' does not clone a string without 'options multiple_contexts'; use 'push(value)' for same-context storage, or did you mean 'push(clone_string(value))' for a cross-context copy", expr.at)
    }
    def override preVisitFunction(fun : FunctionPtr) : void {
        noLint = false
        funcFile = fun.at.fileInfo != null ? "{fun.at.fileInfo.name}" : ""
        if (fun.fromGeneric != null || fun.flags.generated) {
            genericDepth++
        }
        if (fun.moreFlags.isTemplate) {
            noLint = true
            return
        }
        for (ann in fun.annotations) {
            if (ann.annotation.name == "no_lint") {
                noLint = true
                break
            }
        }
        // LINT010 — reset dead-store tracking
        branch_depth = 0
        ds_entries |> clear()
        // LINT021 — reset 64-bit-local tracking
        lint021_candidates |> clear()
        lint021_call_kind |> clear()
        lint021_narrow_depth = 0
        lint021_poison_depth = 0
        lint021_closure_depth = 0
    }
    def override preVisitExprReturn(expr : ExprReturn?) : void {
        if (expr.subexpr != null && expr.subexpr is ExprVar) {
            let ev = expr.subexpr as ExprVar
            let idx = lint014_candidates |> find_index(ev.variable)
            if (idx >= 0) {
                lint014_candidates |> erase(idx)
            }
        }
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        // LINT014 — report unwritten var arguments that were not returned either
        if (!noLint && genericDepth == 0) {
            for (v in lint014_candidates) {
                lint_error("LINT014: mutable argument {v.name}: {describe(v._type)} is never written; declare it without 'var'", v.at)
            }
        }
        lint014_candidates |> clear()
        // LINT010 — emit final dead-store warnings for any still-pending stores
        if (!noLint && genericDepth == 0) {
            for (e in ds_entries) {
                if (!e.skip && e.has_pending && e.pending_pure && e.pending_expr != null) {
                    let vname = "{e.v.name}"
                    lint_error("LINT010: dead store of '{vname}': value written but never read before scope exit", e.pending_expr.at)
                }
            }
        }
        // LINT021 — report 64-bit locals whose every use narrowed
        if (!noLint && genericDepth == 0) {
            for (c in lint021_candidates) {
                if (!c.disqualified && c.safe_uses > 0) {
                    lint_error("LINT021: 64-bit local '{c.v.name}' is never consumed as 64-bit - every use narrows through int(...)/uint(...); narrow once at the declaration (or use length() int math), or lift the sinks to 64-bit (range64, int64 resize/reserve/erase); nolint if the 64-bit math under a cast is load-bearing", c.v.at)
                }
            }
        }
        lint021_candidates |> clear()
        noLint = false
        if (fun.fromGeneric != null || fun.flags.generated) {
            genericDepth--
        }
        return <- fun
    }
    def override preVisitExprBlock(blk : ExprBlock?) : void {
        exprForTerminator |> push(0ul)
        // LINT010: a block passed as a closure / lambda body / generator body executes deferred or conditionally — treat it as a branch so writes inside don't kill pending stores outside and reads inside don't keep an outside-init "live" (the callback may never fire).
        let bf = blk.blockFlags
        if (!noLint && genericDepth == 0 && (bf.isClosure || bf.isLambdaBlock || bf.isGeneratorBlock)) {
            branch_depth++
            lint021_closure_depth++     // a captured use is a 64-bit escape, not a narrow
        }
    }
    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (!empty(exprForTerminator)) {
            exprForTerminator |> pop()
        }
        let bf = blk.blockFlags
        if (!noLint && genericDepth == 0 && (bf.isClosure || bf.isLambdaBlock || bf.isGeneratorBlock)) {
            branch_depth--
            lint021_closure_depth--
        }
        return <- blk
    }
    def override visitExprBlockExpression(_blk : ExprBlock?; var expr : ExpressionPtr) : ExpressionPtr {
        // Flag if a prior direct-child of THIS block already terminated control flow.
        let lb = exprForTerminator |> back()
        if (lb != 0ul) {
            lint_error("LINT001: unreachable code", expr.at)
        }
        // LINT015: a free-floating unary +/- subexpression used as a statement — a continuation line that lost its wrapping parens (`let x = a` ⏎ `+ b`, which re-parses as a unary `+b` whose value is discarded; if the operand is pure the optimizer drops the whole statement silently). Only `+`/`-` qualify: they are the only operators that are both binary and unary, so a split binary `a + b` orphans as a valid unary statement (`~`/`!` aren't binary, `*`/`|>`/etc. can't start a statement → loud error).
        if (!noLint && genericDepth == 0 && expr is ExprOp1 && !expr.genFlags.generated) {
            let op1 = expr as ExprOp1
            if (op1.op == "+" || op1.op == "-") {
                lint_error("LINT015: free-floating unary '{op1.op}' expression as a statement - its value is silently discarded; this is almost always a continuation line missing its parens, wrap the multi-line RHS in (...)", expr.at)
            }
        }
        // Mark this block as terminated only when `expr` IS a direct-child terminator.
        if (is_block_terminator(expr)) {
            exprForTerminator |> pop()
            exprForTerminator |> push(intptr(expr))
        }
        return <- expr
    }
    def override preVisitExprLabel(_expr : ExprLabel?) : void {
        if (!empty(exprForTerminator)) {
            exprForTerminator |> pop()
        }
        exprForTerminator |> push(0ul)
    }
    // --- LINT005: redundant reinterpret cast ---

    def override preVisitExprCast(expr : ExprCast?) : void {
        // alwaysSafe = compiler-authorized (e.g. the early-out relocation const-strips a finally-used `let` with a reinterpret the user never wrote)
        if (noLint || genericDepth > 0
            || !expr.castFlags.reinterpretCast || expr.genFlags.alwaysSafe
            || expr.subexpr == null || expr.subexpr._type == null || expr.castType == null
            || expr.castType.isVoidPointer != expr.subexpr._type.isVoidPointer) return
        if (is_same_type(expr.castType, expr.subexpr._type, false, true, true, false)) {
            lint_error("LINT005: redundant reinterpret cast; target type is the same as source type", expr.at)
        }
    }

    def override preVisitExprForVariable(expr : ExprFor?; v : VariablePtr; _last : bool) : void {
        if (expr.canShadow) return
        validate_var(v, false)
    }

    // `_inl<site id>_…` (`_inl0_l_text`, `_inl3_res`); the digit keeps a user's `_inline_x` in scope
    def private is_inline_temp_name(name : string) : bool {
        if (!(name |> starts_with("_inl")) || length(name) <= 4) return false
        var digit = false
        peek_data(name) $(bytes) {
            digit = is_number(int(bytes[4]))
        }
        return digit
    }

    def validate_var(v : VariablePtr; can_make_const : bool) {
        // marked_used: a transform consumed the reads (devirtualized block holder), or author intent
        if (v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        // skip names the compiler owns rather than the author: `__system`, backtick-mangled, inliner temps
        if (name |> starts_with("__") || find(name, "`") >= 0 || is_inline_temp_name(name)) return
        if (name |> starts_with("_")) {
            if (name |> ends_with("_")) return  // _foo_ - ignore too
            if (v.access_flags.access_ref || v.access_flags.access_pass || v.access_flags.access_get || v.access_flags.access_fold) {
                lint_error("LINT004: variable '{v.name}' is used and should be named without underscore prefix", v.at)
                return
            }
            return
        }
        if (v.isAccessUnused) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        }
        if (!v.access_flags.access_ref && !v.access_flags.access_pass && v.init != null && v.init |> is_expr_const() && !v.access_flags.access_fold) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it) {v.access_flags}", v.at)
            return
        }
        if (!v.access_flags.access_ref && !v.access_flags.access_pass && !v.access_flags.access_get && (v.init == null || v.init.flags.noSideEffects) && !v.access_flags.access_fold) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        }
        if (!v.access_flags.access_get && !v.access_flags.access_ref && (v.init == null)) {
            let sideEffects = (v.init != null && !v.init.flags.noSideEffects)
            if (!sideEffects) {
                lint_error("LINT002: variable {v.name}: {describe(v._type)} is never used", v.at)
            } else {
                lint_error("LINT002: variable {v.name}: {describe(v._type)} is never used (be careful, initializer has side effects)", v.at)
            }
            return
        }
        // inScope variables can't be const — delete/finalize at scope exit requires mutability.
        if (can_make_const && (v.flags.inScope || v.access_info.access_info_pass_mutable)) return
        if (can_make_const && v._type.baseType != Type.tPointer && !v._type.flags.constant && !v.access_flags.access_ref && v._type.canCloneFromConst) {// && !v._type.flags.smartPtr
            lint_error("LINT003: variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return
        }
        if (can_make_const && v._type.baseType == Type.tPointer && !v._type.flags.constant && !(v.access_flags.access_ref || v.access_flags.access_pass) && v._type.canCloneFromConst) {
            lint_error("LINT003: variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return
        }
    }

    def validate_argument(v : VariablePtr; code, kind : string) {
        // skip template/generic/[no_lint] context, generated vars, and [unused_argument(name)] (marked_used)
        if (noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        // A leading underscore on an argument means "leave it alone": intentionally unused, or dodging a reserved keyword / shadow (e.g. `_in`). Unlike locals, we do NOT raise LINT004 for a used `_arg` — a parameter name is often constrained and can't drop the underscore.
        if (name |> starts_with("_") || find(name, "`") >= 0) return
        if (v.isAccessUnused) {
            lint_error("{code}: unused {kind} {v.name}: {describe(v._type)} (add an underscore prefix or [unused_argument({v.name})] if intentional)", v.at)
        }
    }

    def validate_unwritten_var_argument(fun : FunctionPtr; v : VariablePtr) {
        if (noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        // skips, in order: leave-it-alone names; `finalize` overloads (the delete protocol dictates var) and address-taken functions (signature conforms to a function-pointer type); by-value parameters (a mutable copy is not a caller burden); written params (access_ref) and params whose mutability a callee's signature demands transitively (access_info_pass_mutable — the leaf callee gets flagged first; fixing it re-exposes this one); entirely-unused params (LINT012's report)
        if ((name |> starts_with("_")) || find(name, "`") >= 0
                || fun.name == "finalize" || fun.moreFlags.addressTaken
                || v._type == null || v._type.flags.constant
                || !(v._type.flags.ref || v._type.isRefType)
                || v.access_flags.access_ref || v.access_info.access_info_pass_mutable
                || v.isAccessUnused) {
            return
        }
        lint014_candidates |> push(v)
    }

    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; _lastArg : bool) : void {
        // class-method signatures are dictated by the base class / interface, and [extern] stubs by the bound C function — unused params there are structural, not a mistake
        if (fun.flags.isClassMethod) return
        for (ann in fun.annotations) {
            if (ann.annotation.name == "extern") return
        }
        validate_argument(arg, "LINT012", "function argument")
        validate_unwritten_var_argument(fun, arg)
    }

    def override preVisitExprBlockArgument(_blk : ExprBlock?; arg : VariablePtr; _lastArg : bool) : void {
        validate_argument(arg, "LINT013", "block argument")
    }

    // --- LINT017: int64/uint64 cast of a call that has a long_ counterpart ---

    def lint017_long_twin(fname : string; icall : ExprCall?) : string {
        // Receiver-gated, so a same-named user overload or the fixed-array `length` generic in daslib/builtin.das (which has no long_ twin) stays silent.
        let nargs = length(icall.arguments)
        if (nargs == 0 || icall.arguments[0]._type == null) return ""
        let bt = icall.arguments[0]._type.baseType
        if (fname == "length" && (bt == Type.tArray || bt == Type.tTable || bt == Type.tString)) return "long_length"
        if (fname == "capacity" && (bt == Type.tArray || bt == Type.tTable)) return "long_capacity"
        if (fname == "count" && (bt == Type.tArray || bt == Type.tIterator)) return "long_count"
        if (fname == "find_index" && bt == Type.tArray && nargs == 2) return "long_find_index"
        // fio's `file` is a pointer-to-handle; the type gate keeps a user-defined 2-arg fread/fwrite from being told to call a long_ twin that does not exist.
        if ((fname == "fread" || fname == "fwrite") && bt == Type.tPointer && nargs == 2) return "long_{fname}"
        return ""
    }

    def check_lint017_long_cast(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0
            || (expr.name != "int64" && expr.name != "uint64")
            || length(expr.arguments) != 1) return
        let inner = expr.arguments[0]
        if (!(inner is ExprCall)) return
        let icall = inner as ExprCall
        let iname = string(icall.func != null && icall.func.fromGeneric != null
            ? icall.func.fromGeneric.name
            : icall.name)
        let twin = lint017_long_twin(iname, icall)
        if (empty(twin)) return
        lint_error("LINT017: {expr.name}({iname}(...)) widens an already-32-bit result, so the 2^31 limit is still hit inside {iname} (as a wrap, or as the panic guard on array/table/string length) before the cast runs; call {twin}(...) instead", expr.at)
    }

    // --- LINT018: narrowing the size/position argument of a call that has a 64-bit overload ---

    def lint018_check_arg(expr : ExprCall?; fname : string; idx : int; isMem : bool) : bool {
        let a = expr.arguments[idx]
        if (!(a is ExprCall)) return false
        let scall = a as ExprCall
        if (scall.name != "int" || length(scall.arguments) != 1) return false
        let src = scall.arguments[0]._type
        if (src == null) return false
        let bt = src.baseType
        if (isMem) {
            if (bt != Type.tInt64 && bt != Type.tUInt64 && bt != Type.tUInt) return false
            lint_error("LINT018: int(...) truncates the {fname} size above 2^31; {fname} has uint/int64/uint64 overloads, so drop the cast", expr.at)
        } elif (bt == Type.tInt64) {
            lint_error("LINT018: int(...) truncates the {fname} argument above 2^31; {fname} has an int64 overload, so drop the cast", expr.at)
        } elif (bt == Type.tUInt64) {
            lint_error("LINT018: int(...) truncates the {fname} argument above 2^31; {fname} has an int64 overload - widen with int64(...) instead of narrowing", expr.at)
        } else {
            return false
        }
        return true
    }

    def check_lint018_mem_size_narrowing(expr : ExprCall?) : void {
        // Receiver-gated: string resize is int-only (no 64-bit twin), and same-named user functions on non-container receivers stay silent.
        if (noLint || genericDepth > 0) return
        let fname = string(expr.func != null && expr.func.fromGeneric != null
            ? expr.func.fromGeneric.name
            : expr.name)
        let nargs = length(expr.arguments)
        if ((fname == "memcpy" || fname == "memcmp") && nargs == 3) {
            lint018_check_arg(expr, fname, 2, true)
            return
        }
        if (nargs < 2 || expr.arguments[0]._type == null) return
        let bt = expr.arguments[0]._type.baseType
        if (bt == Type.tArray) {
            if (nargs == 2 && (fname == "resize" || fname == "resize_no_init"
                    || fname == "resize_and_init" || fname == "reserve" || fname == "erase")) {
                lint018_check_arg(expr, fname, 1, false)
            } elif (nargs == 3 && fname == "resize_and_init") {
                lint018_check_arg(expr, fname, 1, false)
            } elif (nargs == 3 && fname == "erase") {
                // once per call - both position and count narrowing carry the same fix
                if (!lint018_check_arg(expr, fname, 1, false)) {
                    lint018_check_arg(expr, fname, 2, false)
                }
            }
        } elif (bt == Type.tTable) {
            if (nargs == 2 && fname == "reserve") {
                lint018_check_arg(expr, fname, 1, false)
            }
        }
    }

    // --- LINT020: 64-bit bound narrowed into a 32-bit range constructor ---

    def check_lint020_range_narrowing(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0) return
        let isRange = expr.name == "range"
        let isURange = expr.name == "urange"
        let isInterval = expr.name == "interval"
        if (!isRange && !isURange && !isInterval) return
        let nargs = length(expr.arguments)
        if (nargs < 1 || nargs > 2) return
        for (a in expr.arguments) {
            if (!(a is ExprCall)) { continue; }
            let scall = a as ExprCall
            if ((scall.name != "int" && scall.name != "uint") || length(scall.arguments) != 1) { continue; }
            let src = scall.arguments[0]._type
            if (src == null || (src.baseType != Type.tInt64 && src.baseType != Type.tUInt64)) { continue; }
            if (isInterval) {
                lint_error("LINT020: {scall.name}(...) truncates a 64-bit bound above 2^31; interval has an int64 form, so drop the cast", expr.at)
            } else {
                let twin = isRange ? "range64" : "urange64"
                lint_error("LINT020: {expr.name}({scall.name}(...)) truncates a 64-bit bound above 2^31; use {twin}(...) on the 64-bit values directly - arrays and pointers index with int64/uint64 as-is", expr.at)
            }
            return  // once per constructor, however many bounds narrow
        }
    }

    // --- LINT021 call-context tracking (candidates seeded in preVisitExprLetVariable) ---

    def lint021_is_narrow_cast(expr : ExprCall?) : bool {
        if ((expr.name != "int" && expr.name != "uint") || length(expr.arguments) != 1) return false
        let t = expr.arguments[0]._type
        return t != null && (t.baseType == Type.tInt64 || t.baseType == Type.tUInt64)
    }

    def lint021_is_transparent(expr : ExprCall?) : bool {
        // only unbounded-top math keeps the hazard alive; min/clamp/sign cap the value, so int(clamp(x, lo, hi)) is deliberate saturation and must not fire
        let fname = string(expr.func != null && expr.func.fromGeneric != null
            ? expr.func.fromGeneric.name
            : expr.name)
        return fname == "max" || fname == "abs"
    }

    def override preVisitExprCall(expr : ExprCall?) : void {
        var kind = 0
        if (!noLint && genericDepth == 0) {
            if (lint021_is_narrow_cast(expr)) {
                kind = 1
                lint021_narrow_depth++
            } elif (lint021_narrow_depth > 0 && !lint021_is_transparent(expr)) {
                kind = 2
                lint021_poison_depth++
            }
        }
        lint021_call_kind |> push(kind)
        check_string_push_clone(expr)
        check_lint017_long_cast(expr)
        check_lint018_mem_size_narrowing(expr)
        check_lint020_range_narrowing(expr)
    }

    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (!empty(lint021_call_kind)) {
            let kind = lint021_call_kind |> back()
            lint021_call_kind |> pop()
            if (kind == 1) {
                lint021_narrow_depth--
            } elif (kind == 2) {
                lint021_poison_depth--
            }
        }
        return <- expr
    }

    // `assume name = <block>` keeps a template block at the alias-definition site; every use of
    // `name` is replaced with a *clone* of that block.
    // The template is never invoked directly, so its block arguments keep isAccessUnused == true and
    def override canVisitWithAliasSubexpression(_expr : ExprAssume?) : bool {
        return false
    }

    def override preVisitExprLet(expr : ExprLet?) : void {
        for (v in expr.variables) {
            validate_var(v, true)
        }
    }

    // --- LINT006: division by zero, LINT007: same operands ---

    def is_const_zero(expr : ExpressionPtr) : bool {
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt).value == 0
        if (expr is ExprConstUInt) return (expr as ExprConstUInt).value == 0u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64).value == 0l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64).value == 0ul
        if (expr is ExprConstFloat) return (expr as ExprConstFloat).value == 0.0f
        if (expr is ExprConstDouble) return (expr as ExprConstDouble).value == 0.0lf
        return false
    }

    def expr_equal(a : ExpressionPtr; b : ExpressionPtr; require_pure : bool = true) : bool {
        if (a == null || b == null
            || (require_pure && (!a.flags.noSideEffects || !b.flags.noSideEffects))) return false
        // daslang AST nodes are not shared — each Expression has exactly one parent, so pointer-equality between sibling operands (left vs right) is effectively never true. Structural compare via describe() is the only correct option here.
        return describe(a) == describe(b)
    }

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (noLint || genericDepth > 0) return
        let op = string(expr.op)
        if ((op == "/" || op == "%" || op == "/=" || op == "%=") && is_const_zero(expr.right)) {
            lint_error("LINT006: division by zero ({op})", expr.at)
            return
        }
        let op_same_suspicious = (
            op == "==" || op == "!=" ||
            op == "<"  || op == ">"  ||
            op == "<=" || op == ">=" ||
            op == "-"  || op == "/"  || op == "%" ||
            op == "&&" || op == "||" ||
            op == "&"  || op == "|"  || op == "^" ||
            op == "-=" || op == "/=" || op == "%="
        )
        // identical constants are macro-splice residue (`$v(n) > 1` with n == 1), const-fold food rather than a copy-paste typo
        if (op_same_suspicious && expr_equal(expr.left, expr.right, false)
                && !(expr.left.__rtti |> starts_with("ExprConst"))) {
            lint_error("LINT007: left and right operands of '{op}' are the same", expr.at)
        }
    }

    // --- LINT008: ternary both branches equal ---

    def override preVisitExprOp3(expr : ExprOp3?) : void {
        if (noLint || genericDepth > 0) return
        if (expr_equal(expr.left, expr.right, false)) {
            lint_error("LINT008: both branches of ternary '?:' are equivalent", expr.at)
        }
    }

    // --- LINT009: if-then-else branches equal ---

    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) : void {
        if (noLint || genericDepth > 0) return
        if (expr.if_false != null && expr_equal(expr.if_true, expr.if_false, false)) {
            lint_error("LINT009: 'then' branch is equivalent to 'else' branch", expr.at)
        }
        branch_depth++
    }
    def override visitExprIfThenElse(var expr : ExprIfThenElse?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) {
            branch_depth--
        }
        return <- expr
    }

    // --- LINT010: dead store ---

    def private ds_find(v : VariablePtr) : int {
        // Linear scan — typical function has <10 tracked locals.
        let vc : Variable const? = v
        for (i in range(length(ds_entries))) {
            if (ds_entries[i].v == vc) return i
        }
        return -1
    }

    def private lint021_find(v : VariablePtr) : int {
        let vc : Variable const? = v
        for (i in range(length(lint021_candidates))) {
            if (lint021_candidates[i].v == vc) return i
        }
        return -1
    }

    def override preVisitExprLetVariable(_let : ExprLet?; v : VariablePtr; _last : bool) : void {
        check_string_clone_init(v)
        if (noLint || genericDepth > 0) return
        // LINT021 — seed initialized non-ref 64-bit locals
        if (v._type != null && !v._type.flags.ref && v.init != null && !v.flags.generated
                && (v._type.baseType == Type.tInt64 || v._type.baseType == Type.tUInt64)) {
            lint021_candidates |> emplace(Lint021Candidate(v = v, safe_uses = 0, disqualified = false))
        }
        // Skip reference bindings (`var r & = x`) and inscope smart_ptr vars — these have aliasing or scope-exit semantics we don't reason about.
        if ((v._type != null && v._type.flags.ref)
            || v.flags.inScope
            || v.access_flags.access_fold
            || !v.access_flags.access_get) return
        let has_init = v.init != null
        let init_pure = has_init && v.init.flags.noSideEffects
        ds_entries |> emplace(DeadStoreEntry(
            v = v,
            pending_expr = v.init,
            skip = false,
            has_pending = has_init,
            pending_pure = init_pure
        ))
    }

    def override preVisitGlobalLetVariable(v : VariablePtr; _last : bool) : void {
        check_string_clone_init(v)
    }

    def override preVisitExprVar(expr : ExprVar?) : void {
        if (noLint || genericDepth > 0) return
        // LINT021 — classify this use of a tracked 64-bit local
        let ci = lint021_find(expr.variable)
        if (ci >= 0 && !lint021_candidates[ci].disqualified) {
            if (expr.varFlags.write || lint021_closure_depth > 0
                    || lint021_narrow_depth == 0 || lint021_poison_depth > 0) {
                lint021_candidates[ci].disqualified = true
            } else {
                lint021_candidates[ci].safe_uses++
            }
        }
        let i = ds_find(expr.variable)
        if (i < 0 || ds_entries[i].skip) return
        let vf = expr.varFlags
        // Any reference inside a branch/loop/lambda body, OR a write under an aliasing context (addr-of, ref-bind, mutable-ref-arg, compound op), means we can't reason about this var — bail.
        if (branch_depth > 0 || (vf.write && !vf.under_clone)) {
            ds_entries[i].skip = true
            return
        }
        // Pure store — handled in postVisit for ExprCopy/Clone/Move so the RHS reads have been observed first. preVisit of the LHS fires BEFORE the RHS is walked, so we do nothing here.
        if (vf.write) return
        // Read — kills any pending dead store on this var.
        ds_entries[i].has_pending = false
    }

    def private ds_handle_store(lhs : ExpressionPtr; rhs : ExpressionPtr; var store_expr : ExpressionPtr) : void {
        if (lhs == null || !(lhs is ExprVar)) return
        let ev = lhs as ExprVar
        let i = ds_find(ev.variable)
        if (i < 0 || ds_entries[i].skip || branch_depth > 0) return
        if (ds_entries[i].has_pending && ds_entries[i].pending_pure && ds_entries[i].pending_expr != null) {
            let vname = "{ev.variable.name}"
            lint_error("LINT010: dead store of '{vname}': overwritten without intervening read", ds_entries[i].pending_expr.at)
        }
        ds_entries[i].has_pending = true
        ds_entries[i].pending_expr = store_expr
        ds_entries[i].pending_pure = rhs != null && rhs.flags.noSideEffects
    }

    def override visitExprCopy(var expr : ExprCopy?) : ExpressionPtr {
        check_string_clone_assign(expr)
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }
    def override visitExprClone(var expr : ExprClone?) : ExpressionPtr {
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }
    def override visitExprMove(var expr : ExprMove?) : ExpressionPtr {
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }

    // Branch / loop / try / lambda — bumps branch_depth so any var referenced
    // inside is bailed-on.
    def override preVisitExprFor(_expr : ExprFor?) : void {
        if (!noLint && genericDepth == 0) branch_depth++
    }
    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) branch_depth--
        return <- expr
    }
    def override preVisitExprWhile(_expr : ExprWhile?) : void {
        if (!noLint && genericDepth == 0) branch_depth++
    }
    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) branch_depth--
        return <- expr
    }
    def override preVisitExprTryCatch(_expr : ExprTryCatch?) : void {
        if (!noLint && genericDepth == 0) branch_depth++
    }
    def override visitExprTryCatch(var expr : ExprTryCatch?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) branch_depth--
        return <- expr
    }
    // NOTE on lambda capture: by-copy/clone captures (the daslang default) read

    // --- LINT011: int-literal-to-float precision loss ---
    def override visitExprConstFloat(var expr : ExprConstFloat?) : ExpressionPtr {
        if (!noLint && genericDepth == 0 && expr.inexactFloatPromotion) {
            lint_error("LINT011: int literal does not exactly represent as float", expr.at)
        }
        return <- expr
    }
    def override visitExprConstDouble(var expr : ExprConstDouble?) : ExpressionPtr {
        if (!noLint && genericDepth == 0 && expr.inexactFloatPromotion) {
            lint_error("LINT011: int literal does not exactly represent as double", expr.at)
        }
        return <- expr
    }
}

def public paranoid(prog : ProgramPtr; compile_time_errors : bool) {
    //! Runs the paranoid lint visitor on the program to check for common coding issues.
    let empty_set : table<string>
    paranoid(prog, compile_time_errors, empty_set)
}

def public paranoid(prog : ProgramPtr; compile_time_errors : bool; disabled_codes : table<string>) {
    //! Filter-aware compile-time variant. ``disabled_codes`` suppresses
    //! the listed rule IDs (e.g. "LINT003") alongside the usual
    //! ``// nolint:CODE`` directives.
    var astVisitor = new LintVisitor(
        compile_time_errors = compile_time_errors,
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    make_visitor(*astVisitor) $(adapter) {
        astVisitor.astVisitorAdapter = adapter
        visit(prog, astVisitor.astVisitorAdapter)
    }
    unsafe {
        delete astVisitor
    }
}

def public paranoid_collect(prog : ProgramPtr; var errors : array<string>) : int {
    //! Runs the paranoid lint visitor and collects errors as strings.
    //! Returns the number of lint issues found.
    let empty_set : table<string>
    return paranoid_collect(prog, errors, empty_set, empty_set)
}

def public paranoid_collect(prog : ProgramPtr; var errors : array<string>;
                            disabled_codes, enabled_codes : table<string>) : int {
    //! Filter-aware overload. `disabled_codes` is a denylist; `enabled_codes`
    //! is a whitelist (empty == all). Codes use the bare form (e.g. "LINT002").
    var astVisitor = new LintVisitor(
        compile_time_errors = false,
        collect_errors = true,
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(adapter) {
        astVisitor.astVisitorAdapter = adapter
        visit(prog, astVisitor.astVisitorAdapter)
    }
    let count = length(astVisitor.errors)
    errors |> push_from(astVisitor.errors)
    unsafe {
        delete astVisitor
    }
    return count
}

def public paranoid_collect_issues(prog : ProgramPtr; var issues : array<LintIssue>;
                                   disabled_codes, enabled_codes : table<string>) : int {
    //! Structured twin of ``paranoid_collect`` — same filters, but emits
    //! ``LintIssue`` records (rule code + position) instead of display strings.
    var astVisitor = new LintVisitor(
        compile_time_errors = false,
        collect_errors = true,
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(adapter) {
        astVisitor.astVisitorAdapter = adapter
        visit(prog, astVisitor.astVisitorAdapter)
    }
    let count = length(astVisitor.issues)
    issues |> push_from(astVisitor.issues)
    unsafe {
        delete astVisitor
    }
    return count
}

//! LINT019 - stale nolint detection: reports every ``// nolint:CODE`` in `source` whose code
//! suppressed nothing this process (run-disabled codes are skipped - their rules cannot prove
//! themselves). Call AFTER all lint passes ran; a partial run under-consumes and would lie.
def public stale_nolint_collect(fname, source : string; disabled_codes, enabled_codes : table<string>; var issues : array<LintIssue>) : int {
    return 0 if (key_exists(disabled_codes, "LINT019")
                    || (!empty(enabled_codes) && !key_exists(enabled_codes, "LINT019")))
    var count = 0
    var line_no = 0
    var stale_codes : array<string>
    stale_codes |> reserve(4)
    for (raw_line in split(source, "\n")) {
        line_no++
        // a CRLF checkout leaves \r glued to the last token - strip it
        let line = raw_line |> ends_with("\r") ? slice(raw_line, 0, length(raw_line) - 1) : raw_line
        let cpos = find(line, "//")
        continue if (cpos < 0)
        stale_codes |> clear()
        var self_suppressed = false
        var at = find(line, "nolint:", cpos)
        while (at >= 0) {
            // comma-list parse, mirroring the C-side scanner in rtti_is_nolint_suppressed
            var p = at + length("nolint:")
            peek_data(line) $(bytes) {
                let n = length(bytes)
                while (p < n) {
                    while (p < n && (int(bytes[p]) == ' ' || int(bytes[p]) == '\t')) {
                        p++
                    }
                    let tok_start = p
                    while (p < n && int(bytes[p]) != ',' && int(bytes[p]) != ' ' && int(bytes[p]) != '\t') {
                        p++
                    }
                    break if (p == tok_start)
                    let code = slice(line, tok_start, p)
                    if (code == "LINT019") {
                        self_suppressed = true
                    } elif (is_rule_code(code)
                            && !key_exists(disabled_codes, code)
                            && (empty(enabled_codes) || key_exists(enabled_codes, code))
                            && !was_nolint_consumed(fname, line_no, code)) {
                        stale_codes |> push(code)
                    }
                    while (p < n && (int(bytes[p]) == ' ' || int(bytes[p]) == '\t')) {
                        p++
                    }
                    break if (p >= n || int(bytes[p]) != ',')
                    p++
                }
            }
            at = find(line, "nolint:", p)
        }
        if (!self_suppressed) {
            issues |> reserve(length(issues) + length(stale_codes))
            for (code in stale_codes) {
                let msg = "LINT019: stale nolint - {code} suppresses nothing on this line; remove it, or add LINT019 to the list if the rule fires only in downstream compiles (generic bodies - instantiations elsewhere fire at the generic's line - macro-template lines, option-gated rules)"
                issues |> push(LintIssue(code = "LINT019", message = msg, file = fname,
                    line = line_no, column = 0, last_line = line_no, last_column = 0))
                count++
            }
        }
    }
    return count
}

// a rule id is LINT/PERF/STYLE + digits - anything else on a nolint list is prose
// (docs quoting the syntax) or a typo the C-side scanner would never match anyway
def private is_rule_code(code : string) : bool {
    var prefix_len = 0
    if (code |> starts_with("LINT") || code |> starts_with("PERF")) {
        prefix_len = 4
    } elif (code |> starts_with("STYLE")) {
        prefix_len = 5
    } else {
        return false
    }
    var digits = 0
    var ok = true
    peek_data(code) $(bytes) {
        let n = length(bytes)
        for (i in range(prefix_len, n)) {
            if (!is_number(int(bytes[i]))) {
                ok = false
            }
            digits++
        }
    }
    return ok && digits > 0
}
