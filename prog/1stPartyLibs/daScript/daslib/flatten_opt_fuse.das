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
    if (e is ExprConstDouble) return new ExprConstDouble(at = e.at, value = -(e as ExprConstDouble).value)
    var L = const_lanes(e)
    if (L.kind == CK_FLOAT) { L.f = -L.f; return make_const_lanes(L, e.at) }
    if (L.kind == CK_INT) { L.i = -L.i; return make_const_lanes(L, e.at) }
    return null
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

def private match_mad(var that : ExprOp2?; var ma, mb, mc : ExpressionPtr&) : bool {
    if (that == null) return false
    var fa, fb, other : ExpressionPtr
    var negOther = false
    var negFactor = false
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

def private match_neg_add(var that : ExprOp2?; var pos, neg : ExpressionPtr&) : bool {
    if (that == null || !is_mad_elem(expr_bt(that))) return false
    return (qmatch(that, -$e(neg) + $e(pos)).matched ||
        qmatch(that, $e(pos) + -$e(neg)).matched)
}

def private match_lerp(var cll : ExprCall?; var la, lb, lt : ExpressionPtr&) : bool {
    if (cll == null) return false
    var da, db, aa, tt : ExpressionPtr
    if (!qmatch(cll, mad($e(db) - $e(da), $e(tt), $e(aa))).matched &&     // nolint:STYLE016 (qmatch macro expansion)
        !qmatch(cll, mad(-$e(da) + $e(db), $e(tt), $e(aa))).matched &&
        !qmatch(cll, mad($e(db) + -$e(da), $e(tt), $e(aa))).matched) return false
    if (describe(da) != describe(aa) ||
        !lerp_shape(expr_bt(da), expr_bt(db), expr_bt(tt))) return false
    la = aa
    lb = db
    lt = tt
    return true
}

let private SIDE_NONE = 0
let private SIDE_EQUAL = 1
let private SIDE_COMPONENT = 2
let private SIDE_GATHER = 3

def private vectorize_side(var that : ExprCall?; d : int; takeLeft : bool; var node : ExpressionPtr&) : int {
    var sides : array<ExpressionPtr>
    sides |> reserve(d)
    for (i in range(d)) {
        var li = that.arguments[i] as ExprOp2
        sides |> push(takeLeft ? li.left : li.right)
    }
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
    var lanes <- [for (sd in sides); clone_expression(sd)]
    node = make_float_ctor(d, that.at, lanes)
    unsafe {
        delete lanes
        delete sides
    }
    return SIDE_GATHER
}

def private ctor_fuse_same_op(var that : ExprCall?; d : int) : ExpressionPtr {
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
    if (sameOp == "") return null
    var aN : ExpressionPtr
    var bN : ExpressionPtr
    let ka = vectorize_side(that, d, true, aN)
    let kb = vectorize_side(that, d, false, bN)
    if (ka != SIDE_COMPONENT && kb != SIDE_COMPONENT) return null
    if (ka == SIDE_EQUAL && (sameOp == "+" || sameOp == "-")) {
        aN = make_float_splat(d, that.at, aN)
    }
    if (kb == SIDE_EQUAL && (sameOp == "+" || sameOp == "-")) {
        bN = make_float_splat(d, that.at, bN)
    }
    return new ExprOp2(at = that.at, op := sameOp, left = aN, right = bN)
}

def private ctor_fuse_shared_factor(var that : ExprCall?; d : int) : ExpressionPtr {
    if (!(that.arguments[0] is ExprOp2)) return null
    var l0 = that.arguments[0] as ExprOp2
    if (l0.op != "*" && l0.op != "/") return null
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
    return null
}

def private lane_const_factors(var that : ExprCall?; d : int; var laneC, laneV : array<float>;
                               var laneE : array<ExpressionPtr>) {
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
}

def private majority_factor(laneC : array<float>; laneE : array<ExpressionPtr>; d : int; var bestC : float&) : int {
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
    return bestN
}

def private ctor_fuse_majority_const(var that : ExprCall?; d : int) : ExpressionPtr {
    var laneC : array<float>
    var laneV : array<float>
    var laneE : array<ExpressionPtr>
    lane_const_factors(that, d, laneC, laneV, laneE)
    var bestC = 0.0
    let bestN = majority_factor(laneC, laneE, d, bestC)
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
    var res : ExpressionPtr = null
    if (bestN >= 2 && after < before) {
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
        res = new ExprOp2(at = that.at, op := "*", left = nc, right = new ExprConstFloat(at = that.at, value = bestC))
        unsafe {
            delete lanes
        }
    }
    delete laneC
    delete laneV
    unsafe {
        delete laneE
    }
    return res
}

def private ctor_fuse_rewrite(var that : ExprCall?; no_fast_math : bool) : ExpressionPtr {
    let d = ctor_dim_by_name(that)
    if (d < 2 || !is_float_vec(expr_bt(that))) return null
    var r = ctor_fuse_same_op(that, d)
    if (r != null) return r
    r = ctor_fuse_shared_factor(that, d)
    if (r != null) return r
    if (no_fast_math) return null
    return ctor_fuse_majority_const(that, d)
}

struct private DotGroup {
    base : ExpressionPtr
    dim : int
    weights : float4
    nterms : int
}

def private dot_term_lane(var e : ExpressionPtr; var baseKey : string&; var base : ExpressionPtr&;
                          var lane : int&; var w : float&) : bool {
    var comp = ""
    if (component_read_of(e, baseKey, base, comp)) {
        lane = lane_index(comp)
        w = 1.0
        return true
    }
    var l, r : ExpressionPtr
    if (!qmatch(e, $e(l) * $e(r)).matched) return false
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

def private dot_chain_groups(var terms : array<ReTerm>; var groups : table<string; DotGroup>;
                             var tKey : array<string>) : bool {
    tKey |> resize(length(terms))
    for (t, key in terms, tKey) {
        var baseKey = ""
        var base : ExpressionPtr
        var lane = 0
        var w = 0.0
        if (!dot_term_lane(t.e, baseKey, base, lane, w)) {
            continue
        }
        key = baseKey
        if (t.neg) {
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
    for (t, k in terms, tKey) {
        if (k == "") {
            newTerms |> push(t)
            continue
        }
        var g = groups?[k] ?? default<DotGroup>
        if (g.nterms < 2) {
            newTerms |> push(t)
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

def private dot_fold_expr(var e : ExpressionPtr; var changed : bool&) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if ((o.op == "+" || o.op == "-") && expr_bt(o) == Type.tFloat) {
            var terms : array<ReTerm>
            collect_add(e, false, terms)
            for (t in terms) {
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

def private reduce_lane(var t : ReTerm; var s : ExpressionPtr&; var w : float&; var stripped : bool&) : bool {
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
            return false
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

let private ELEMENTWISE_UNARY : table<string> <- {
    "sin", "cos", "sqrt", "exp", "log", "abs", "frac", "floor", "ceil",
    "sign", "saturate", "radians", "degrees", "one_minus"
}

def private shared_elementwise_unary(lanes : array<ExpressionPtr>) : string {
    var g = ""
    for (e in lanes) {
        if (!(e is ExprCall)) return ""
        let c = e as ExprCall
        if (length(c.arguments) != 1 || expr_bt(c.arguments[0]) != Type.tFloat) return ""
        let fullname = "{c.name}"
        if (find(fullname, ':', 0) >= 0) return ""
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

def private make_hadd_arg(var lanes : array<ExpressionPtr>; w : int; at : LineInfo; mod : Module?) : ExpressionPtr {
    let g = shared_elementwise_unary(lanes)
    if (g == "" || !has_unary_vec_overload(mod, g, float_vec_type(w))) {
        return make_float_ctor(w, at, lanes)
    }
    var args : array<ExpressionPtr>
    args |> reserve(w)
    for (ln in lanes) {
        args |> push(clone_expression((ln as ExprCall).arguments[0]))
    }
    var argCtor = make_float_ctor(w, at, args)
    var gcall = new ExprCall(at = at, name := g)
    emplace(gcall.arguments, argCtor)
    unsafe {
        delete args
    }
    return gcall
}

def private collect_reduce_lanes(var terms : array<ReTerm>; dotCallable : bool; var cand : array<int>;
                                 var laneScalar : array<ExpressionPtr>; var laneWeight : array<float>;
                                 var laneStripped : array<bool>) {
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
}

def private make_reduce_group_call(var terms : array<ReTerm>; cand : array<int>;
                                   laneScalar : array<ExpressionPtr>; laneWeight : array<float>;
                                   laneStripped : array<bool>; o, w : int; at : LineInfo;
                                   mod : Module?) : ExpressionPtr {
    var weighted = false
    for (li in range(w)) {
        if (laneStripped[o + li] || laneWeight[o + li] != 1.0) {
            weighted = true
        }
    }
    if (weighted) {
        var scalars : array<ExpressionPtr>
        var weights : array<ExpressionPtr>
        scalars |> reserve(w)
        weights |> reserve(w)
        for (li in range(w)) {
            scalars |> push(clone_expression(laneScalar[o + li]))
            weights |> push(new ExprConstFloat(at = at, value = laneWeight[o + li]))
        }
        var vecArg = make_float_ctor(w, at, scalars)
        var mask = make_float_ctor(w, at, weights)
        var dcll = new ExprCall(at = at, name := "dot")
        emplace(dcll.arguments, vecArg)
        emplace(dcll.arguments, mask)
        unsafe {
            delete scalars
            delete weights
        }
        return dcll
    }
    var lanes : array<ExpressionPtr>
    lanes |> reserve(w)
    for (li in range(w)) {
        lanes |> push(terms[cand[o + li]].e)
    }
    var ctor = make_hadd_arg(lanes, w, at, mod)
    var hcll = new ExprCall(at = at, name := "hadd")
    emplace(hcll.arguments, ctor)
    unsafe {
        delete lanes
    }
    return hcll
}

def private hadd_fold_chain(var that : ExpressionPtr; var terms : array<ReTerm>; minWidth : int; mod : Module?) : ExpressionPtr {
    var cand : array<int>
    var laneScalar : array<ExpressionPtr>
    var laneWeight : array<float>
    var laneStripped : array<bool>
    collect_reduce_lanes(terms, fuse_callable(mod, "dot", 2), cand, laneScalar, laneWeight, laneStripped)
    var groups <- plan_groups(length(cand), minWidth)
    var covered = 0
    for (w in groups) {
        covered += w
    }
    if (covered == 0) {
        delete cand
        delete laneWeight
        delete laneStripped
        delete groups
        unsafe {
            delete laneScalar
        }
        return null
    }
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
            var cll = make_reduce_group_call(terms, cand, laneScalar, laneWeight, laneStripped,
                groupOff[gi], groups[gi], that.at, mod)
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

def private hadd_fold_expr(var e : ExpressionPtr; var changed : bool&; minWidth : int; mod : Module?) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if ((o.op == "+" || o.op == "-") && expr_bt(o) == Type.tFloat) {
            var terms : array<ReTerm>
            collect_add(e, false, terms)
            for (t in terms) {
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

def private has_unfused_hadd(var that : ExprOp2?; dotCallable : bool; minWidth : int) : bool {
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
                continue
            }
            n ++
        }
    }
    unsafe {
        delete terms
    }
    return n >= minWidth
}

class public FuseResidualVisitor : AstVisitor {
    checkMad : bool
    checkLerp : bool
    checkDot : bool
    checkHadd : bool
    haddMinWidth : int
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
        if (checkHadd && has_unfused_hadd(expr, checkDot, haddMinWidth)) {
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
