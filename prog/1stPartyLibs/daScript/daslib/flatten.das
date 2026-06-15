options gen2
options indenting = 4
options strict_smart_pointers = true

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
let private PHASE_DONE = 999

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

//! Default cap on the product of nested constant-loop iteration counts a twin may
//! unroll to. Unrolling multiplies across nesting (`range(20)^4` = 160k body copies),
//! so without a bound deep nests blow the heap. 4096 is generous for real shader
//! unrolling yet far below the blowup zone; `[flatten(max_unroll=N)]` overrides it
//! per-function, and direct `flatten_function` callers pass their own.
let public FLATTEN_MAX_UNROLL = 4096

def private get_flatten_max_unroll(func : FunctionPtr) : int {
    //! Reads the opt-in `[flatten(max_unroll=N)]` argument: the cap on the product of
    //! nested constant-loop iteration counts the twin may unroll to. Exceeding it is a
    //! clean compile error, not an exponential-expansion blowup. Default FLATTEN_MAX_UNROLL.
    for (ann in func.annotations) {
        if (ann.annotation.name == "flatten") {
            let mv = find_arg(ann.arguments, "max_unroll")
            if (mv is tInt) return mv as tInt
        }
    }
    return FLATTEN_MAX_UNROLL
}

// ===== Predicated lowering (return-elim + if-conversion + call inlining) =====
//
// Shared state lives on FlatCtx (one instance per twin, threaded by pointer so
// the gensym counter and recursion worklist are shared across inline frames).
// liveName/retName/hasRet are the CURRENT frame; inline_call saves & restores
// them when it descends into a callee. structPred is the AND of enclosing
// if-conditions on the path (null == statically true).

// One stack entry per enclosing flattened loop (innermost last). `loopLive` is the
// break mask (declared once, persists across unrolled copies); `iterLive` is the
// continue mask (re-declared `= true` at each copy head). Either is "" when that
// flavor is absent in the loop body — live_pred skips empty names.
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

// The write predicate at the current point: the AND of every active mask
// (funcLive plus each enclosing loop's loopLive/iterLive) and structPred. Passing
// `exclude` drops that one mask from the AND — used to build a narrow term, where
// the target mask cancels itself out (`M = M && !(rest)`). Returns null if nothing
// remains (statically true), which the callers treat as `= false` on a narrow.
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
        // Lift the argument FIRST (under the caller's predicate): a nested user call
        // gets its own inline frame, a nested builtin is whitelist-checked. Without
        // this, `f(g(x))` would bind `var __flat_arg = g(x)` verbatim — leaving `g`
        // un-inlined and any non-whitelisted builtin in the arg unchecked.
        let lifted = lift_expr(ctx, call.arguments[ai], structPred, out)
        out |> push <| qmacro_expr(${ var $i(argN) = $e(lifted); })
        subst |> replaceVariable(string(p.name)) <| qmacro($i(argN))
    }
    // Uniquify the callee's own locals per inline. Inlining the same helper at
    // multiple sites — distinct call sites, or one site multiplied by loop
    // unrolling — would otherwise emit colliding `var` decls in the single
    // flattened scope (error[30704]). Mirrors lower_for's per-copy local rename.
    var seenLocals : array<string>
    let calleeLocals <- collect_let_names(callee.body)
    for (ln in calleeLocals) {
        if (!(seenLocals |> contains(ln))) {
            seenLocals |> push(ln)
            subst |> renameVariable(ln, fresh(ctx, "__flat_loc"))
        }
    }
    // The callee runs where the caller's full predicate holds — funcLive AND every
    // enclosing loop's break/continue masks AND structPred — all baked into the frame
    // seed. The callee body then lowers with this seed as its base mask and an EMPTY
    // loop stack (its returns narrow only this frame; it cannot break the caller's loop).
    let seed = live_pred(ctx, structPred)
    out |> push <| qmacro_expr(${ var $i(liveN) = $e(seed); })
    // Result temp only for value-returning callees. A void callee is inlined
    // for its side effects (e.g. predicated writes to output globals); it has
    // no value to bind, so emitting `default<void>` would be a type error.
    var retN = ""
    if (!isVoidCallee) {
        retN = fresh(ctx, "__flat_ret")
        var crt = clone_type(callee.result)
        crt.flags.constant = false
        // Bare decl: das auto-inits to default<T>, so the explicit init is
        // redundant. The first write self-reads the auto-init value.
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

// Lift every user call out of a value expression (at any depth) into inline
// frames, in evaluation order, returning the call-free rewrite. Plain
// recursion — NOT a visitor — because inlining a call recurses back through
// the lowering pipeline, and nested make_visitor traversals corrupt the
// visitor machinery. Operand order matches daslang left-to-right evaluation.
def private lift_expr(var ctx : FlatCtx?; e : ExpressionPtr; structPred : ExpressionPtr; var out : array<ExpressionPtr>) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprCall) {
        let c = e as ExprCall
        let bname = string(c.name)
        // Whitelist takes precedence over inlining: a whitelisted leaf primitive is
        // KEPT (args lifted), even if it has a body. Backend [hint] primitives are
        // [stub] functions — non-null (empty) body — so without this they'd be
        // treated as user calls and inlined away to nothing. Match the SIMPLE name:
        // OR-type generics monomorphize to `<scope>`<name>`<hash>`, but the backend
        // whitelists by the bare name.
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
        // A pure builtin with all-constant arguments (e.g. `float(2)`, `float3(0.5)`,
        // `min(2,3)`) is a compile-time constant — eval_to_const collapses it at the
        // post-infer fold, so no runtime primitive reaches the backend and the
        // (runtime-primitive) whitelist does not apply. Keep it, lifting any nested
        // calls in its args. Gated to the evaluator's foldable types so a non-foldable
        // const builtin still errors here rather than leaking past the fold.
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
    // Member / index / cast / constructor expressions: a user call can hide inside
    // the operand of a swizzle (`f(x).xyz`), a struct field read (`f(x).a`), a cast,
    // an index, or a constructor field (`Out(emission = f(x))`). Recurse into the
    // operands so the call still gets inlined — without these, the call survives
    // lowering and the post-pass verify scan rejects it as an "internal error".
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
    // Typer-inserted ref/value/pointer conversion wrappers (e.g. ExprRef2Value
    // around a field read `f(x).a`). Recurse through to reach the buried call.
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

// Lower a `break` (isBreak) or `continue` by narrowing the innermost loop's mask.
// The narrow term excludes the target mask (it cancels) and is gated by every other
// active mask + structPred, so a prior return/break/continue on the same path
// correctly suppresses this one.
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
        // Narrow funcLive on the executing path. The term is every OTHER active mask
        // (enclosing loop masks) AND structPred — funcLive itself cancels. With no
        // enclosing loop this reduces to the old `funcLive = funcLive && !structPred`.
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
        // A local declaration always executes; inline any user-call initializer
        // (its frame already encodes the path predicate via the live mask).
        var letClone = clone_expression(stmt)
        var lc = letClone as ExprLet
        for (v in lc.variables) {
            if (v.init != null) {
                v.init = lower_value(ctx, v.init, structPred, out)
            }
        }
        out |> push <| letClone
    } elif (stmt is ExprFor) {
        lower_for(ctx, stmt as ExprFor, structPred, out)
    } elif (stmt is ExprWhile) {
        ctx.ok = false
        ctx.err = "flatten: while loops are not supported; use a fixed range(N) or array-literal for loop"
    } elif (stmt is ExprBreak) {
        // Narrow the innermost loop's break mask: it persists across unrolled copies,
        // so once a copy breaks, the rest of it and every later copy mask off.
        lower_loop_exit(ctx, structPred, out, true)
    } elif (stmt is ExprContinue) {
        // Narrow the innermost loop's per-iteration mask: the rest of THIS copy masks
        // off; the next copy re-declares it `= true`.
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
        // `x++` / `x--` as a statement → `x = P ? (x +/- 1) : x` (the postfix value
        // is discarded at statement level, so pre/post is irrelevant here). Same
        // drop-if-unhandled hazard as compound assignment; PERF013 nudges toward these.
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
        // `with (x) { body }` is pure infer-time name resolution: post-infer the
        // body's accesses are already bound to x's fields, and the node simulates
        // to its body alone (ast_simulate.cpp: visit(ExprWith) = body). So unwrap
        // it — lower the body under the same predicate, exactly like a bare block.
        lower_branch(ctx, (stmt as ExprWith).body, structPred, out)
    } else {
        // Bare expression statement (a discarded value). Lifting emits any inline
        // frames and their side effects (global writes, etc.) as their own
        // statements; the residual value is unused, so it is NOT emitted. A bare
        // pure expression is dead — and e.g. `x * x` as a statement is a hard
        // compile error (30151 "no side effect operation"). Copy-prop would
        // otherwise fold a discarded result temp into exactly that.
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

// Does THIS loop's body contain a break / continue that targets THIS loop? A
// nested for/while owns its own break/continue, so we only count those at nesting
// depth 0 (the depth counter ignores any inside a deeper loop). Drives whether
// lower_for allocates the loopLive (break) / iterLive (continue) masks at all —
// a loop with neither stays byte-identical to the pre-M4.1 output.
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
        // Array-literal source: `for (v in [a, b, c])` parses as
        // `to_array_move([[a, b, c]])` (ExprCall wrapping ExprMakeArray), or a bare
        // ExprMakeArray. Each element expression substitutes for one iteration.
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

// Fixed-count loop unrolling (M4). A `for` over a constant `range(N)` or an array
// literal has a compile-time-known iteration count: clone the body once per
// iteration, substitute the loop variable with that iteration's constant, and lower
// each unrolled copy under the SAME predicate (an unrolled body always executes).
// Per-iteration locals are renamed (`o` → `o_0`, `o_1`, …) so the copies don't
// shadow each other in the flattened scope. Mirrors daslib/unroll.das. break /
// continue (M4.1) allocate runtime masks — loopLive (persists across copies) for
// break, a per-copy iterLive for continue — pushed on ctx.loopMasks for the body to
// narrow; absent both, the loop is byte-identical to the pre-M4.1 output. while
// loops and non-constant sources error.
def private lower_for(var ctx : FlatCtx?; efor : ExprFor?; structPred : ExpressionPtr; var out : array<ExpressionPtr>) {
    let nIter = length(efor.iterators)
    if (nIter == 0 || length(efor.sources) != nIter) {
        ctx.ok = false
        ctx.err = "flatten: a for loop needs one source per iterator"
        return
    }
    // Build the per-iteration value list for each source. Multiple iterators
    // (`for (a, b in [...], [...])`) unroll in lockstep, so every source must
    // yield the same count. perSource[v][k] is iterator v's value for copy k.
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
    // `count` is the (shared) iteration count of every source.
    // Unrolling multiplies across nesting: this loop emits `count` body copies
    // per enclosing iteration, so the running product (int64, overflow-safe) is the true
    // expansion. Bail BEFORE the clones if it exceeds the cap — converts a heap blowup
    // into a clean error. Restore the factor on exit so sibling loops start fresh.
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
        // Substitute every iterator with its value for copy k (one iterator for a
        // plain loop, several for `for (a, b in [...], [...])` unrolled in lockstep).
        for (vi in range(nIter)) {
            let van = "{efor.iterators[vi]}"
            remove_deref(van, iblk)            // strip ExprRef2Value around the workhorse loop var
            rules |> replaceVariable(van) <| perSource[vi][k]
        }
        // Rename this copy's locals uniquely so the unrolled bodies don't shadow
        // each other once flattened into one scope.
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
    //! Flatten `func`'s body in place to branchless, call-free form: every user
    //! call inlined, all control flow lowered to predicated `?:` selects, early
    //! `return` to a runtime live mask. `whitelist` is the set of leaf-builtin
    //! names permitted to survive into the output (must be pure / branch-safe /
    //! backend-expressible). `maxUnroll` caps the product of nested constant-loop
    //! iteration counts (exceeding it is a clean error, not a heap blowup). Returns
    //! the FlatCtx — inspect `.ok` and `.err`.
    //!
    //! Backends call this directly with their own primitive set, before their own
    //! macro walks the result; `[flatten]` calls it with the std default. The
    //! body is uninferred after this — re-infer (e.g. via patch + astChanged)
    //! before reading types or running the shape check.
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
            // The return accumulator is read self-referentially by every `__flat_ret = pred
            // ? v : __flat_ret` write (lower_stmt), so its first read is the bare decl. That
            // read is always a discarded placeholder — overwritten by the taken return, or
            // kept only on the not-taken path — so the uninitialized read is safe by design.
            // copy_prop's mask-fold collapses it away when the live mask folds (single return), but
            // a live early-return leaves the self-ref intact, and `var x : Struct` is otherwise
            // rejected (error[31016]) for any struct return type.
            // rt.flags.safeWhenUninitialized = true
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

// ===== Post-infer select folding (infer_type_fold) =====
//
// The typer folds constant comparisons during re-infer (`0 >= 2` → false) but
// leaves the wrapping `?:` in place: at infer time it lacks the side-effect flags
// to prove the dead arm droppable, so a const-condition select survives — and a
// predicated self-assign then collapses (post-typer) to `acc = acc`, which a
// dataflow backend cannot resolve (an accumulator that reads itself with no
// producer). This pass runs AFTER re-infer, where the conditions are already
// `ExprConstBool`: it collapses every `const ? a : b` to the live arm (full tree,
// so deeply-buried selects fold too), drops the resulting `lhs = lhs` self-assigns
// whose lvalue is provably side-effect-free, and strips redundant zero/default
// initializers (`var acc = 0` → `var acc : int`). The cleanups the compiler will
// not do at infer time, done here with the lint side-effect estimate.

[macro_function]
def public flatten_fold(var func : FunctionPtr) : bool {
    //! Post-infer select-folding pass: collapse `const ? a : b` to the live arm, drop
    //! the side-effect-free `lhs = lhs` self-assigns left behind by a folded false-select,
    //! and strip redundant zero/default initializers (`var acc = 0` → `var acc : int`).
    //! Run it AFTER re-inference — the constant conditions must already be `ExprConstBool`,
    //! which only the typer produces (and the var types must be settled). Returns true if anything
    //! changed (the caller should then re-infer for clean codegen). `[flatten]` runs
    //! this automatically between lowering and the shape check; backends calling
    //! `flatten_function` run it after their own re-infer, before consuming the twin.
    return fold_body(func)
}

// ===== Post-inference shape verification =====
//
// After lowering + re-infer, the twin must be branchless and call-free: no
// ExprIfThenElse (all if/else became `?:` selects) and no surviving user call
// (all inlined). A violation is a lowering bug — fail loudly rather than hand a
// branchful/callful twin to a backend that cannot represent it. Runs in cycle 2
// (post-infer) because user-call detection needs the resolved `call.func`.
//
// Fold completeness (a missed const-fold) is NOT checked here — that is a
// suboptimal-but-correct outcome, validated post-compile by the test framework
// via `flatten_fold_residuals`, not gated at transform time.

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
//
// `flatten_fold_residuals` is the ONLY fold-completeness check — there is no compile-time
// flag, because a missed fold is suboptimal, not wrong. It walks a COMPILED twin's final
// AST and collects every const-foldable residual a complete fold should have collapsed —
// an unconditional algebraic identity (`x*1`/`x+0`/`x-0`/`x*-1`, scalar or vector), an
// all-const call of an eval-foldable type, a const-condition `?:`, a const `!`. Empty
// result = clean. It runs over the ACTUAL compiled output (not at transform time), so it
// covers the `[pixel_shader]` backend path uniformly with `[flatten]`. (Branch / user-call
// survivors are a DIFFERENT class — a lowering failure — and stay a hard compile error in
// patch via verify_flat; they never reach a successfully compiled unit.)

class private FoldResidualVisitor : AstVisitor {
    found : array<string>
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
    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (is_unfolded_identity_op2(expr)) {
            found |> push("an unfolded algebraic identity ('{expr.op}')")
        }
    }
    def override preVisitExprOp1(expr : ExprOp1?) : void {
        if (expr.op == "!" && expr.subexpr is ExprConstBool) {
            found |> push("an unfolded constant '!'")
        }
    }
}

def public flatten_fold_residuals(var func : FunctionPtr) : array<string> {
    //! Collect the const-foldable residuals left in `func`'s (post-flatten) body — empty
    //! means the fold is complete. For test-framework / fuzzer verification of a compiled
    //! flattened twin; reuses the same predicates the macro's own fold uses.
    var res : array<string>
    if (func == null || func.body == null) {
        return <- res
    }
    var v = new FoldResidualVisitor()
    make_visitor(*v) $(adapter) {
        visit(func.body, adapter)
    }
    res <- v.found
    unsafe {
        delete v
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
        // Lint runs with constant folding off, so `range(N)` is not yet an
        // ExprConstRange and the generated twin is irrelevant to linting the source
        // anyway. Leave the twin as the inert clone from `apply` (mirrors how
        // daslib/unroll.das skips unrolling under is_in_lint_check()).
        if (is_in_lint_check()) return true
        let ph = get_flatten_phase(func)
        if (ph == PHASE_DONE) return true
        var twin = find_unique_function(compiling_module(), twin_name(func), true)
        if (twin == null) {
            errors := "flatten: twin {twin_name(func)} was not declared"
            return false
        }
        if (ph != PHASE_LOWERED && ph != PHASE_FOLDED) {
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
            // Cycle 2+: the twin is inferred — collapse const-condition selects, fold
            // algebraic identities, and drop the resulting pure self-assigns. Re-infer
            // if anything changed so the verify scan and codegen see clean types.
            //
            // Iterate to a fixpoint: flatten_fold folds runtime-operand identities the
            // typer will not (`0*b → 0`, `x*-1 → -x`); the re-infer between passes then
            // const-folds the freshly-constant operands (`24 >> (24 & 31) → 0`), which
            // can expose a new identity (`x - 0`) for the next pass. A single fold pass
            // is therefore not enough — re-run until nothing folds.
            let folded = flatten_fold(twin)
            set_flatten_phase(func, PHASE_FOLDED)
            if (folded) {
                astChanged = true
                return true
            }
            // Nothing left to fold — fall through to the shape check this cycle.
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
