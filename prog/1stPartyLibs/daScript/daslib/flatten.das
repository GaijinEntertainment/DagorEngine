options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten shared private

//! Function flattening macro (cond-only).
//!
//! ``[flatten]`` on a function generates a semantically equivalent flattened
//! twin ``<name>_flat`` in which every user call is inlined away and all
//! control flow is lowered to predicated ``cond ? a : b`` selects (early
//! ``return`` becomes a runtime "live mask"). The output contains no user
//! calls and no branches — the form a branchless GPU-shader backend requires.
//!
//! The original function is left intact. ``apply`` declares the twin early so
//! its symbol exists for inference and callers; ``patch`` transforms the
//! twin's body across compiler re-infer passes.

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/rtti
require strings
require daslib/flatten_opt

// Phase token carried as the "phase" argument of the [flatten] annotation on
// the ORIGINAL function (which we own and may mutate in place). Cycle 1 lowers
// the twin and parks it at PHASE_LOWERED (then re-infers so call.func resolves);
// cycle 2 runs the post-infer select fold and parks at PHASE_FOLDED (re-infers
// again if it changed anything); the final cycle runs the shape scan and parks at
// PHASE_DONE so the entry never re-transforms.
let private PHASE_LOWERED = 1
let private PHASE_FOLDED = 2
let private PHASE_OPTIMIZED = 3
let private PHASE_FUSED = 4
let private PHASE_DONE = 999

// Calls that must stay per-pixel (can't run in the per-draw preshader). The [flatten] annotation
// path is pure math (STD_WHITELIST), so these never appear here — listed for robustness; a backend
// driving flatten_function directly passes its own sampler/intrinsic set to flatten_preshade_cse.
let private FLATTEN_BARRIERS : table<string> <- {
    "tex2d", "noise"
}

// Leaf builtins permitted to survive into flattened output: pure, branch-safe,
// GPU-expressible. Everything else (print, allocation, I/O, ...) is rejected.
// Hardcoded for now; intended to become a FlattenConfig argument.
let private STD_WHITELIST : table<string> <- {
    "min", "max", "abs", "clamp", "saturate", "sign", "floor", "ceil", "round", "trunc", "frac",
    "sqrt", "rsqrt", "pow", "exp", "exp2", "log", "log2",
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sincos",
    "dot", "cross", "length", "normalize", "distance", "reflect", "refract",
    "lerp", "mix", "step", "smooth_step", "rcp", "mad",
    "float2", "float3", "float4", "int2", "int3", "int4", "uint2", "uint3", "uint4"
}

// Compound-assignment operators → their base binary op. `acc += rhs` survives infer
// as an `ExprOp2` (op="+="), not an `ExprCopy`; predicated lowering rewrites it to
// `acc = P ? (acc + rhs) : acc` using the base op. This is the shader accumulator idiom.
let private ASSIGN_BASE : table<string; string> <- {
    "+=" => "+", "-=" => "-", "*=" => "*", "/=" => "/", "%=" => "%",
    "&=" => "&", "|=" => "|", "^=" => "^", "<<=" => "<<", ">>=" => ">>"
}

def private twin_name(func : FunctionPtr) : string {
    return "{func.name}_flat"
}

def private get_flatten_phase(func : FunctionPtr) : int {
    for (ann in func.annotations) {
        if (ann.annotation.name == "flatten") {
            let pv = find_arg(ann.arguments, "phase")
            if (pv is tInt) return pv as tInt
        }
    }
    return -1
}

def private set_flatten_phase(var func : FunctionPtr; ph : int) {
    for (ann in func.annotations) {
        if (ann.annotation.name != "flatten") continue
        var found = false
        for (a in ann.arguments) {
            if ("{a.name}" == "phase") {
                a.iValue = ph
                found = true
            }
        }
        if (!found) ann.arguments |> add_annotation_argument("phase", ph)
    }
}

//! Default cap on the product of nested constant-loop iteration counts a twin may unroll
//! to — unrolling multiplies across nesting, so an unbounded deep nest blows the heap.
//! `[flatten(max_unroll=N)]` overrides per-function.
let public FLATTEN_MAX_UNROLL = 4096

def private get_flatten_max_unroll(func : FunctionPtr) : int {
    for (ann in func.annotations) {
        if (ann.annotation.name == "flatten") {
            let mv = find_arg(ann.arguments, "max_unroll")
            if (mv is tInt) return mv as tInt
        }
    }
    return FLATTEN_MAX_UNROLL
}

// ===== Predicated lowering (return-elim + if-conversion + call inlining) =====

// One stack entry per enclosing flattened loop: loopLive = break mask (persists across
// copies), iterLive = continue mask (re-declared each copy); "" when that flavor is absent.
struct private LoopMask {
    loopLive : string
    iterLive : string
}

class private FlatCtx {
    counter   : int
    liveName  : string
    retName   : string
    hasRet    : bool
    worklist  : array<string>
    loopMasks : array<LoopMask>   // enclosing loops' break/continue masks (innermost last)
    whitelist : table<string>     // leaf-builtin names allowed to survive (backend-supplied)
    unrollFactor : int            // running product of enclosing loop iteration counts (1 at top)
    maxUnroll : int               // cap on unrollFactor*count before bailing (FLATTEN_MAX_UNROLL default)
    ok        : bool
    err      : string
}

def private fresh(var ctx : FlatCtx?; pfx : string) : string {
    let n = "{pfx}_{ctx.counter}"
    ctx.counter ++
    return n
}

def private contains(a : array<string>; s : string) : bool {
    for (x in a) {
        if (x == s) return true
    }
    return false
}

// Strip a monomorphized generic's mangling to the bare function name: an OR-type
// instance is named `<scope>`<name>`<hash>` (backtick = char 96). Used so the
// caller-supplied whitelist can match by simple name regardless of instantiation.
def private simple_name(fullname : string) : string {
    let t1 = find(fullname, 96, 0)
    if (t1 < 0) return fullname
    let t2 = find(fullname, 96, t1 + 1)
    let endp = t2 < 0 ? length(fullname) : t2
    return slice(fullname, t1 + 1, endp)
}

def private p_and(a, b : ExpressionPtr) : ExpressionPtr {
    if (a == null && b == null) return null
    if (a == null) return clone_expression(b)
    if (b == null) return clone_expression(a)
    return qmacro($e(a) && $e(b))
}

// The write predicate here: AND of every active mask (funcLive + each loop's loopLive/
// iterLive) and structPred. `exclude` drops one mask (self-cancels in a narrow term);
// null return = statically true.
def private live_pred(ctx : FlatCtx?; structPred : ExpressionPtr; exclude : string = "") : ExpressionPtr {
    var acc : ExpressionPtr = null
    if (ctx.liveName != exclude) {
        acc = qmacro($i(ctx.liveName))
    }
    for (lm in ctx.loopMasks) {
        if (lm.loopLive != "" && lm.loopLive != exclude) {
            acc = acc == null ? qmacro($i(lm.loopLive)) : qmacro($e(acc) && $i(lm.loopLive))
        }
        if (lm.iterLive != "" && lm.iterLive != exclude) {
            acc = acc == null ? qmacro($i(lm.iterLive)) : qmacro($e(acc) && $i(lm.iterLive))
        }
    }
    if (structPred != null) {
        acc = acc == null ? clone_expression(structPred) : qmacro($e(acc) && $e(structPred))
    }
    return acc
}

// Emit a mask narrow `name = name && !(term)` on the executing path. A null term
// means the path is statically reached → `name = false`. Shared by return / break
// / continue (their only difference is which mask narrows and what term excludes).
def private emit_narrow(var out : array<ExpressionPtr>; name : string; term : ExpressionPtr) {
    if (term == null) {
        out |> push <| qmacro_expr(${ $i(name) = false; })
    } else {
        out |> push <| qmacro_expr(${ $i(name) = $i(name) && !($e(term)); })
    }
}

def private is_user_call(e : ExpressionPtr) : bool {
    if (!(e is ExprCall)) return false
    let c = e as ExprCall
    return c.func != null && !c.func.flags.builtIn && c.func.body != null
}

// Emit a callee frame into `out`; return an expr yielding the callee's result.
def private inline_call(var ctx : FlatCtx?; call : ExprCall?; structPred : ExpressionPtr; var out : array<ExpressionPtr>) : ExpressionPtr {
    let callee = call.func
    let cname = string(callee.name)
    if (ctx.worklist |> contains(cname)) {
        ctx.ok = false
        ctx.err = "flatten: recursion through '{cname}' cannot be flattened"
        return qmacro(0)
    }
    let liveN = fresh(ctx, "__flat_live")
    let isVoidCallee = callee.result.isVoid
    var subst : Template
    for (p, ai in callee.arguments, count()) {
        let argN = fresh(ctx, "__flat_arg")
        // Lift the arg first (own inline frame / whitelist check) so `f(g(x))` inlines g.
        let lifted = lift_expr(ctx, call.arguments[ai], structPred, out)
        out |> push <| qmacro_expr(${ var $i(argN) = $e(lifted); })
        subst |> replaceVariable(string(p.name)) <| qmacro($i(argN))
    }
    // Uniquify the callee's locals per inline — else repeated inlining collides decls (30704).
    var seenLocals : array<string>
    let calleeLocals <- collect_let_names(callee.body)
    for (ln in calleeLocals) {
        if (!(seenLocals |> contains(ln))) {
            seenLocals |> push(ln)
            subst |> renameVariable(ln, fresh(ctx, "__flat_loc"))
        }
    }
    // Seed the callee frame with the caller's full predicate; it lowers with an empty loop stack.
    let seed = live_pred(ctx, structPred)
    out |> push <| qmacro_expr(${ var $i(liveN) = $e(seed); })
    // Result temp only for value-returning callees (a void callee has no value to bind).
    var retN = ""
    if (!isVoidCallee) {
        retN = fresh(ctx, "__flat_ret")
        var crt = clone_type(callee.result)
        crt.flags.constant = false
        // Bare decl: das auto-inits to default<T>; the first write self-reads it.
        out |> push <| qmacro_expr(${ var $i(retN) : $t(crt); })
    }
    let savedLive = ctx.liveName
    let savedRet = ctx.retName
    let savedHas = ctx.hasRet
    var savedLoops <- ctx.loopMasks   // move out → ctx.loopMasks is empty for the callee body
    ctx.liveName = liveN
    ctx.retName = retN
    ctx.hasRet = !isVoidCallee
    ctx.worklist |> push(cname)
    var cbody = clone_expression(callee.body)
    subst |> apply_template(call.at, cbody, false)
    lower_list(ctx, cbody as ExprBlock, null, out)
    ctx.worklist |> pop()
    ctx.liveName = savedLive
    ctx.retName = savedRet
    ctx.hasRet = savedHas
    ctx.loopMasks <- savedLoops
    if (isVoidCallee) return null
    return qmacro($i(retN))
}

// Lift every user call out of a value expression into inline frames, in eval order,
// returning the call-free rewrite. Plain recursion, NOT a visitor — inlining recurses back
// through the pipeline, and nested make_visitor traversals corrupt the visitor machinery.
def private lift_expr(var ctx : FlatCtx?; e : ExpressionPtr; structPred : ExpressionPtr; var out : array<ExpressionPtr>) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprCall) {
        let c = e as ExprCall
        let bname = string(c.name)
        // Whitelisted leaf primitive KEPT (args lifted); match simple name (generics mangle).
        if (key_exists(ctx.whitelist, simple_name(bname))) {
            var n = clone_expression(e)
            var o = n as ExprCall
            for (ai in range(length(c.arguments))) {
                o.arguments[ai] = lift_expr(ctx, c.arguments[ai], structPred, out)
            }
            return n
        }
        // A real user helper (resolved, non-builtin, has a body) → inline it.
        if (is_user_call(e)) {
            return inline_call(ctx, c, structPred, out)
        }
        // All-const pure builtin folds at the post-infer fold (foldable-type-gated); keep it, lift arg calls.
        if (is_all_const(e) && eval_to_const_supported(e._type)) {
            var n = clone_expression(e)
            var o = n as ExprCall
            for (ai in range(length(c.arguments))) {
                o.arguments[ai] = lift_expr(ctx, c.arguments[ai], structPred, out)
            }
            return n
        }
        // Otherwise: a non-whitelisted leaf/builtin — not backend-expressible.
        ctx.ok = false
        ctx.err = "flatten: builtin '{bname}' is not on the whitelist (not GPU-expressible / may have side effects)"
        return clone_expression(e)
    }
    if (e is ExprOp2) {
        var n = clone_expression(e)
        var o = n as ExprOp2
        let oe = e as ExprOp2
        o.left = lift_expr(ctx, oe.left, structPred, out)
        o.right = lift_expr(ctx, oe.right, structPred, out)
        return n
    }
    if (e is ExprOp1) {
        var n = clone_expression(e)
        var o = n as ExprOp1
        let oe = e as ExprOp1
        o.subexpr = lift_expr(ctx, oe.subexpr, structPred, out)
        return n
    }
    if (e is ExprOp3) {
        var n = clone_expression(e)
        var o = n as ExprOp3
        let oe = e as ExprOp3
        o.subexpr = lift_expr(ctx, oe.subexpr, structPred, out)
        o.left = lift_expr(ctx, oe.left, structPred, out)
        o.right = lift_expr(ctx, oe.right, structPred, out)
        return n
    }
    // A user call can hide in a swizzle/field/cast/index/ctor operand — recurse so it inlines.
    if (e is ExprField) {
        var n = clone_expression(e)
        var o = n as ExprField
        o.value = lift_expr(ctx, (e as ExprField).value, structPred, out)
        return n
    }
    if (e is ExprSwizzle) {
        var n = clone_expression(e)
        var o = n as ExprSwizzle
        o.value = lift_expr(ctx, (e as ExprSwizzle).value, structPred, out)
        return n
    }
    if (e is ExprCast) {
        var n = clone_expression(e)
        var o = n as ExprCast
        o.subexpr = lift_expr(ctx, (e as ExprCast).subexpr, structPred, out)
        return n
    }
    if (e is ExprAt) {
        var n = clone_expression(e)
        var o = n as ExprAt
        let oe = e as ExprAt
        o.subexpr = lift_expr(ctx, oe.subexpr, structPred, out)
        o.index = lift_expr(ctx, oe.index, structPred, out)
        return n
    }
    if (e is ExprMakeStruct) {
        var n = clone_expression(e)
        var o = n as ExprMakeStruct
        for (si in range(length(o.structs))) {
            for (fld in *(o.structs[si])) {
                fld.value = lift_expr(ctx, fld.value, structPred, out)
            }
        }
        return n
    }
    if (e is ExprMakeTuple) {
        var n = clone_expression(e)
        var o = n as ExprMakeTuple
        for (vi in range(length(o.values))) {
            o.values[vi] = lift_expr(ctx, o.values[vi], structPred, out)
        }
        return n
    }
    // Typer-inserted ref/value/ptr conversion wrappers — recurse through to the buried call.
    if (e is ExprRef2Value) {
        var n = clone_expression(e)
        var o = n as ExprRef2Value
        o.subexpr = lift_expr(ctx, (e as ExprRef2Value).subexpr, structPred, out)
        return n
    }
    if (e is ExprPtr2Ref) {
        var n = clone_expression(e)
        var o = n as ExprPtr2Ref
        o.subexpr = lift_expr(ctx, (e as ExprPtr2Ref).subexpr, structPred, out)
        return n
    }
    return clone_expression(e)
}

def private lower_value(var ctx : FlatCtx?; e : ExpressionPtr; structPred : ExpressionPtr; var out : array<ExpressionPtr>) : ExpressionPtr {
    return lift_expr(ctx, e, structPred, out)
}

def private lower_list(var ctx : FlatCtx?; blk : ExprBlock?; structPred : ExpressionPtr; var out : array<ExpressionPtr>) {
    for (s in blk.list) {
        lower_stmt(ctx, s, structPred, out)
    }
}

def private lower_branch(var ctx : FlatCtx?; br : ExpressionPtr; structPred : ExpressionPtr; var out : array<ExpressionPtr>) {
    if (br is ExprBlock) {
        lower_list(ctx, br as ExprBlock, structPred, out)
    } else {
        lower_stmt(ctx, br, structPred, out)
    }
}

// Lower `break`/`continue` by narrowing the innermost loop's mask. The narrow term excludes
// the target mask (self-cancels) and is gated by the other active masks + structPred, so a
// prior exit on the same path suppresses this one.
def private lower_loop_exit(var ctx : FlatCtx?; structPred : ExpressionPtr; var out : array<ExpressionPtr>; isBreak : bool) {
    if (empty(ctx.loopMasks)) {
        ctx.ok = false
        ctx.err = "flatten: internal error — {isBreak ? "break" : "continue"} outside a flattened loop"
        return
    }
    let lm = ctx.loopMasks[length(ctx.loopMasks) - 1]
    let nm = isBreak ? lm.loopLive : lm.iterLive
    if (nm == "") {
        ctx.ok = false
        ctx.err = "flatten: internal error — {isBreak ? "break" : "continue"} mask was not allocated"
        return
    }
    emit_narrow(out, nm, live_pred(ctx, structPred, nm))
}

def private lower_stmt(var ctx : FlatCtx?; stmt : ExpressionPtr; structPred : ExpressionPtr; var out : array<ExpressionPtr>) {
    if (stmt is ExprReturn) {
        let r = stmt as ExprReturn
        if (ctx.hasRet && r.subexpr != null) {
            let valExpr = lower_value(ctx, r.subexpr, structPred, out)
            let pred = live_pred(ctx, structPred)
            out |> push <| qmacro_expr(${ $i(ctx.retName) = $e(pred) ? $e(valExpr) : $i(ctx.retName); })
        }
        // Narrow funcLive on the executing path (funcLive self-cancels in the term).
        emit_narrow(out, ctx.liveName, live_pred(ctx, structPred, ctx.liveName))
    } elif (stmt is ExprIfThenElse) {
        let ite = stmt as ExprIfThenElse
        let cN = fresh(ctx, "__flat_c")
        out |> push <| qmacro_expr(${ let $i(cN) = $e(ite.cond); })
        lower_branch(ctx, ite.if_true, p_and(structPred, qmacro($i(cN))), out)
        if (ite.if_false != null) {
            lower_branch(ctx, ite.if_false, p_and(structPred, qmacro(!$i(cN))), out)
        }
    } elif (stmt is ExprCopy) {
        let cp = stmt as ExprCopy
        let rhs = lower_value(ctx, cp.right, structPred, out)
        let pred = live_pred(ctx, structPred)
        out |> push <| qmacro_expr(${ $e(cp.left) = $e(pred) ? $e(rhs) : $e(cp.left); })
    } elif (stmt is ExprLet) {
        // A local decl always executes; inline any user-call initializer (its frame is predicated).
        var letClone = clone_expression(stmt)
        var lc = letClone as ExprLet
        for (v in lc.variables) {
            if (v.init != null) {
                v.init = lower_value(ctx, v.init, structPred, out)
            } else {
                // materialize the implicit zero-init — the predicated store's false path READS it
                v.init = zero_const_of(v._type, v.at)
            }
        }
        out |> push <| letClone
    } elif (stmt is ExprFor) {
        lower_for(ctx, stmt as ExprFor, structPred, out)
    } elif (stmt is ExprWhile) {
        ctx.ok = false
        ctx.err = "flatten: while loops are not supported; use a fixed range(N) or array-literal for loop"
    } elif (stmt is ExprBreak) {
        // Narrow the loop's break mask: persists across copies, so a break masks off the rest.
        lower_loop_exit(ctx, structPred, out, true)
    } elif (stmt is ExprContinue) {
        // Narrow the per-iteration mask: the rest of THIS copy masks off; next copy resets it.
        lower_loop_exit(ctx, structPred, out, false)
    } elif (stmt is ExprMove || stmt is ExprClone) {
        ctx.ok = false
        ctx.err = "flatten: move (<-) / clone (:=) cannot be predicated; only copyable-value assignment is supported"
    } elif (stmt is ExprOp2 && key_exists(ASSIGN_BASE, "{(stmt as ExprOp2).op}")) {
        // Compound assignment `lhs OP= rhs` → `lhs = P ? (lhs OP rhs) : lhs`.
        let o = stmt as ExprOp2
        let bop = ASSIGN_BASE?["{o.op}"] ?? ""
        let pred = live_pred(ctx, structPred)
        var combined = new ExprOp2(at = o.at, op := bop, left = clone_expression(o.left),
            right = lower_value(ctx, o.right, structPred, out))
        out |> push <| qmacro_expr(${ $e(o.left) = $e(pred) ? $e(combined) : $e(o.left); })
    } elif (stmt is ExprOp1 && ("{(stmt as ExprOp1).op}" == "++" || "{(stmt as ExprOp1).op}" == "--")) {
        // `x++`/`x--` statement → `x = P ? (x +/- 1) : x` (postfix value discarded at stmt level).
        let o = stmt as ExprOp1
        let bop = "{o.op}" == "++" ? "+" : "-"
        let pred = live_pred(ctx, structPred)
        var combined = new ExprOp2(at = o.at, op := bop, left = clone_expression(o.subexpr),
            right = new ExprConstInt(at = o.at, value = 1))
        out |> push <| qmacro_expr(${ $e(o.subexpr) = $e(pred) ? $e(combined) : $e(o.subexpr); })
    } elif (stmt is ExprBlock) {
        // Bare lexical scope — flatten its statements under the same predicate.
        lower_list(ctx, stmt as ExprBlock, structPred, out)
    } elif (stmt is ExprWith) {
        // `with (x) { body }` is infer-time only; post-infer it simulates to its body — unwrap it.
        lower_branch(ctx, (stmt as ExprWith).body, structPred, out)
    } else {
        // Bare expr statement: lift emits side-effecting frames; the unused pure value is dropped.
        lower_value(ctx, stmt, structPred, out)
    }
}

// Collects the names of every `let`/`var` declared anywhere in a loop body, so
// each unrolled copy can rename them uniquely (they would otherwise all land in
// the single flat scope and shadow each other).
class private LetNameScan : AstVisitor {
    names : array<string>
    def override preVisitExprLet(expr : ExprLet?) : void {
        for (v in expr.variables) {
            names |> push("{v.name}")   // nolint:PERF006 -- macro-time, count is the (small) local set
        }
    }
}

def private collect_let_names(body : ExpressionPtr) : array<string> {
    var v = new LetNameScan()
    make_visitor(*v) $(adapter) {
        visit(body, adapter)
    }
    var result <- v.names
    unsafe {
        delete v
    }
    return <- result
}

// Does THIS loop's body have a break/continue targeting it? A nested loop owns its own, so
// count only at depth 0. Drives whether lower_for allocates the break/continue masks — a loop
// with neither stays byte-identical to the pre-mask output.
class private BreakContinueScan : AstVisitor {
    depth       : int
    hasBreak    : bool
    hasContinue : bool
    def override preVisitExprFor(expr : ExprFor?) : void {
        depth ++
    }
    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        depth --
        return expr
    }
    def override preVisitExprWhile(expr : ExprWhile?) : void {
        depth ++
    }
    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        depth --
        return expr
    }
    def override preVisitExprBreak(expr : ExprBreak?) : void {
        if (depth == 0) {
            hasBreak = true
        }
    }
    def override preVisitExprContinue(expr : ExprContinue?) : void {
        if (depth == 0) {
            hasContinue = true
        }
    }
}

def private scan_break_continue(body : ExpressionPtr; var hasBreak : bool&; var hasContinue : bool&) {
    var v = new BreakContinueScan()
    make_visitor(*v) $(adapter) {
        visit(body, adapter)
    }
    if (v.hasBreak) {
        hasBreak = true
    }
    if (v.hasContinue) {
        hasContinue = true
    }
    unsafe {
        delete v
    }
}

// Build one substitution expression per iteration for a single for-source — a
// constant `range`/`urange` (the index) or an array literal (each element). Sets
// ctx.ok=false + ctx.err and returns an empty list on an unrollable source.
def private for_source_substs(var ctx : FlatCtx?; src : Expression?; at : LineInfo) : array<ExpressionPtr> {
    var substs : array<ExpressionPtr>
    if (src is ExprConstRange) {
        for (i in (src as ExprConstRange).value) {
            var e : ExpressionPtr = new ExprConstInt(at = at, value = i)
            substs |> push(e)   // nolint:PERF006 -- macro-time, count is the (small) unroll bound
        }
    } elif (src is ExprConstURange) {
        for (i in (src as ExprConstURange).value) {
            var e : ExpressionPtr = new ExprConstUInt(at = at, value = i)
            substs |> push(e)   // nolint:PERF006 -- macro-time, count is the (small) unroll bound
        }
    } else {
        // Array-literal source (`for (v in [a,b,c])`): each element substitutes for one iteration.
        var mk : ExprMakeArray? = null
        if (src is ExprMakeArray) {
            mk = src as ExprMakeArray
        } elif (src is ExprCall && !empty((src as ExprCall).arguments) &&
                (src as ExprCall).arguments[0] is ExprMakeArray) {
            mk = (src as ExprCall).arguments[0] as ExprMakeArray
        }
        if (mk == null) {
            ctx.ok = false
            ctx.err = "flatten: for loop source must be a constant range(N)/urange(N) or an array literal"
            return <- substs
        }
        substs <- [for (v in mk.values); clone_expression(v)]
    }
    return <- substs
}

// Fixed-count loop unrolling: a `for` over a constant range/array literal clones its body once
// per iteration (loop var → that iteration's constant), each copy lowered under the SAME
// predicate; per-copy locals are renamed. break/continue allocate runtime masks. while errors.
def private lower_for(var ctx : FlatCtx?; efor : ExprFor?; structPred : ExpressionPtr; var out : array<ExpressionPtr>) {
    let nIter = length(efor.iterators)
    if (nIter == 0 || length(efor.sources) != nIter) {
        ctx.ok = false
        ctx.err = "flatten: a for loop needs one source per iterator"
        return
    }
    // Per-iteration value list per source; parallel iterators unroll in lockstep (same count).
    var perSource : array<array<ExpressionPtr>>
    perSource |> reserve(nIter)
    var count = -1
    for (si in range(nIter)) {
        var s <- for_source_substs(ctx, efor.sources[si], efor.at)
        if (!ctx.ok) {
            return
        }
        if (count >= 0 && length(s) != count) {
            ctx.ok = false
            ctx.err = "flatten: parallel for sources must all have the same length ({count} vs {length(s)})"
            return
        }
        count = length(s)
        perSource |> emplace(s)
    }
    // Unrolling multiplies across nesting; bail before cloning if the int64 product exceeds the cap.
    let projected = int64(ctx.unrollFactor) * int64(count)
    if (projected > int64(ctx.maxUnroll)) {
        ctx.ok = false
        ctx.err = "flatten: loop unrolling exceeds max_unroll={ctx.maxUnroll} (would expand to {projected} iterations); reduce the range/nesting or raise [flatten(max_unroll=N)]"
        return
    }
    let savedFactor = ctx.unrollFactor
    ctx.unrollFactor = int(projected)
    let localNames <- collect_let_names(efor.body)
    // Allocate break/continue masks only if THIS loop uses them (depth-0 scan).
    var hasBreak = false
    var hasContinue = false
    scan_break_continue(efor.body, hasBreak, hasContinue)
    var pushed = false
    if (hasBreak || hasContinue) {
        var loopName = ""
        if (hasBreak) {
            loopName = fresh(ctx, "__flat_loop")
            out |> push <| qmacro_expr(${ var $i(loopName) = true; })   // persists across copies
        }
        ctx.loopMasks |> push(LoopMask(loopLive = loopName, iterLive = ""))
        pushed = true
    }
    for (k in range(count)) {
        if (pushed && hasContinue) {
            // Fresh per-copy continue mask, reset to true at this copy's head.
            let iterName = fresh(ctx, "__flat_iter")
            out |> push <| qmacro_expr(${ var $i(iterName) = true; })
            ctx.loopMasks[length(ctx.loopMasks) - 1].iterLive = iterName
        }
        var iblk = clone_expression(efor.body)
        var rules : Template
        // Substitute every iterator with its copy-k value (several for a lockstep parallel loop).
        for (vi in range(nIter)) {
            let van = "{efor.iterators[vi]}"
            remove_deref(van, iblk)            // strip ExprRef2Value around the workhorse loop var
            rules |> replaceVariable(van) <| perSource[vi][k]
        }
        // Rename this copy's locals uniquely so unrolled bodies don't shadow in the flat scope.
        for (ln in localNames) {
            rules |> renameVariable(ln, "{ln}_{k}")
        }
        apply_template(rules, efor.at, iblk, false)
        lower_branch(ctx, iblk, structPred, out)
        if (!ctx.ok) {
            if (pushed) {
                ctx.loopMasks |> pop()
            }
            ctx.unrollFactor = savedFactor
            return
        }
    }
    if (pushed) {
        ctx.loopMasks |> pop()
    }
    ctx.unrollFactor = savedFactor
}

[macro_function]
def public flatten_function(var func : FunctionPtr; whitelist : table<string>; maxUnroll : int = FLATTEN_MAX_UNROLL) : FlatCtx? {
    //! Flatten `func`'s body in place to branchless, call-free form: user calls inlined, control
    //! flow → predicated `?:`, early `return` → a live mask. `whitelist` = leaf builtins allowed to
    //! survive; `maxUnroll` caps unroll. Returns FlatCtx (`.ok`/`.err`); body is uninferred after.
    var ctx = new FlatCtx(counter = 0, liveName = "__flat_live", retName = "__flat_ret",
        hasRet = !func.result.isVoid, unrollFactor = 1, maxUnroll = maxUnroll, ok = true)
    ctx.whitelist := whitelist
    if (!(func.body is ExprBlock)) return ctx
    let bodyBlk = func.body as ExprBlock
    // Build the lowered body on a scoped gc root: the unroll/fold intermediates are swept on
    // scope exit, only the live twin body (func.body) is promoted to the enclosing root.
    ast_gc_collect_scope(func) {
        var out : array<ExpressionPtr>
        out |> push <| qmacro_expr(${ var $i(ctx.liveName) = true; })
        if (ctx.hasRet) {
            var rt = clone_type(func.result)
            rt.flags.constant = false
            // The return accumulator is read self-referentially by every `__flat_ret = pred ? v :
            // __flat_ret` write; the first (bare-decl) read is a discarded placeholder, so it is
            // safeWhenUninitialized — else `var x : Struct` is rejected (error[31016]).
            rt.flags.safeWhenUninitialized = true
            out |> push <| qmacro_expr(${ var $i(ctx.retName) : $t(rt); })
        }
        lower_list(ctx, bodyBlk, null, out)
        if (ctx.hasRet) {
            out |> push <| qmacro_expr(${ return $i(ctx.retName); })
        }
        if (ctx.ok) {
            dse_pass(out)          // drop dead mask-kills (`__flat_live = false`) first…
            copy_prop_pass(out)    // inline single-def temps AND const-fold the masks they expose
                                   // (`__flat_live=true` → `true ? a:b` → a; `__flat_ret=c; return __flat_ret` → `return c`)
            dse_pass(out)          // remove anything left dead (collapsed mask decls, folded temps)
            ssa_rename_pass(out)   // single-def-ify reassigned user locals → fold_body const-props the chain
        }
        var newBody = new ExprBlock(at = func.body.at)
        for (s in out) {
            newBody.list |> emplace(s)
        }
        func.body = newBody
    }
    return ctx
}

// ===== Post-infer select folding =====

[macro_function]
def public flatten_no_fast_math : bool {
    //! True when the COMPILING module sets `options _flatten_no_fast_math = true` — the user
    //! opt-out of flatten's value-changing rewrites; bit-exact passes ignore it. COMPILE-TIME ONLY
    //! (panics outside a compile): patch code reads it once, runtime tools pass their flag explicitly.
    return compiling_program()._options |> find_arg("_flatten_no_fast_math") ?as tBool ?? false
}

[macro_function]
def public flatten_expand_lerp : bool {
    //! True when the COMPILING module sets `options _flatten_expand_lerp = true` — opt-in lerp
    //! disassembly for backends WITHOUT a native lerp instruction (a native-lerp ISA only loses
    //! nodes to it). COMPILE-TIME ONLY: patch code reads it once, runtime tools pass their flag.
    return compiling_program()._options |> find_arg("_flatten_expand_lerp") ?as tBool ?? false
}

[macro_function]
def public flatten_fold(var func : FunctionPtr; no_fast_math : bool = false; expand_lerp : bool = false) : bool {
    //! Post-infer fold pass (delegates to `fold_body`): collapse `const ? a : b`, fold identities,
    //! drop pure self-assigns. Run AFTER re-inference; returns true if anything changed. Compile-time
    //! callers pass `flatten_no_fast_math()` / `flatten_expand_lerp()` to honor the module options.
    return fold_body(func, no_fast_math, expand_lerp)
}

[macro_function]
def public flatten_log_cse : bool {
    //! True when the COMPILING module sets `options _flatten_log_cse = true` — dump each twin's
    //! final CSE value-number cache (pure-subtree tally + const-minus shapes) after the optimize
    //! fixpoint. COMPILE-TIME ONLY: patch code reads it once, runtime tools pass their flag.
    return compiling_program()._options |> find_arg("_flatten_log_cse") ?as tBool ?? false
}

[macro_function]
def public flatten_optimize(var func : FunctionPtr; barriers : table<string>; no_fast_math : bool = false; log_cse : bool = false) : bool {
    //! Post-fold optimize pass (delegates to `flatten_preshade_cse`): hoist uniform subtrees to
    //! per-draw `_preshader_` lets + regroup const-minus sums toward live `C - x` twins + CSE-dedup
    //! + rcp-rewrite uniform divisions. After the `flatten_fold` fixpoint; `barriers` = sampler names.
    return flatten_preshade_cse(func, barriers, no_fast_math, log_cse)
}

[macro_function]
def public flatten_fuse(var func : FunctionPtr; no_fast_math : bool = false) : bool {
    //! PHASE_FUSED finishing pass (delegates to `fuse_body`): collapse same-vector lane sums into
    //! `dot(v, mask)`, re-pack ctor lane patterns, `a*b ± c` into mad, and the expanded lerp shape
    //! back into lerp. Call on the optimized, re-inferred body; repeat until it returns false.
    return fuse_body(func, no_fast_math)
}

def public flatten_opt_residuals(var func : FunctionPtr; no_fast_math : bool = false) : array<string> {
    //! Completeness oracle for the optimize pass (delegates to `opt_residuals`): empty ⇒ the twin
    //! has no un-extracted uniform subtree, no un-CSE'd repeat, no un-rcp'd uniform division,
    //! no un-packed ctor lane pattern, no un-fused lane sum, no regroup-to-share miss.
    var res : array<string>
    ast_gc_guard() {
        res <- opt_residuals(func, FLATTEN_BARRIERS, no_fast_math)
    }
    return <- res
}

// ===== Post-inference shape verification — twin must be branchless + call-free =====
// A violation is a lowering bug (fail loudly). Runs cycle 2 (user-call detection needs the
// resolved `call.func`); fold completeness is a separate, correctness-irrelevant concern.

class private FlatVerify : AstVisitor {
    sawBranch   : bool
    sawCall     : bool
    callName    : string
    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) : void {
        sawBranch = true
    }
    def override preVisitExprCall(expr : ExprCall?) : void {
        if (expr.func != null && !expr.func.flags.builtIn && expr.func.body != null) {
            sawCall = true
            callName = "{expr.name}"
        }
    }
}

def private verify_flat(var twin : FunctionPtr) : string {
    var v = new FlatVerify()
    make_visitor(*v) $(adapter) {
        visit(twin.body, adapter)
    }
    var msg = ""
    if (v.sawBranch) {
        msg = "flatten: internal error — a branch (if/else) survived lowering in '{twin.name}'"
    } elif (v.sawCall) {
        msg = "flatten: internal error — user call '{v.callName}' survived lowering in '{twin.name}'"
    }
    unsafe {
        delete v
    }
    return msg
}

// ===== Fold-completeness validation (test-framework facing) =====
// `flatten_fold_residuals` is the ONLY fold-completeness check — a missed fold is suboptimal,
// not wrong, so it's validated over the COMPILED twin's AST, never gated at compile time.

class private FoldResidualVisitor : AstVisitor {
    found : array<string>
    noFastMath : bool
    def override preVisitExprCall(expr : ExprCall?) : void {
        if (expr.func != null && expr.func.flags.builtIn && is_all_const(expr) && eval_to_const_supported(expr._type)) {
            found |> push("an unfolded all-const call '{expr.name}'")
        }
    }
    def override preVisitExprOp3(expr : ExprOp3?) : void {
        if (expr.subexpr is ExprConstBool) {
            found |> push("an unfolded const-condition select")
        }
    }
    def override preVisitExprOp2(var expr : ExprOp2?) : void {
        if (is_unfolded_identity_op2(expr)) {
            found |> push("an unfolded algebraic identity ('{expr.op}')")
        } elif (is_all_const(expr) && eval_to_const_supported(expr._type)) {
            found |> push("an unfolded all-const arithmetic ('{expr.op}')")
        } elif (has_scattered_consts(expr, noFastMath)) {
            found |> push("a scattered const chain ('{expr.op}') reassoc should gather")
        } elif (!noFastMath && is_unrcp_const_div(expr)) {
            found |> push("an un-rewritten const division ('x / C' should be 'x * (1/C)')")
        } elif (is_uncollapsed_splat_op2(expr)) {
            found |> push("an uncollapsed splat operand ('v {expr.op} floatN(s)' should use the scalar)")
        } elif (is_unkilled_ctor_zero_lane(expr, noFastMath)) {
            found |> push("an un-killed ctor lane under a zero const lane")
        }
    }
    def override preVisitExprOp1(expr : ExprOp1?) : void {
        if (expr.op == "!" && expr.subexpr is ExprConstBool) {
            found |> push("an unfolded constant '!'")
        } elif (expr.op == "!" && can_negate_bool(expr.subexpr, noFastMath)) {
            found |> push("an unfolded boolean inversion ('!' over a comparison / '!' / && / ||)")
        }
    }
    def override preVisitExprSwizzle(var expr : ExprSwizzle?) : void {
        if (is_unfolded_swizzle_lanes(expr)) {
            found |> push("an un-folded swizzle over decomposable lanes ('ctor(…).mask' should re-pack)")
        }
    }
}

def public flatten_fold_residuals(var func : FunctionPtr; no_fast_math : bool = false) : array<string> {
    //! Collect the const-foldable residuals left in `func`'s (post-flatten) body — empty means the
    //! fold is complete; reuses the exact predicates the macro's own fold uses (test/fuzzer facing).
    //! Pass `no_fast_math = true` for a unit compiled under `options _flatten_no_fast_math`.
    var res : array<string>
    if (func == null || func.body == null) {
        return <- res
    }
    // the probe matchers (recip_const, ctor_fuse_rewrite) clone AST nodes; at runtime (tests,
    // fuzzer) those land on the thread root — guard the walk so they are swept, not leaked
    ast_gc_guard() {
        var v = new FoldResidualVisitor()
        v.noFastMath = no_fast_math
        make_visitor(*v) $(adapter) {
            visit(func.body, adapter)
        }
        res <- v.found
        unsafe {
            delete v
        }
    }
    return <- res
}

// ===== Annotation entry point =====

[function_macro(name="flatten")]
class FlattenAnnotation : AstFunctionAnnotation {
    //! ``[flatten]`` entry point.
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        let tn = twin_name(func)
        if (find_unique_function(compiling_module(), tn, true) == null) {
            var fn = clone_function(func)
            fn.name := tn
            fn.flags |= FunctionFlags.generated | FunctionFlags.exports
            while (fn.annotations |> length != 0) {
                fn.annotations |> erase(0)
            }
            compiling_module() |> add_function(fn)
        }
        return true
    }
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       args, progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        // Lint runs with const-folding off, so `range(N)` isn't an ExprConstRange yet and the twin
        // is irrelevant to linting; leave it the inert clone from `apply` (mirrors unroll.das).
        if (is_in_lint_check()) return true
        let ph = get_flatten_phase(func)
        if (ph == PHASE_DONE) return true
        var twin = find_unique_function(compiling_module(), twin_name(func), true)
        if (twin == null) {
            errors := "flatten: twin {twin_name(func)} was not declared"
            return false
        }
        if (ph != PHASE_LOWERED && ph != PHASE_FOLDED && ph != PHASE_OPTIMIZED && ph != PHASE_FUSED) {
            // Cycle 1: lower the twin with the std whitelist, then re-infer so
            // call.func resolves. (Backends call flatten_function directly with
            // their own primitive set instead of going through this annotation.)
            var ctx = flatten_function(twin, STD_WHITELIST, get_flatten_max_unroll(func))
            if (!ctx.ok) {
                errors := ctx.err
                return false
            }
            set_flatten_phase(func, PHASE_LOWERED)
            astChanged = true
            return true
        }
        if (ph == PHASE_LOWERED || ph == PHASE_FOLDED) {
            // Cycle 2+: fold the inferred twin (const selects, algebraic identities, pure self-
            // assigns), re-infer if anything changed. Iterate to a fixpoint — fold and the typer's
            // re-infer const-fold are mutually-enabling, so one pass isn't enough.
            let folded = flatten_fold(twin, flatten_no_fast_math(), flatten_expand_lerp())
            set_flatten_phase(func, PHASE_FOLDED)
            if (folded) {
                astChanged = true
                return true
            }
            // Fold fixpoint reached → run the optimize pass once on the canonical body: hoist uniform
            // subtrees to top-of-body `_preshader_` lets, CSE-dedup repeated subtrees. Re-infer to
            // type the new lets, then fuse next cycle.
            set_flatten_phase(func, PHASE_OPTIMIZED)
            if (flatten_optimize(twin, FLATTEN_BARRIERS, flatten_no_fast_math(), flatten_log_cse())) {
                astChanged = true
                return true
            }
        }
        if (get_flatten_phase(func) == PHASE_OPTIMIZED || get_flatten_phase(func) == PHASE_FUSED) {
            // Optimize converged → re-pack `a*b ± c` into `mad` and the lerp shape back into
            // `lerp`, re-inferring to resolve the new calls; iterates (each round strictly
            // drops a `+`/`-`, so it converges), a clean round falls through to the shape check.
            set_flatten_phase(func, PHASE_FUSED)
            if (flatten_fuse(twin, flatten_no_fast_math())) {
                astChanged = true
                return true
            }
        }
        // Final cycle: verify the twin is branchless and call-free.
        let problem = verify_flat(twin)
        if (problem != "") {
            errors := problem
            return false
        }
        set_flatten_phase(func, PHASE_DONE)
        return true
    }
}
