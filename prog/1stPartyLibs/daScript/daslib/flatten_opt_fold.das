options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_fold shared private

//! Post-infer select-fold: FoldVisitor (identities, const-fold, mad/lerp disassembly), constant reassociation, same-condition select-chain fusion.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require daslib/flatten_opt_common
require daslib/flatten_opt_swizzle


// radians/degrees are each exactly one float32 multiply by these conversion constants —
// the bit pattern the C++ helpers (aot_builtin_math.h radians_float/degrees_float) fold to,
// verified bit-identical to evaluating the builtin at 1.0 (0x3c8efa35 / 0x42652ee0).
let private DEG_TO_RAD = 0.017453292f
let private RAD_TO_DEG = 57.295776f

class private FoldVisitor : AstVisitor {
    changed : bool
    noFastMath : bool
    expandLerp : bool
    def override visitExprOp2(var that : ExprOp2?) : ExpressionPtr {
        // A fully-constant ExprOp2 the typer left unfolded — it folds scalar const arithmetic but leaves vector const (`float3(2)+float3(3)`), what reassoc's gathered groups become. Fold to one literal via the evaluator; an un-inferred node is skipped (folded next re-infer).
        if (is_all_const(that) && eval_to_const_supported(that._type)) {
            var folded = eval_to_const(that)
            if (folded |> is_expr_const()) { changed = true; return folded }
        }
        // Algebraic identities over the scalar + vector (float/int/uint) family, under fast-math.
        match (that.op) {
            if ("*") {
                if (is_one_const_op(that.right) && same_base_type(that, that.left)) { changed = true; return that.left }
                if (is_one_const_op(that.left) && same_base_type(that, that.right)) { changed = true; return that.right }
                if (is_neg_one_const_op(that.right) && same_base_type(that, that.left)) { changed = true; return negate(that.left) }
                if (is_neg_one_const_op(that.left) && same_base_type(that, that.right)) { changed = true; return negate(that.right) }
                // `*0` changes inf/NaN propagation — float forms are fast-math only (int exact)
                if (is_zero_const_op(that.right) && !has_sideeffects(that.left) && (!noFastMath || !is_float_valued(that))) {
                    var z = zero_const_of(that._type, that.at)
                    if (z != null) { changed = true; return z }
                }
                if (is_zero_const_op(that.left) && !has_sideeffects(that.right) && (!noFastMath || !is_float_valued(that))) {
                    var z = zero_const_of(that._type, that.at)
                    if (z != null) { changed = true; return z }
                }
                // ctor-lane kill: a zero lane of a const vector factor zeroes the matching lane of a full-arity ctor factor IN PLACE — the lane compute dies (DSE) without distributing the multiply. Same inf/NaN class as scalar `*0` → float forms are fast-math only.
                if (!noFastMath || !is_float_valued(that)) {
                    if (ctor_lane_kill(that.left, that.right) || ctor_lane_kill(that.right, that.left)) {
                        changed = true
                        return that
                    }
                }
                // splat collapse: `v * floatN(s) → v * s` (either side) — das vec×scalar is native, the splat ctor node disappears. Bit-exact (same per-lane product), so never gated.
                var spm = splat_scalar_arg(that.right)
                if (spm != null && is_float_vec(expr_bt(that.left))) {
                    changed = true
                    that.right = spm
                    return that
                }
                spm = splat_scalar_arg(that.left)
                if (spm != null && is_float_vec(expr_bt(that.right))) {
                    changed = true
                    that.left = spm
                    return that
                }
            }
            if ("+") {
                if (is_zero_const_op(that.right)) { changed = true; return that.left }
                if (is_zero_const_op(that.left)) { changed = true; return that.right }
            }
            if ("-") {
                if (is_zero_const_op(that.right)) { changed = true; return that.left }
                if (is_zero_const_op(that.left)) { changed = true; return negate(that.right) }
                // `x - x → 0` — purity-gated like `*0` (and like it, inf/NaN operands aside:
                if (describe(that.left) == describe(that.right) && !has_sideeffects(that.left) &&
                        (!noFastMath || !is_float_valued(that))) {
                    var z = zero_const_of(that._type, that.at)
                    if (z != null) { changed = true; return z }
                }
            }
            if ("/") {
                // `x / C → x * (1/C)` — a mul instead of a div; the reciprocal rounds (fast-math).
                if (!noFastMath && is_float_valued(that)) {
                    var rec = recip_const(that.right)
                    if (rec != null) {
                        changed = true
                        return new ExprOp2(at = that.at, op := "*", left = that.left, right = rec)
                    }
                }
                // splat collapse for division: `v / floatN(s) → v / s`, `floatN(s) / v → s / v` — both vec÷scalar forms are native das. Bit-exact, never gated.
                var spd = splat_scalar_arg(that.right)
                if (spd != null && is_float_vec(expr_bt(that.left))) {
                    changed = true
                    that.right = spd
                    return that
                }
                spd = splat_scalar_arg(that.left)
                if (spd != null && is_float_vec(expr_bt(that.right))) {
                    changed = true
                    that.left = spd
                    return that
                }
            }
            if ("&&") {
                // Short-circuit `&&`: `false && x` never evaluates x, so dropping x is safe; `x && false` DOES evaluate x, so that drop is gated on purity.
                if (is_bool_val(that.left, true)) { changed = true; return that.right }
                if (is_bool_val(that.right, true)) { changed = true; return that.left }
                if (is_bool_val(that.left, false)) { changed = true; return that.left }
                if (is_bool_val(that.right, false) && !has_sideeffects(that.left)) { changed = true; return that.right }
            }
            if ("||") {
                // Mirror of `&&`: `true || x` never evaluates x; `x || true` does.
                if (is_bool_val(that.left, false)) { changed = true; return that.right }
                if (is_bool_val(that.right, false)) { changed = true; return that.left }
                if (is_bool_val(that.left, true)) { changed = true; return that.left }
                if (is_bool_val(that.right, true) && !has_sideeffects(that.left)) { changed = true; return that.right }
            }
        }
        return that
    }
    def override visitExprOp1(var that : ExprOp1?) : ExpressionPtr {
        // Fold const `!`; else push `!` into the operand via negate_bool (flip a comparison, drop a double `!`, De-Morgan) — this collapses the `!comparison` the select-fold above emits.
        if (that.op == "!") {
            if (is_bool_val(that.subexpr, true)) { changed = true; return new ExprConstBool(at = that.at, value = false) }
            if (is_bool_val(that.subexpr, false)) { changed = true; return new ExprConstBool(at = that.at, value = true) }
            var neg = negate_bool(that.subexpr, noFastMath)
            if (neg != null) { changed = true; return neg }
        }
        return that
    }
    def override visitExprOp3(var that : ExprOp3?) : ExpressionPtr {
        // Post-order: subexpr/arms already folded, so a now-constant condition collapses to its live arm (the dead arm + condition become unreachable).
        if (that.subexpr is ExprConstBool) {
            changed = true
            return (that.subexpr as ExprConstBool).value ? that.left : that.right
        }
        // Bool-const arms collapse to the condition: `c ? true : false → c`, `c ? false : true → !c`. Common after if-conversion of a bool-valued branch.
        if (is_bool_val(that.left, true) && is_bool_val(that.right, false)) {
            changed = true
            return that.subexpr
        }
        if (is_bool_val(that.left, false) && is_bool_val(that.right, true)) {
            changed = true
            return new ExprOp1(at = that.at, op := "!", subexpr = that.subexpr)
        }
        // Equal arms (`c ? a : a → a`) — e.g. after `acc + 0 → acc` makes both arms `acc`.
        if (describe(that.left) == describe(that.right) && !has_sideeffects(that.subexpr)) {
            changed = true
            return that.left
        }
        return that
    }
    def override visitExprCall(var that : ExprCall?) : ExpressionPtr {
        // A fully-constant, side-effect-free call (vector constructor or pure builtin over const args) folds to one literal. The typer folds const ARITHMETIC but leaves const CONSTRUCTORS; this collapses them. Post-order, so const-folded args are visible.
        if (is_all_const(that)) {
            var folded = eval_to_const(that)
            if (!(folded is ExprCall)) {   // eval_to_const returns the call unchanged on failure
                changed = true
                return folded
            }
        }
        // Lower radians/degrees to their defining `x * K` (bit-exact, K = the builtin at 1.0) so a backend needs only `mul` and an adjacent const factor merges via reassoc.
        if (that.func != null && that.func.flags.builtIn && length(that.arguments) == 1
                && is_float_family(expr_bt(that.arguments[0]))) {
            let nm = simple_name("{that.func.name}")
            if (nm == "radians" || nm == "degrees") {
                changed = true
                let k = nm == "radians" ? DEG_TO_RAD : RAD_TO_DEG
                return new ExprOp2(at = that.at, op := "*", left = that.arguments[0],
                    right = new ExprConstFloat(at = that.at, value = k))
            }
        }
        // DISASSEMBLE mad (and, under `expandLerp`, lerp) into raw arithmetic so the folds, reassoc and CSE see through them; PHASE_FUSED re-packs survivors. Type-gated to the das math overload set — a loose DSL-stub instantiation must not expand ill-typed.
        if (that.func != null && length(that.arguments) == 3) {
            let fname = simple_name("{that.func.name}")
            let ta = expr_bt(that.arguments[0])
            let tb = expr_bt(that.arguments[1])
            let tc = expr_bt(that.arguments[2])
            if (fname == "lerp" && lerp_shape(ta, tb, tc)) {
                var aArg = that.arguments[0]
                var bArg = that.arguments[1]
                var tArg = that.arguments[2]
                // const-selector short-circuits — no expand/re-fuse round trip. They DROP an argument (`(b-a)*0` would still propagate b's inf/NaN) → fast-math only.
                if (!noFastMath) {
                    if (is_zero_const_op(tArg) && !has_sideeffects(bArg)) { changed = true; return aArg }
                    if (is_one_const_op(tArg) && !has_sideeffects(aArg)) { changed = true; return bArg }
                    if (describe(aArg) == describe(bArg) && !has_sideeffects(bArg) && !has_sideeffects(tArg)) {
                        changed = true
                        return aArg
                    }
                }
                if (!expandLerp) return that
                changed = true
                var sub = new ExprOp2(at = that.at, op := "-", left = bArg, right = clone_expression(aArg))
                var mul = new ExprOp2(at = that.at, op := "*", left = sub, right = tArg)
                return new ExprOp2(at = that.at, op := "+", left = mul, right = aArg)
            }
            if (fname == "mad" && mad_shape(ta, tb, tc)) {
                changed = true
                var mul = new ExprOp2(at = that.at, op := "*", left = that.arguments[0], right = that.arguments[1])
                return new ExprOp2(at = that.at, op := "+", left = mul, right = that.arguments[2])
            }
        }
        return that
    }
    def override visitExprSwizzle(var that : ExprSwizzle?) : ExpressionPtr {
        var r = swizzle_fold_rewrite(that)
        if (r != null) {
            changed = true
            return r
        }
        return that
    }
}

// True if `e` is an op2 FoldVisitor's identity arms would rewrite away (`x*1`/`x*-1`/`x+0`/`x-0`,
// scalar or vector const). MIRRORS the fold arms exactly — `*1`/`*-1` carry the `same_base_type`
// gate, `+`/`-` don't, `x*0` is excluded (purity-gated, so a survivor isn't a miss).
def public is_unfolded_identity_op2(e : ExprOp2?) : bool {
    if (e == null) return false
    if (e.op == "*") {
        return (((is_one_const_op(e.right) || is_neg_one_const_op(e.right)) && same_base_type(e, e.left)) ||
            ((is_one_const_op(e.left) || is_neg_one_const_op(e.left)) && same_base_type(e, e.right)))
    }
    if (e.op == "+" || e.op == "-") {
        return (is_zero_const_op(e.right) || is_zero_const_op(e.left))
    }
    return false
}

def private same_base_type(a, b : ExpressionPtr) : bool {
    if (a == null || b == null || a._type == null || b._type == null) return false
    return a._type.baseType == b._type.baseType
}

// Push `!` down to negatable leaves (flip a comparison, drop `!!`, De-Morgan) and return the
// residual-`!`-free result; null when a wrapping `!` would remain. NaN charter: ordered float
// comparisons flip only under fast-math, equality flips are always exact.
def private negate_bool(var e : ExpressionPtr; noFastMath : bool) : ExpressionPtr {
    if (e is ExprConstBool) {
        return new ExprConstBool(at = e.at, value = !((e as ExprConstBool).value))
    }
    if (e is ExprOp1 && (e as ExprOp1).op == "!") {
        return (e as ExprOp1).subexpr               // !!x → x
    }
    if (!(e is ExprOp2)) return null
    var o = e as ExprOp2
    let ordered = o.op == ">" || o.op == "<" || o.op == ">=" || o.op == "<="
    // comparison → complement; skip a user/non-builtin op (no auto-complement) or a NaN-unsafe ordered float compare
    if (ordered || o.op == "==" || o.op == "!=") {
        if (o.func == null || !o.func.flags.builtIn || (ordered && noFastMath && is_float_valued(o.left))) {
            return null
        }
        var flip = ""
        match (o.op) {
            if (">")  { flip = "<=" }
            if ("<")  { flip = ">=" }
            if (">=") { flip = "<" }
            if ("<=") { flip = ">" }
            if ("==") { flip = "!=" }
            if ("!=") { flip = "==" }
        }
        return new ExprOp2(at = e.at, op := flip, left = o.left, right = o.right)
    }
    if (o.op == "&&" || o.op == "||") {        // De Morgan, only if both sides negate clean
        var nl = negate_bool(o.left, noFastMath)
        if (nl == null) return null
        var nr = negate_bool(o.right, noFastMath)
        if (nr == null) return null
        return new ExprOp2(at = e.at, op := o.op == "&&" ? "||" : "&&", left = nl, right = nr)
    }
    return null
}

// Read-only twin of negate_bool: would a wrapping `!` over `e` fold away? Used by the residual
// oracle (which must NOT call negate_bool — it reuses `e`'s children and would alias the live
// tree). Keep the foldability rules here in lockstep with negate_bool above.
def public can_negate_bool(e : ExpressionPtr; noFastMath : bool) : bool {
    if (e is ExprConstBool || (e is ExprOp1 && (e as ExprOp1).op == "!")) return true   // const / !!x
    if (!(e is ExprOp2)) return false
    let o = e as ExprOp2
    match (o.op) {
        if ("==" || "!=") { return o.func != null && o.func.flags.builtIn }
        if (">" || "<" || ">=" || "<=") {
            return o.func != null && o.func.flags.builtIn && !(noFastMath && is_float_valued(o.left))
        }
        if ("&&" || "||") {
            return can_negate_bool(o.left, noFastMath) && can_negate_bool(o.right, noFastMath)
        }
    }
    return false
}

// Zero the full-arity ctor lanes under a zero lane of the const multiplier — the lane compute
// dies (DSE) without distributing the multiply. Skips already-zero lanes (fixpoint). NO purity
// gate: flat lanes are pure by contract, and macro-time has_sideeffects calls DSL stubs impure.
def private ctor_lane_kill(var ctorE : ExpressionPtr; constE : ExpressionPtr) : bool {
    if (ctorE == null || !(ctorE is ExprCall)) return false
    var c = ctorE as ExprCall
    let d = ctor_dim_by_name(c)
    if (d == 0) return false
    let m = const_vec_zero_mask(constE)
    if (m == 0) return false
    var any = false
    for (i in range(d)) {
        if ((m & (1 << i)) != 0 && !is_zero_const(c.arguments[i])) {
            var z = zero_const_of(c.arguments[i]._type, c.arguments[i].at)
            if (z != null) {
                c.arguments[i] = z
                any = true
            }
        }
    }
    return any
}

// Oracle mirrors for the ctor fold arms (FoldResidualVisitor in flatten.das).
def public is_uncollapsed_splat_op2(var e : ExprOp2?) : bool {
    if (e == null || (e.op != "*" && e.op != "/")) return false
    return ((splat_scalar_arg(e.left) != null && is_float_vec(expr_bt(e.right))) ||
        (splat_scalar_arg(e.right) != null && is_float_vec(expr_bt(e.left))))
}

def public is_unkilled_ctor_zero_lane(var e : ExprOp2?; no_fast_math : bool = false) : bool {
    // the float gate mirrors the kill arm's
    if (e == null || e.op != "*" || (no_fast_math && is_float_valued(e))) return false
    var ctorE = e.left
    var constE = e.right
    if (const_vec_zero_mask(constE) == 0) {
        ctorE = e.right
        constE = e.left
    }
    if (!(ctorE is ExprCall)) return false
    var c = ctorE as ExprCall
    let d = ctor_dim_by_name(c)
    if (d == 0) return false
    let m = const_vec_zero_mask(constE)
    if (m == 0) return false
    for (i in range(d)) {
        if ((m & (1 << i)) != 0 && !is_zero_const(c.arguments[i])) return true
    }
    return false
}

def private body_read_count(var blk : ExprBlock?; nm : string) : int {
    var total = 0
    for (j in range(length(blk.list))) {
        total += count_refs(blk.list[j], nm)
    }
    return total
}

def private const_prop_locals(var blk : ExprBlock?) : bool {
    var changed = false
    var constmap : table<string; ExpressionPtr>
    var i = 0
    while (i < length(blk.list)) {
        // Substitute already-known constants into this statement's reads.
        if (!empty(constmap) && reads_any(blk.list[i], constmap)) {
            blk.list[i] = subst_consts(blk.list[i], constmap)
            changed = true
        }
        // A single-def user local with a now-literal init folds to a constant and joins the map.
        var s = blk.list[i]
        if (s is ExprLet && length((s as ExprLet).variables) == 1) {
            var lc = s as ExprLet
            if (!is_flat_temp(lc.variables[0].name) && lc.variables[0].init != null && is_all_const(lc.variables[0].init)) {
                var oldInit = lc.variables[0].init
                if (oldInit |> is_expr_const()) {
                    // Already a literal (e.g. a fold-produced `0f`): map it directly so its uses inline.
                    constmap["{lc.variables[0].name}"] = oldInit
                    changed = true
                } else {
                    // A const expression (`2.0 * 3.0`): eval to its literal, folding the init in place.
                    var folded = eval_to_const(oldInit)
                    if (folded |> is_expr_const() && folded != oldInit) {
                        lc.variables[0].init = folded
                        constmap["{lc.variables[0].name}"] = folded
                        changed = true
                    }
                }
            }
        }
        i ++
    }
    // Drop the now-dead decls of fully-propagated locals (every use was substituted).
    if (!empty(constmap)) {
        var dead : table<int>
        for (j in range(length(blk.list))) {
            let s = blk.list[j]
            if (s is ExprLet && length((s as ExprLet).variables) == 1) {
                let nm = "{(s as ExprLet).variables[0].name}"
                if (key_exists(constmap, nm) && body_read_count(blk, nm) == 0) {
                    dead |> insert(j)
                }
            }
        }
        if (!empty(dead)) {
            // blk.list is a managed vector — erase high-to-low so indices stay valid.
            var si = length(blk.list) - 1
            while (si >= 0) {
                if (key_exists(dead, si)) {
                    blk.list |> erase(si)
                }
                si --
            }
            changed = true
        }
        delete dead
    }
    return changed
}

// True if the chain rooted at `e` (walked over `addOp`'s monoid: `+`/`-` for "+", `*` for "*")
// has any const leaf. Backs the already-grouped check.
def private chain_has_const(e : ExpressionPtr; addOp : string) : bool {
    if (e == null) return false
    if (e is ExprOp2) {
        let o = e as ExprOp2
        let isChainOp = addOp == "+" ? (o.op == "+" || o.op == "-") : (o.op == addOp)
        if (isChainOp) {
            return chain_has_const(o.left, addOp) || chain_has_const(o.right, addOp)
        }
    } elif (e is ExprOp1) {
        let o = e as ExprOp1
        if (addOp == "+" && o.op == "-") {
            return chain_has_const(o.subexpr, addOp)
        }
    }
    return is_all_const(e)
}

// The canonical reassoc output is `<vars> addOp <all-const-group>` with no const in the var
// part. Recognizing it makes reassoc idempotent — a fired chain lands here and never re-fires,
// so the macro's fixpoint terminates. (The const group is folded separately by FoldVisitor.)
def private already_grouped(that : ExprOp2?; addOp : string) : bool {
    return that.op == addOp && is_all_const(that.right) && !chain_has_const(that.left, addOp)
}

// True if the var terms are already canonical: positives first (so build_chain emits `b - a`,
// never `-a + b` = neg + add, whose uniform `-a` would hoist into a `_preshader_ = -_preshader_`
// alias), describe-ascending per sign group. Mirrors the regroup sort exactly (idempotence).
def private vars_sorted(vars : array<ReTerm>) : bool {
    for (i in range(1, length(vars))) {
        if ((vars[i - 1].neg && !vars[i].neg) ||
            (vars[i - 1].neg == vars[i].neg && describe(vars[i].e) < describe(vars[i - 1].e))) {
            return false
        }
    }
    return true
}

// Partition `terms` into var/const and rebuild as `<vars> addOp <consts>`. Two canonicalizations:
// gather ≥2 scattered consts into one group, and sort the var terms into a canonical order (for a
// later CSE) — the sort gated on side-effect freedom. Null when neither applies / the chain is all-const.
def private regroup(that : ExprOp2?; var terms : array<ReTerm>; addOp, subOp : string) : ExpressionPtr {
    var vars : array<ReTerm>
    var consts : array<ReTerm>
    for (t in terms) {
        if (is_all_const(t.e)) {
            consts |> push(t)
        } else {
            vars |> push(t)
        }
    }
    if (empty(vars)) {
        return null
    }
    var pure = true
    for (t in vars) {
        if (has_sideeffects(t.e)) {
            pure = false
            break
        }
    }
    let needs_gather = length(consts) >= 2 && !already_grouped(that, addOp)
    let needs_sort = pure && length(vars) >= 2 && !vars_sorted(vars)
    if (!needs_gather && !needs_sort) {
        return null
    }
    if (pure) {
        sort(vars) $(a, b : ReTerm) : bool {
            if (a.neg != b.neg) {
                return !a.neg
            }
            return describe(a.e) < describe(b.e)
        }
    }
    var vt = build_chain(vars, that.at, addOp, subOp)
    if (empty(consts)) {
        return vt
    }
    var ct = build_chain(consts, that.at, addOp, subOp)
    return new ExprOp2(at = that.at, op := addOp, left = vt, right = ct)
}

// Count the var / const terms in a chain (walked over `addOp`'s monoid) without building a term
// list — const-friendly, since the residual oracle only needs the tallies.
def private count_terms(e : ExpressionPtr; addOp : string; var nv : int&; var nc : int&) {
    if (e == null) return
    if (e is ExprOp2) {
        let o = e as ExprOp2
        let isChainOp = addOp == "+" ? (o.op == "+" || o.op == "-") : (o.op == addOp)
        if (isChainOp) {
            count_terms(o.left, addOp, nv, nc)
            count_terms(o.right, addOp, nv, nc)
            return
        }
    } elif (e is ExprOp1) {
        let o = e as ExprOp1
        if (addOp == "+" && o.op == "-") {
            count_terms(o.subexpr, addOp, nv, nc)
            return
        }
    }
    if (is_all_const(e)) {
        nc ++
    } else {
        nv ++
    }
}

// True if reassoc WOULD gather this chain: ≥1 var term, ≥2 scattered consts, not already grouped.
// The fold-completeness oracle (flatten.das FoldResidualVisitor) mirrors the transform's exact gate
// through this — incl. `fast_math` (float chains stay scattered under `_flatten_no_fast_math`).
def public has_scattered_consts(that : ExprOp2?; no_fast_math : bool = false) : bool {
    if (that == null || (no_fast_math && is_float_valued(that))) return false
    var nv = 0
    var nc = 0
    if (that.op == "+" || that.op == "-") {
        if (already_grouped(that, "+")) return false
        count_terms(that, "+", nv, nc)
    } elif (that.op == "*") {
        if (already_grouped(that, "*")) return false
        count_terms(that, "*", nv, nc)
    } else {
        return false
    }
    return nv >= 1 && nc >= 2
}

class private ReassocVisitor : AstVisitor {
    changed : bool
    noFastMath : bool
    def override visitExprOp2(var that : ExprOp2?) : ExpressionPtr {
        // Both the const-gather and the canonical re-sort rebuild the chain — a float sum/product picks up a different association order, so floats are fast-math only.
        if (noFastMath && is_float_valued(that)) {
            return that
        }
        var res : ExpressionPtr = null
        if (that.op == "*") {
            var terms : array<ReTerm>
            collect_mul(that, terms)
            res = regroup(that, terms, "*", "/")
        } elif (that.op == "+" || that.op == "-") {
            var terms : array<ReTerm>
            collect_add(that, false, terms)
            res = regroup(that, terms, "+", "-")
        }
        if (res != null) {
            changed = true
            return res
        }
        return that
    }
}

// The select-let `var a = (c ? T : F)` at `s`: its var name (or "" if not a single-var select-let).
def private select_let_name(s : ExpressionPtr) : string {
    if (s is ExprLet && length((s as ExprLet).variables) == 1 &&
            sel3((s as ExprLet).variables[0].init) != null) {
        return "{(s as ExprLet).variables[0].name}"
    }
    return ""
}

// Safe to evaluate UNCONDITIONALLY? A stripped link's true arm runs even when the guard is false,
// so it must not fault or have side effects. Conservative: whitelist known-pure nodes and recurse;
// anything else (index OOB, null deref, integer div-by-zero, unknown) is unsafe — just not fused.
def private select_arm_eval_safe(e : ExpressionPtr) : bool {
    if (e == null || e is ExprVar || e |> is_expr_const()) return true   // null / var read / literal is pure
    if (e is ExprOp1) return select_arm_eval_safe((e as ExprOp1).subexpr)
    if (e is ExprOp2) {
        let o = e as ExprOp2
        if ((o.op == "/" || o.op == "%") && !is_float_valued(e)) return false
        return select_arm_eval_safe(o.left) && select_arm_eval_safe(o.right)
    }
    if (e is ExprOp3) {
        let o = e as ExprOp3
        return select_arm_eval_safe(o.subexpr) && select_arm_eval_safe(o.left) && select_arm_eval_safe(o.right)
    }
    if (e is ExprCall) {
        for (a in (e as ExprCall).arguments) {
            if (!select_arm_eval_safe(a)) return false
        }
        return true
    }
    if (e is ExprRef2Value) return select_arm_eval_safe((e as ExprRef2Value).subexpr)
    if (e is ExprSwizzle) return select_arm_eval_safe((e as ExprSwizzle).value)
    if (e is ExprField) return select_arm_eval_safe((e as ExprField).value)
    if (e is ExprCast) return select_arm_eval_safe((e as ExprCast).subexpr)
    if (e is ExprMakeTuple) {
        for (v in (e as ExprMakeTuple).values) {
            if (!select_arm_eval_safe(v)) return false
        }
        return true
    }
    return false   // ExprAt / ExprPtr2Ref / ExprNullCoalescing / unrecognized → conservatively unsafe
}

// Walk the def-use successor of `accName` (its sole referencing statement, since single-use) and,
// if that statement is a same-`condKey` select whose false arm is `accName`, return its index; else -1.
def private chain_successor(var ub : FlatUsesBuilder?; var blk : ExprBlock?; accName, condKey : string;
                            afterIdx : int; consumed : table<int>) : int {
    let hk = hash(accName)
    // single-use: accName referenced in exactly one statement (its successor)
    if (!key_exists(ub.nameUses, hk) || length(ub.nameUses[hk]) != 1) return -1
    let j = ub.nameUses[hk][0]
    // successor must be a later, unconsumed select-let
    if (j <= afterIdx || key_exists(consumed, j) || select_let_name(blk.list[j]) == "") return -1
    let succSel = sel3((blk.list[j] as ExprLet).variables[0].init)
    // …on the same condition, with its false arm = the running accumulator
    if (describe(succSel.subexpr) != condKey || ref_var_name(succSel.right) != accName) return -1
    return j
}

// The maximal same-condition select chain headed at `i` (or <2 when none). Shared by the fuse
// transform and its residual oracle so they can't drift. The condition must be STABLE — a
// mutable-reading condition (a narrowed loop mask) has a different value per link, so it's rejected.
def private build_select_chain(var ub : FlatUsesBuilder?; var blk : ExprBlock?; mutableVars : table<string>;
                               consumed : table<int>; i : int) : array<int> {
    var chain : array<int>
    let headName = select_let_name(blk.list[i])
    if (headName == "") return <- chain
    var headSel = sel3((blk.list[i] as ExprLet).variables[0].init)
    if (reads_mutable(headSel.subexpr, mutableVars)) return <- chain
    let condKey = describe(headSel.subexpr)
    chain |> push(i)
    var accName = headName
    while (true) {
        let j = chain_successor(ub, blk, accName, condKey, chain[length(chain) - 1], consumed)
        if (j < 0) break
        // stop if the new tail's now-unconditional true arm could fault / have a side effect
        let tailSel = sel3((blk.list[chain[length(chain) - 1]] as ExprLet).variables[0].init)
        if (!select_arm_eval_safe(tailSel.left)) break
        chain |> push(j)
        accName = "{(blk.list[j] as ExprLet).variables[0].name}"
    }
    return <- chain
}

def public fuse_select_chains(var blk : ExprBlock?) : bool {
    let n = length(blk.list)
    if (n < 2) return false
    var mutableVars <- collect_mutable(blk)
    var ub = new FlatUsesBuilder()
    make_visitor(*ub) $(adapter) {
        for (j in range(n)) {
            ub.j = j
            clear(ub.seen)
            visit(blk.list[j], adapter)
        }
    }
    var consumed : table<int>
    var changed = false
    for (i in range(n)) {
        if (key_exists(consumed, i)) continue
        var chain <- build_select_chain(ub, blk, mutableVars, consumed, i)
        if (length(chain) >= 2) {
            // base = the head select's false arm; clone it before re-parenting onto the tail select.
            var baseExpr = clone_expression(sel3((blk.list[chain[0]] as ExprLet).variables[0].init).right)
            for (ci in range(length(chain) - 1)) {     // strip every link but the last → its true arm
                var lc = blk.list[chain[ci]] as ExprLet
                lc.variables[0].init = sel3(lc.variables[0].init).left
                consumed |> insert(chain[ci])
            }
            let lastIdx = chain[length(chain) - 1]      // tail keeps its select; false arm := base
            sel3((blk.list[lastIdx] as ExprLet).variables[0].init).right = baseExpr
            consumed |> insert(lastIdx)
            changed = true
        }
        delete chain
    }
    delete consumed
    delete mutableVars
    unsafe {
        delete ub
    }
    return changed
}

// Read-only oracle mirror: true if a same-condition select chain (≥2 links) is still un-fused.
def public has_fusable_select_chain(var blk : ExprBlock?) : bool {
    let n = length(blk.list)
    if (n < 2) return false
    var mutableVars <- collect_mutable(blk)
    var ub = new FlatUsesBuilder()
    make_visitor(*ub) $(adapter) {
        for (j in range(n)) {
            ub.j = j
            clear(ub.seen)
            visit(blk.list[j], adapter)
        }
    }
    var consumed : table<int>   // always empty here — the oracle does not consume
    var found = false
    for (i in range(n)) {
        var chain <- build_select_chain(ub, blk, mutableVars, consumed, i)
        let isChain = length(chain) >= 2
        delete chain
        if (isChain) {
            found = true
            break
        }
    }
    delete consumed
    delete mutableVars
    unsafe {
        delete ub
    }
    return found
}

def public fold_body(var func : FunctionPtr; no_fast_math : bool = false; expand_lerp : bool = false) : bool {
    //! Post-infer fold over a flattened body: const-propagate single-def locals, reassociate scattered
    //! consts, collapse `const ? a : b`, fold identities/const ctors, drop pure self-assigns. SINGLE pass
    //! (caller loops); `no_fast_math` skips value-changing arms, `expand_lerp` disassembles `lerp` calls.
    if (!(func.body is ExprBlock)) return false
    let nfm = no_fast_math
    var changed = const_prop_locals(func.body as ExprBlock)
    // Hoist a shared condition out of an invariant-guarded accumulation chain (N selects → 1),
    // exposing the now-unconditional accumulation to reassoc / CSE / fuse below.
    if (fuse_select_chains(func.body as ExprBlock)) {
        changed = true
    }
    // Reassociate BEFORE the fold — gather scattered consts into one adjacent group so the
    // FoldVisitor all-const arm (and the typer's re-infer) can collapse them. After const-prop,
    // which is what scatters consts (a substituted single-def local lands mid-chain).
    var rv = new ReassocVisitor()
    rv.noFastMath = nfm
    make_visitor(*rv) $(adapter) {
        visit(func.body, adapter)
    }
    if (rv.changed) {
        changed = true
    }
    unsafe {
        delete rv
    }
    var v = new FoldVisitor(noFastMath = nfm, expandLerp = expand_lerp)
    make_visitor(*v) $(adapter) {
        visit(func.body, adapter)
    }
    if (v.changed) {
        changed = true
    }
    unsafe {
        delete v
    }
    // Cleanup 1 — drop pure self-assigns (`acc = acc`) a collapsed false-select leaves
    // behind. has_sideeffects gates it (`g[k()] = g[k()]` keeps — the lvalue isn't free).
    // Erase in reverse so earlier indices stay valid.
    var blk = func.body as ExprBlock
    var si = length(blk.list) - 1
    while (si >= 0) {
        var s = blk.list[si]
        if (s is ExprCopy) {
            let cp = s as ExprCopy
            if (describe(cp.left) == describe(cp.right) && !has_sideeffects(cp.left)) {
                blk.list |> erase(si)
                changed = true
            }
        }
        si --
    }
    // Cleanup 2 — strip a redundant zero/default init (`var acc = 0` → `var acc : int`). DEFERRED
    // to a converged pass: a const-folded local must keep its init until const_prop_locals
    // propagates it into uses; nulling it earlier strands the use. Sets changed → caller re-infers.
    if (!changed) {
        si = length(blk.list) - 1
        while (si >= 0) {
            var s = blk.list[si]
            if (s is ExprLet) {
                var lt = s as ExprLet
                for (lv in lt.variables) {
                    if (is_zero_const(lv.init)) {
                        lv.init = null
                        changed = true
                    }
                }
            }
            si --
        }
    }
    return changed
}

// Oracle mirror for the `x / C → x * (1/C)` fold arm: true when the (fast-math) rewrite would fire.
def public is_unrcp_const_div(e : ExprOp2?) : bool {
    if (e == null || e.op != "/" || !is_float_valued(e)) return false
    return recip_const(e.right) != null
}
