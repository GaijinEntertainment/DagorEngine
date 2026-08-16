options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_fuse shared private

//! PHASE_FUSED finishing pass: re-pack a*b +/- c into mad, the expanded lerp shape into lerp, lane sums into dot, and ctor lane patterns.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require strings
require daslib/flatten_opt_common


// A negated copy of a scalar/vector float/int const literal; null for any other node
// (uint excluded — modular negation is value-correct but backend-hostile).
def private negate_const(e : ExpressionPtr) : ExpressionPtr {
    if (e is ExprConstDouble) return new ExprConstDouble(at = e.at, value = -(e as ExprConstDouble).value)   // double: outside the 32-bit grid
    var L = const_lanes(e)
    if (L.kind == CK_FLOAT) { L.f = -L.f; return make_const_lanes(L, e.at) }
    if (L.kind == CK_INT) { L.i = -L.i; return make_const_lanes(L, e.at) }
    return null   // uint excluded — modular negation is value-correct but backend-hostile
}

def private has_arity_named(mod : Module?; fname : string; argc : int) : bool {
    var found = false
    for_each_function(mod, fname) $(fn) {
        if (length(fn.arguments) == argc) {
            found = true
        }
    }
    for_each_generic(mod, fname) $(fn) {
        if (length(fn.arguments) == argc) {
            found = true
        }
    }
    return found
}

def public fuse_callable(mod : Module?; fname : string; argc : int = 3) : bool {
    if (mod == null) return false
    if (has_arity_named(mod, fname, argc)) return true
    var found = false
    module_for_each_dependency(mod) $(dep, _isPub) {
        if (!found && has_arity_named(dep, fname, argc)) {
            found = true
        }
    }
    return found
}

// Match `m ± c` / `c ± m` (m = a*b; `-` needs a const-negatable side) against the das mad shapes;
// out-params carry (a, b, c) with the vec-scalar swap + negation applied. The ONE gate for fuser AND
// residual; no purity gate — flat-body calls are pure by contract (and has_sideeffects is timing-dependent).
def private match_mad(var that : ExprOp2?; var ma, mb, mc : ExpressionPtr&) : bool {
    if (that == null) return false
    var fa, fb, other : ExpressionPtr
    var negOther = false        // `m - C`  → c = -C
    var negFactor = false       // `C - m`  → negate a const factor of m
    if (qmatch(that, $e(fa) * $e(fb) + $e(other)).matched) {
        pass
    } elif (qmatch(that, $e(other) + $e(fa) * $e(fb)).matched) {
        pass
    } elif (qmatch(that, $e(fa) * $e(fb) - $e(other)).matched) {
        negOther = true
    } elif (qmatch(that, $e(other) - $e(fa) * $e(fb)).matched) {
        negFactor = true
    } else {
        return false
    }
    // types captured BEFORE negation — a fresh negated literal is untyped until re-infer
    var bta = expr_bt(fa)
    var btb = expr_bt(fb)
    let btc = expr_bt(other)
    if (negFactor) {
        var nf = negate_const(fa)
        if (nf != null) {
            fa = nf
        } else {
            nf = negate_const(fb)
            if (nf == null) return false
            fb = nf
        }
    }
    if (negOther) {
        other = negate_const(other)
        if (other == null) return false
    }
    // a scalar-left mul swaps to the MadS (vec, scalar, vec) form
    if (vec_elem_bt(btb) != Type.none && bta == vec_elem_bt(btb)) {
        var tmp = fa
        fa = fb
        fb = tmp
        let tmpbt = bta
        bta = btb
        btb = tmpbt
    }
    if (!mad_shape(bta, btb, btc)) return false
    ma = fa
    mb = fb
    mc = other
    return true
}

// `(-a) + b` / `b + (-a)` → `b - a`: one sub node instead of neg + add. Reassoc no longer emits
// the form (positives-first canonical order), so this catches the other producers — the `0 - x`
// and `*-1` fold arms. Shared by fuser AND residual.
def private match_neg_add(var that : ExprOp2?; var pos, neg : ExpressionPtr&) : bool {
    if (that == null || !is_mad_elem(expr_bt(that))) return false
    return (qmatch(that, -$e(neg) + $e(pos)).matched ||    // nolint:STYLE016 (qmatch macro expansion)
        qmatch(that, $e(pos) + -$e(neg)).matched)           // nolint:STYLE016 (qmatch macro expansion)
}

// `mad(b - a, t, a)` → `lerp(a, b, t)`: the re-packed expansion shape, `a` matched by
// describe-equality. The neg-add form is matched too (defense — reassoc no longer emits it).
// Deliberately misses a CSE-shared `b - a` (behind a `_cse_` var — keeping the share IS the win).
def private match_lerp(var cll : ExprCall?; var la, lb, lt : ExpressionPtr&) : bool {
    if (cll == null) return false
    var da, db, aa, tt : ExpressionPtr
    if (!qmatch(cll, mad($e(db) - $e(da), $e(tt), $e(aa))).matched &&     // nolint:STYLE016 (qmatch macro expansion)
        !qmatch(cll, mad(-$e(da) + $e(db), $e(tt), $e(aa))).matched &&    // nolint:STYLE016 (qmatch macro expansion)
        !qmatch(cll, mad($e(db) + -$e(da), $e(tt), $e(aa))).matched) return false  // nolint:STYLE016 (qmatch macro expansion)
    if (describe(da) != describe(aa) ||
        !lerp_shape(expr_bt(da), expr_bt(db), expr_bt(tt))) return false
    la = aa
    lb = db
    lt = tt
    return true
}

let private SIDE_NONE = 0
let private SIDE_EQUAL = 1       // every lane carries the SAME scalar (describe-equal)
let private SIDE_COMPONENT = 2   // lane i reads component_i of one shared vector base
let private SIDE_GATHER = 3      // arbitrary scalars — packed into a fresh ctor (or const literal)

// Vectorize one side (lefts or rights) of a ctor whose lanes are same-op ExprOp2: classify the
// d side-exprs and build the vector replacement from CLONES. SIDE_COMPONENT with the natural
// full mask collapses to the base itself (no swizzle node).
def private vectorize_side(var that : ExprCall?; d : int; takeLeft : bool; var node : ExpressionPtr&) : int {
    var sides : array<ExpressionPtr>
    sides |> reserve(d)
    for (i in range(d)) {
        var li = that.arguments[i] as ExprOp2
        sides |> push(takeLeft ? li.left : li.right)
    }
    // EQUAL — all lanes the same pure scalar
    if (expr_bt(sides[0]) == Type.tFloat) {
        let key0 = describe(sides[0])
        var eq = true
        for (i in range(1, d)) {
            if (describe(sides[i]) != key0) {
                eq = false
                break
            }
        }
        if (eq) {
            node = clone_expression(sides[0])
            unsafe {
                delete sides
            }
            return SIDE_EQUAL
        }
    }
    // COMPONENT — lane i reads a component of one shared base; the masks concatenate to a swizzle
    var baseKey = ""
    var base : ExpressionPtr
    var mask = ""
    var isComp = true
    for (i in range(d)) {
        var bk = ""
        var b : ExpressionPtr
        var c = ""
        if (!component_read_of(sides[i], bk, b, c) || (i > 0 && bk != baseKey)) {
            isComp = false
            break
        }
        baseKey = bk
        base = b
        mask += c // nolint:PERF001 — at most 4 single-char appends
    }
    if (isComp) {
        let dim = expr_bt(base) == Type.tFloat2 ? 2 : (expr_bt(base) == Type.tFloat3 ? 3 : 4)
        if (dim == d && mask == slice("xyzw", 0, d)) {
            node = clone_expression(base)
        } else {
            node = new ExprField(at = that.at, value = clone_expression(base), name := mask)
        }
        unsafe {
            delete sides
        }
        return SIDE_COMPONENT
    }
    // GATHER — pack the scalars into a fresh ctor (a const literal when every lane is const)
    var lanes <- [for (sd in sides); clone_expression(sd)]
    node = make_float_ctor(d, that.at, lanes)
    unsafe {
        delete lanes
        delete sides
    }
    return SIDE_GATHER
}

def private ctor_fuse_rewrite(var that : ExprCall?; no_fast_math : bool) : ExpressionPtr {
    let d = ctor_dim_by_name(that)
    if (d < 2 || !is_float_vec(expr_bt(that))) return null
    // re-vectorize same-op lanes: `floatN(v.x + c1, v.y + c2, …) → v + floatN(c1,c2,…)` (exact)
    var sameOp = ""
    if (that.arguments[0] is ExprOp2) {
        let op0 = "{(that.arguments[0] as ExprOp2).op}"
        if (op0 == "+" || op0 == "-" || op0 == "*" || op0 == "/") {
            sameOp = op0
            for (i in range(1, d)) {
                if (!(that.arguments[i] is ExprOp2) || "{(that.arguments[i] as ExprOp2).op}" != op0) {
                    sameOp = ""
                    break
                }
            }
        }
    }
    if (sameOp != "") {
        var aN : ExpressionPtr
        var bN : ExpressionPtr
        let ka = vectorize_side(that, d, true, aN)
        let kb = vectorize_side(that, d, false, bN)
        // profitability bar: a component pattern on a side (EQUAL/GATHER pairs are node-neutral)
        if (ka == SIDE_COMPONENT || kb == SIDE_COMPONENT) {
            // a lone EQUAL scalar broadcasts natively under * and /; + and - need a splat ctor
            if (ka == SIDE_EQUAL && (sameOp == "+" || sameOp == "-")) {
                aN = make_float_splat(d, that.at, aN)
            }
            if (kb == SIDE_EQUAL && (sameOp == "+" || sameOp == "-")) {
                bN = make_float_splat(d, that.at, bN)
            }
            return new ExprOp2(at = that.at, op := sameOp, left = aN, right = bN)
        }
    }
    // shared factor / divisor, keyed on lane 0's candidates (describe equality across lanes)
    if (that.arguments[0] is ExprOp2) {
        var l0 = that.arguments[0] as ExprOp2
        if (l0.op == "*" || l0.op == "/") {
            for (ci in range(l0.op == "*" ? 2 : 1)) {
                var cand = (l0.op == "/" || ci == 1) ? l0.right : l0.left
                if (expr_bt(cand) != Type.tFloat) {
                    continue
                }
                let key = describe(cand)
                var rests : array<ExpressionPtr>
                var allMatch = true
                for (i in range(d)) {
                    var ok = false
                    if (that.arguments[i] is ExprOp2) {
                        var li = that.arguments[i] as ExprOp2
                        if (li.op == l0.op) {
                            if (li.op == "*" && describe(li.left) == key && expr_bt(li.left) == Type.tFloat) {
                                rests |> push(clone_expression(li.right))
                                ok = true
                            } elif (describe(li.right) == key && expr_bt(li.right) == Type.tFloat) {
                                rests |> push(clone_expression(li.left))
                                ok = true
                            }
                        }
                    }
                    if (!ok) {
                        allMatch = false
                        break
                    }
                }
                if (allMatch) {
                    var nc = make_float_ctor(d, that.at, rests)
                    var sc = clone_expression(cand)
                    unsafe {
                        delete rests
                    }
                    return new ExprOp2(at = that.at, op := l0.op, left = nc, right = sc)
                }
                unsafe {
                    delete rests
                }
            }
        }
    }
    // majority const factor with compensation (fast-math): most frequent lane factor C ≠ 0,1 wins
    if (no_fast_math) return null
    var laneC : array<float>
    var laneV : array<float>
    var laneE : array<ExpressionPtr>
    for (i in range(d)) {
        var cv = 0.0
        if (scalar_float_const(that.arguments[i], cv)) {
            laneC |> push(0.0)
            laneV |> push(cv)
            laneE |> push(null)
            continue
        }
        var ci = 1.0
        var ei = that.arguments[i]
        if (that.arguments[i] is ExprOp2) {
            var li = that.arguments[i] as ExprOp2
            if (li.op == "*") {
                if (scalar_float_const(li.right, cv) && !(li.left |> is_expr_const())) {
                    ci = cv
                    ei = li.left
                } elif (scalar_float_const(li.left, cv) && !(li.right |> is_expr_const())) {
                    ci = cv
                    ei = li.right
                }
            }
        }
        laneC |> push(ci)
        laneV |> push(0.0)
        laneE |> push(ei)
    }
    var bestC = 0.0
    var bestN = 0
    for (i in range(d)) {
        if (laneE[i] == null || laneC[i] == 1.0 || laneC[i] == 0.0) {
            continue
        }
        var n = 0
        for (j in range(d)) {
            if (laneE[j] != null && laneC[j] == laneC[i]) {
                n ++
            }
        }
        if (n > bestN) {
            bestN = n
            bestC = laneC[i]
        }
    }
    if (bestN < 2) {
        delete laneC
        delete laneV
        unsafe {
            delete laneE
        }
        return null
    }
    var before = 0
    var after = 1
    for (i in range(d)) {
        if (laneE[i] != null && laneC[i] != 1.0) {
            before ++
        }
        if (laneE[i] != null && laneC[i] != bestC) {
            after ++
        }
    }
    if (after >= before) {
        delete laneC
        delete laneV
        unsafe {
            delete laneE
        }
        return null
    }
    var lanes : array<ExpressionPtr>
    for (i in range(d)) {
        if (laneE[i] == null) {
            lanes |> push(new ExprConstFloat(at = that.at, value = laneV[i] / bestC))
        } elif (laneC[i] == bestC) {
            lanes |> push(clone_expression(laneE[i]))
        } else {
            var comp : ExpressionPtr = new ExprConstFloat(at = that.at, value = laneC[i] / bestC)
            lanes |> push(new ExprOp2(at = that.at, op := "*", left = clone_expression(laneE[i]), right = comp))
        }
    }
    var nc = make_float_ctor(d, that.at, lanes)
    delete laneC
    delete laneV
    unsafe {
        delete lanes
        delete laneE
    }
    return new ExprOp2(at = that.at, op := "*", left = nc, right = new ExprConstFloat(at = that.at, value = bestC))
}

struct private DotGroup {
    base : ExpressionPtr        // first lane's base node, reused as the dot's vector arg
    dim : int
    weights : float4
    nterms : int
}

// One additive term as a (possibly const-weighted) lane read: `v.x`, `v.x * c`, `c * v.x`.
def private dot_term_lane(var e : ExpressionPtr; var baseKey : string&; var base : ExpressionPtr&;
                          var lane : int&; var w : float&) : bool {
    var comp = ""
    if (component_read_of(e, baseKey, base, comp)) {
        lane = lane_index(comp)
        w = 1.0
        return true
    }
    var l, r : ExpressionPtr
    if (!qmatch(e, $e(l) * $e(r)).matched) return false    // nolint:STYLE016 (qmatch macro expansion)
    var c = 0.0
    if (scalar_float_const(r, c) && component_read_of(l, baseKey, base, comp)) {
        lane = lane_index(comp)
        w = c
        return true
    }
    if (scalar_float_const(l, c) && component_read_of(r, baseKey, base, comp)) {
        lane = lane_index(comp)
        w = c
        return true
    }
    return false
}

// Bucket a chain's terms into per-base lane groups; `tKey[i]` carries term i's group key ("" for
// a non-lane term). Shared by the fuser and the residual oracle — ONE gate, so a survivor is
// always a fuser miss.
def private dot_chain_groups(var terms : array<ReTerm>; var groups : table<string; DotGroup>;
                             var tKey : array<string>) : bool {
    tKey |> resize(length(terms))
    for (i in range(length(terms))) {
        var baseKey = ""
        var base : ExpressionPtr
        var lane = 0
        var w = 0.0
        if (!dot_term_lane(terms[i].e, baseKey, base, lane, w)) {
            continue
        }
        tKey[i] = baseKey
        if (terms[i].neg) {
            w = -w
        }
        if (!key_exists(groups, baseKey)) {
            groups[baseKey] = DotGroup(base = base, dim = float_vec_dim(expr_bt(base)))
        }
        groups[baseKey].weights[lane] += w
        groups[baseKey].nterms ++
    }
    var any = false
    for (g in values(groups)) {
        if (g.nterms >= 2) {
            any = true
        }
    }
    return any
}

def private has_unfused_lane_dot(var that : ExprOp2?) : bool {
    if (that == null || !(that.op == "+" || that.op == "-") || expr_bt(that) != Type.tFloat) return false
    var terms : array<ReTerm>
    collect_add(that, false, terms)
    var groups : table<string; DotGroup>
    var tKey : array<string>
    let any = dot_chain_groups(terms, groups, tKey)
    unsafe {
        delete terms
        delete groups
    }
    delete tKey
    return any
}

// Rebuild the chain with every ≥2-lane group collapsed to `dot(base, mask)` — emitted at the
// group's first term, term order otherwise preserved; null when no group qualifies. Operand
// nodes are reused (each lands in one slot); the fused lane reads just drop.
def private dot_fold_chain(var that : ExpressionPtr; var terms : array<ReTerm>) : ExpressionPtr {
    var groups : table<string; DotGroup>
    var tKey : array<string>
    if (!dot_chain_groups(terms, groups, tKey)) {
        unsafe {
            delete groups
        }
        delete tKey
        return null
    }
    var emitted : table<string>
    var newTerms : array<ReTerm>
    for (i in range(length(terms))) {
        let k = tKey[i]
        if (k == "") {
            newTerms |> push(terms[i])
            continue
        }
        var g = groups?[k] ?? default<DotGroup>
        if (g.nterms < 2) {
            newTerms |> push(terms[i])
            continue
        }
        if (key_exists(emitted, k)) {
            continue
        }
        emitted |> insert(k)
        var lanes : array<ExpressionPtr>
        lanes |> reserve(g.dim)
        for (li in range(g.dim)) {
            lanes |> push(new ExprConstFloat(at = that.at, value = g.weights[li]))
        }
        var cll = new ExprCall(at = that.at, name := "dot")
        emplace(cll.arguments, g.base)
        var mask = make_float_ctor(g.dim, that.at, lanes)
        emplace(cll.arguments, mask)
        newTerms |> push(ReTerm(neg = false, e = cll))
        unsafe {
            delete lanes
        }
    }
    var r = build_chain(newTerms, that.at, "+", "-")
    unsafe {
        delete groups
        delete newTerms
    }
    delete tKey
    delete emitted
    return r
}

// Top-down rewrite over the closed post-flatten node set: fold each maximal `+`/`-` chain once
// (a bottom-up visitor would fold a sub-chain first and strand the outer lanes), recursing into
// chain leaves and every other child slot.
def private dot_fold_expr(var e : ExpressionPtr; var changed : bool&) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if ((o.op == "+" || o.op == "-") && expr_bt(o) == Type.tFloat) {
            var terms : array<ReTerm>
            collect_add(e, false, terms)
            for (t in terms) {
                // a chain leaf is never itself a chain, so the result is always `t.e` back
                dot_fold_expr(t.e, changed)
            }
            var r = dot_fold_chain(e, terms)
            unsafe {
                delete terms
            }
            if (r != null) {
                changed = true
                return r
            }
            return e
        }
    }
    rewrite_children(e) $(var c : ExpressionPtr) { return dot_fold_expr(c, changed) }
    return e
}

def private dot_fold_stmt(var s : ExpressionPtr; var changed : bool&) {
    if (s is ExprLet) {
        var lc = s as ExprLet
        for (lv in lc.variables) {
            if (lv.init != null) {
                lv.init = dot_fold_expr(lv.init, changed)
            }
        }
    } elif (s is ExprCopy) {
        var cp = s as ExprCopy
        cp.right = dot_fold_expr(cp.right, changed)
    } elif (s is ExprReturn) {
        var rt = s as ExprReturn
        if (rt.subexpr != null) {
            rt.subexpr = dot_fold_expr(rt.subexpr, changed)
        }
    }
}

class private CtorFuseVisitor : AstVisitor {
    changed : bool
    noFastMath : bool
    def override visitExprCall(var that : ExprCall?) : ExpressionPtr {
        var r = ctor_fuse_rewrite(that, noFastMath)
        if (r != null) {
            changed = true
            return r
        }
        return that
    }
}

class private MadFuseVisitor : AstVisitor {
    changed : bool
    def override visitExprOp2(var that : ExprOp2?) : ExpressionPtr {
        var ma : ExpressionPtr
        var mb : ExpressionPtr
        var mc : ExpressionPtr
        if (match_mad(that, ma, mb, mc)) {
            changed = true
            var cll = new ExprCall(at = that.at, name := "mad")
            emplace(cll.arguments, ma)
            emplace(cll.arguments, mb)
            emplace(cll.arguments, mc)
            return cll
        }
        var pos : ExpressionPtr
        var neg : ExpressionPtr
        if (match_neg_add(that, pos, neg)) {
            changed = true
            return new ExprOp2(at = that.at, op := "-", left = pos, right = neg)
        }
        return that
    }
}

class private LerpFuseVisitor : AstVisitor {
    changed : bool
    def override visitExprCall(var that : ExprCall?) : ExpressionPtr {
        var la : ExpressionPtr
        var lb : ExpressionPtr
        var lt : ExpressionPtr
        if (match_lerp(that, la, lb, lt)) {
            changed = true
            var cll = new ExprCall(at = that.at, name := "lerp")
            emplace(cll.arguments, la)
            emplace(cll.arguments, lb)
            emplace(cll.arguments, lt)
            return cll
        }
        return that
    }
}

// A horizontal-reduce lane that no better fuser already claims: a plain scalar `s` (weight 1) OR a const-weighted scalar `s*c` / `c*s` (weight c). `s` must not be const, a runtime*runtime product (left for mad), or a fuse-pass output call (`mad`/`dot`/`lerp`/`hadd` — already optimally fused, and re-claiming one across re-infer rounds would make the pass non-idempotent). A group of these packs into `hadd(floatN(s))` when all weights are 1, else `dot(floatN(s), floatN(weights))` — the weighted reduction beats the mad-chain at width>=3 (combine + dot = 2 nodes vs the N-node mad-chain). The fuser packs groups of width >=3 by default (width-2 is +1 node on a node-graph backend — pure combine overhead — so a bare pair stays scalar; `options _flatten_hadd_pack_pairs = true` lowers the floor to 2 for a SIMD CPU backend). `stripped` = the term carried a `*const`, so its scalar must be cloned (the `*` wrapper drops). The residual oracle counts the same predicate, so it mirrors the fuser exactly.
def private reduce_lane(var t : ReTerm; var s : ExpressionPtr&; var w : float&; var stripped : bool&) : bool {   // nolint:LINT014 — `var t` is needed for NON-CONST field access (t.e feeds is_all_const / scalar_float_const), not to write t
    if (t.neg) return false
    var e = t.e
    w = 1.0
    stripped = false
    if (e is ExprOp2 && (e as ExprOp2).op == "*") {
        var o = e as ExprOp2
        var c = 0.0
        if (scalar_float_const(o.right, c) && !is_all_const(o.left)) {
            e = o.left
            w = c
            stripped = true
        } elif (scalar_float_const(o.left, c) && !is_all_const(o.right)) {
            e = o.right
            w = c
            stripped = true
        } else {
            return false   // runtime*runtime product -> leave for mad
        }
    }
    if (is_all_const(e) || expr_bt(e) != Type.tFloat) return false
    if (e is ExprCall) {
        let nm = simple_name("{(e as ExprCall).name}")
        if (nm == "hadd" || nm == "mad" || nm == "dot" || nm == "lerp") return false
    }
    s = e
    return true
}

// ===== producer-vectorization for hadd groups =====

// Componentwise unary float->float builtins (their floatN overload is per-lane). NOT length/normalize/
// dot/cross (vector->scalar or cross-lane) — vectorizing those would change the value.
let private ELEMENTWISE_UNARY : table<string> <- {
    "sin", "cos", "sqrt", "exp", "log", "abs", "frac", "floor", "ceil",
    "sign", "saturate", "radians", "degrees", "one_minus"
}

// If every lane is `g(scalarFloat)` for the SAME whitelisted elementwise-unary g, return g; else "".
def private shared_elementwise_unary(lanes : array<ExpressionPtr>) : string {
    var g = ""
    for (e in lanes) {
        if (!(e is ExprCall)) return ""
        let c = e as ExprCall
        if (length(c.arguments) != 1 || expr_bt(c.arguments[0]) != Type.tFloat) return ""
        let fullname = "{c.name}"
        if (find(fullname, 58, 0) >= 0) return ""   // 58 = ':' — a module qualifier (math::sin); see above
        let nm = simple_name(fullname)
        if (!key_exists(ELEMENTWISE_UNARY, nm)) return ""
        if (g == "") {
            g = nm
        } elif (g != nm) {
            return ""
        }
    }
    return g
}

def private mod_has_unary_vec(mod : Module?; fname : string; want : Type) : bool {
    var found = false
    for_each_function(mod, fname) $(fn) {
        if (length(fn.arguments) == 1 && fn.arguments[0]._type != null && fn.arguments[0]._type.baseType == want) {
            found = true
        }
    }
    return found
}

// True if `fname` has a single-arg overload taking exactly `want` (a floatN) in `mod` or its deps —
// the gate that keeps producer-vectorization from emitting an unresolvable `g(floatN)` on a backend
// whose `g` is scalar-only (it falls back to leaf-packing rather than breaking a shader that compiled).
def private has_unary_vec_overload(mod : Module?; fname : string; want : Type) : bool {
    if (mod == null) return false
    if (mod_has_unary_vec(mod, fname, want)) return true
    var found = false
    module_for_each_dependency(mod) $(dep, _isPub) {
        if (!found && mod_has_unary_vec(dep, fname, want)) {
            found = true
        }
    }
    return found
}

def private float_vec_type(dim : int) : Type {
    return dim == 2 ? Type.tFloat2 : (dim == 3 ? Type.tFloat3 : Type.tFloat4)
}

// Build a hadd group's argument: the producer-vectorized `g(floatN(args))` when all `lanes` share an
// elementwise-unary `g` with a floatN overload, else the plain `floatN(lanes)` leaf pack.
def private make_hadd_arg(var lanes : array<ExpressionPtr>; w : int; at : LineInfo; mod : Module?) : ExpressionPtr {
    let g = shared_elementwise_unary(lanes)
    if (g == "" || !has_unary_vec_overload(mod, g, float_vec_type(w))) {
        return make_float_ctor(w, at, lanes)
    }
    var args : array<ExpressionPtr>
    args |> reserve(w)
    for (ln in lanes) {
        args |> push(clone_expression((ln as ExprCall).arguments[0]))   // clone: the g-call wrapper drops
    }
    var argCtor = make_float_ctor(w, at, args)
    var gcall = new ExprCall(at = at, name := g)
    emplace(gcall.arguments, argCtor)
    unsafe {
        delete args
    }
    return gcall
}

// Pack the independent-scalar reduce lanes of a maximal `+`/`-` chain (see reduce_lane) into width [minWidth..4] groups via plan_groups; null when no group reaches minWidth. The dot fold above only reaches lanes of ONE vector (`v.x + v.y + …`); unrelated scalars (plasma's `v1 + v2 + v3 + v4`) have no shared base, so a combine + reduce is the collapse. Each group emits `hadd(floatN(s))` when all its weights are 1, else `dot(floatN(s), floatN(weights))` (a const-weighted sum like an fBm octave chain).
def private hadd_fold_chain(var that : ExpressionPtr; var terms : array<ReTerm>; minWidth : int; mod : Module?) : ExpressionPtr {
    // collect reduce lanes (scalar + const weight) in source order; a weighted lane emits `dot`, so it is only a candidate when dot is callable — else it stays for the mad walk (never break a shader)
    let dotCallable = fuse_callable(mod, "dot", 2)
    var cand : array<int>
    var laneScalar : array<ExpressionPtr>
    var laneWeight : array<float>
    var laneStripped : array<bool>
    for (i in range(length(terms))) {
        var s : ExpressionPtr
        var w = 1.0
        var stripped = false
        if (reduce_lane(terms[i], s, w, stripped)) {
            if (stripped && !dotCallable) {
                continue
            }
            cand |> push(i)
            laneScalar |> push(s)
            laneWeight |> push(w)
            laneStripped |> push(stripped)
        }
    }
    var groups <- plan_groups(length(cand), minWidth)
    var covered = 0
    for (w in groups) {
        covered += w
    }
    if (covered == 0) {     // no group reached minWidth — the lanes (if any) stay scalar
        delete cand
        delete laneWeight
        delete laneStripped
        delete groups
        unsafe {
            delete laneScalar
        }
        return null
    }
    // head term idx -> its group index in `groups`; groupOff[gi] = that group's first candidate slot
    var headGroup : table<int; int>
    var groupOff : array<int>
    groupOff |> reserve(length(groups))
    var dropped : table<int>
    var off = 0
    for (gi in range(length(groups))) {
        let w = groups[gi]
        headGroup |> insert(cand[off], gi)
        groupOff |> push(off)
        for (li in range(1, w)) {
            dropped |> insert(cand[off + li])
        }
        off += w
    }
    var newTerms : array<ReTerm>
    for (i in range(length(terms))) {
        if (key_exists(dropped, i)) continue
        if (key_exists(headGroup, i)) {
            let gi = headGroup[i]
            let w = groups[gi]
            let o = groupOff[gi]
            var weighted = false
            for (li in range(w)) {
                if (laneStripped[o + li] || laneWeight[o + li] != 1.0) {
                    weighted = true
                }
            }
            var cll : ExpressionPtr
            if (weighted) {
                // dot(floatN(scalars), const floatN(weights)) — clone scalars (the `*const` wrappers drop)
                var scalars : array<ExpressionPtr>
                var weights : array<ExpressionPtr>
                scalars |> reserve(w)
                weights |> reserve(w)
                for (li in range(w)) {
                    scalars |> push(clone_expression(laneScalar[o + li]))
                    weights |> push(new ExprConstFloat(at = that.at, value = laneWeight[o + li]))
                }
                var vecArg = make_float_ctor(w, that.at, scalars)
                var mask = make_float_ctor(w, that.at, weights)
                var dcll = new ExprCall(at = that.at, name := "dot")
                emplace(dcll.arguments, vecArg)
                emplace(dcll.arguments, mask)
                cll = dcll
                unsafe {
                    delete scalars
                    delete weights
                }
            } else {
                var lanes : array<ExpressionPtr>
                lanes |> reserve(w)
                for (li in range(w)) {
                    lanes |> push(terms[cand[o + li]].e)   // reuse the leaf node (each used once)
                }
                var ctor = make_hadd_arg(lanes, w, that.at, mod)   // producer-vectorizes a shared unary
                var hcll = new ExprCall(at = that.at, name := "hadd")
                emplace(hcll.arguments, ctor)
                cll = hcll
                unsafe {
                    delete lanes
                }
            }
            newTerms |> push(ReTerm(neg = false, e = cll))
        } else {
            newTerms |> push(terms[i])
        }
    }
    var r = build_chain(newTerms, that.at, "+", "-")
    delete cand
    delete laneWeight
    delete laneStripped
    delete groups
    delete headGroup
    delete groupOff
    delete dropped
    unsafe {
        delete laneScalar
        delete newTerms
    }
    return r
}

// Top-down rewrite mirroring dot_fold_expr: pack each maximal `+`/`-` chain once. A bottom-up visitor would pack an inner sub-chain into a narrow hadd and strand the outer lanes into a second one (e.g.
def private hadd_fold_expr(var e : ExpressionPtr; var changed : bool&; minWidth : int; mod : Module?) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if ((o.op == "+" || o.op == "-") && expr_bt(o) == Type.tFloat) {
            var terms : array<ReTerm>
            collect_add(e, false, terms)
            for (t in terms) {
                // a chain leaf is never itself a chain, so it mutates in place and returns `t.e`
                hadd_fold_expr(t.e, changed, minWidth, mod)
            }
            var r = hadd_fold_chain(e, terms, minWidth, mod)
            unsafe {
                delete terms
            }
            if (r != null) {
                changed = true
                return r
            }
            return e
        }
    }
    rewrite_children(e) $(var c : ExpressionPtr) { return hadd_fold_expr(c, changed, minWidth, mod) }
    return e
}

def private hadd_fold_stmt(var s : ExpressionPtr; var changed : bool&; minWidth : int; mod : Module?) {
    if (s is ExprLet) {
        var lc = s as ExprLet
        for (lv in lc.variables) {
            if (lv.init != null) {
                lv.init = hadd_fold_expr(lv.init, changed, minWidth, mod)
            }
        }
    } elif (s is ExprCopy) {
        var cp = s as ExprCopy
        cp.right = hadd_fold_expr(cp.right, changed, minWidth, mod)
    } elif (s is ExprReturn) {
        var rt = s as ExprReturn
        if (rt.subexpr != null) {
            rt.subexpr = hadd_fold_expr(rt.subexpr, changed, minWidth, mod)
        }
    }
}

// A maximal `+`/`-` chain still carrying ≥3 reduce lanes the fuser should have packed (into hadd or a weighted dot) — the residual twin of the reduce fold, sharing `reduce_lane`. The >=3 gate is mode-agnostic on purpose: the fuser packs width>=3 in BOTH modes (the pack-pairs option only LOWERS the floor to 2), so a >=3-lane survivor is a real miss under either, while a 2-lane chain left scalar by the default is not flagged.
def private has_unfused_hadd(var that : ExprOp2?; dotCallable : bool) : bool {
    if (that == null || !(that.op == "+" || that.op == "-") || expr_bt(that) != Type.tFloat) return false
    var terms : array<ReTerm>
    collect_add(that, false, terms)
    var n = 0
    for (t in terms) {
        var s : ExpressionPtr
        var w = 1.0
        var stripped = false
        if (reduce_lane(t, s, w, stripped)) {
            if (stripped && !dotCallable) {
                continue   // a weighted lane only packs (into dot) when dot is callable — mirror the fuser gate
            }
            n ++
        }
    }
    unsafe {
        delete terms
    }
    return n >= 3
}

// Fusion residuals — mirror the fuser's matchers (same gates, same shapes), so a survivor means the
// PHASE_FUSED pass missed, never that the gate was narrower here. (The hadd/reduce oracle uses a fixed
// width-3 floor, mode-agnostic — see has_unfused_hadd — since the fuser packs width>=3 in both modes.)
class public FuseResidualVisitor : AstVisitor {
    checkMad : bool
    checkLerp : bool
    checkDot : bool
    checkHadd : bool
    noFastMath : bool
    foundMad : bool
    foundLerp : bool
    foundNegAdd : bool
    foundCtor : bool
    foundDot : bool
    foundHadd : bool
    def override preVisitExprOp2(var expr : ExprOp2?) : void {
        if (checkDot && has_unfused_lane_dot(expr)) {
            foundDot = true
        }
        if (checkHadd && has_unfused_hadd(expr, checkDot)) {   // checkDot == dot-callable here (checkHadd ⟹ !no_fast_math)
            foundHadd = true
        }
        if (!checkMad) {
            return
        }
        var ma : ExpressionPtr
        var mb : ExpressionPtr
        var mc : ExpressionPtr
        if (match_mad(expr, ma, mb, mc)) {
            foundMad = true
        }
        var pos : ExpressionPtr
        var neg : ExpressionPtr
        if (match_neg_add(expr, pos, neg)) {
            foundNegAdd = true
        }
    }
    def override preVisitExprCall(var expr : ExprCall?) : void {
        if (checkLerp) {
            var la : ExpressionPtr
            var lb : ExpressionPtr
            var lt : ExpressionPtr
            if (match_lerp(expr, la, lb, lt)) {
                foundLerp = true
            }
        }
        if (ctor_fuse_rewrite(expr, noFastMath) != null) {
            foundCtor = true
        }
    }
}

def public fuse_body(var func : FunctionPtr; no_fast_math : bool = false; hadd_min_width : int = 3) : bool {
    //! PHASE_FUSED finishing pass: collapse same-vector lane sums into `dot(v, mask)` and independent scalar sums into `hadd(floatN(...))` — or `dot(floatN(s), floatN(weights))` when the lanes carry const weights (groups of width [hadd_min_width..4] via plan_groups — the default 3 keeps a bare pair scalar, 2 packs pairs too), re-pack ctor lane patterns (re-vectorize / shared factor), `a*b ± c` into `mad(a,b,c)`, and the expanded lerp shape back into `lerp(a, b, t)`. Caller re-infers after each round.
    if (!(func.body is ExprBlock)) return false
    var changed = false
    // lane-dot fold first — the mad walk would otherwise steal a weighted lane (`v.x * w + rest`)
    if (!no_fast_math && fuse_callable(func._module, "dot", 2)) {
        var blk = func.body as ExprBlock
        var dchanged = false
        for (s in blk.list) {
            dot_fold_stmt(s, dchanged)
        }
        if (dchanged) {
            changed = true
        }
    }
    // ctor re-pack next — it claims same-op ctor lanes (`floatN(x+a, x+b, …)` → `splat(x) + floatN(a,b,…)`)
    // BEFORE the hadd fold, so a hadd at width 2 can't pre-empt that vectorization by turning a lane into
    // a `hadd(...)` call; its `ctor * s` / vector-op output also feeds the mad walk this same round.
    var cv = new CtorFuseVisitor()
    cv.noFastMath = no_fast_math
    make_visitor(*cv) $(adapter) {
        visit(func.body, adapter)
    }
    if (cv.changed) {
        changed = true
    }
    unsafe {
        delete cv
    }
    // then horizontal reduce: pack unrelated scalar lanes into hadd(floatN(...)) — or dot(floatN(s), floatN(weights)) when they carry const weights — in width-[hadd_min_width..4] groups via plan_groups.
    if (!no_fast_math && fuse_callable(func._module, "hadd", 1)) {
        var blk = func.body as ExprBlock
        var hchanged = false
        for (s in blk.list) {
            hadd_fold_stmt(s, hchanged, hadd_min_width, func._module)
        }
        if (hchanged) {
            changed = true
        }
    }
    if (!fuse_callable(func._module, "mad")) return changed
    var v = new MadFuseVisitor()
    make_visitor(*v) $(adapter) {
        visit(func.body, adapter)
    }
    if (v.changed) {
        changed = true
    }
    unsafe {
        delete v
    }
    // lerp re-fusion is a separate walk over the fresh mads — a visitor never descends into
    // the node it just returned, so the mad → lerp chain can't happen in one pass.
    if (fuse_callable(func._module, "lerp")) {
        var lv = new LerpFuseVisitor()
        make_visitor(*lv) $(adapter) {
            visit(func.body, adapter)
        }
        if (lv.changed) {
            changed = true
        }
        unsafe {
            delete lv
        }
    }
    return changed
}
