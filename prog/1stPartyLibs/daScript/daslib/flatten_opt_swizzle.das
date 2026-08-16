options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_swizzle shared private

//! Swizzle lane fold: resolve each output lane of a ctor/const/swizzle and re-emit the cheapest form (base / swizzle / scalar / splat / narrower ctor).

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require daslib/flatten_opt_common


// ===== Swizzle lane fold =====
// `ctor(args…).mask` pays a combine + an extract for separable lanes: resolve each output lane's
// provenance, re-emit the cheapest form (base / swizzle / scalar / splat / narrower ctor).

struct private LaneSrc {
    lane : int          // -1: e itself is the lane; >= 0: lane of e
    e : ExpressionPtr   // scalar expr when lane < 0, else the (opaque) float-vector base
}

def private lane_letter(i : int) : string {
    if (i == 0) return "x"
    if (i == 1) return "y"
    if (i == 2) return "z"
    return "w"
}

// float2/3/4 ctor call of ANY arity (full per-lane, mixed vector+scalar, splat); null otherwise.
def private float_ctor_of(e : ExpressionPtr) : ExprCall? {
    if (e == null || !(e is ExprCall)) return null
    var c = e as ExprCall
    let nm = simple_name("{c.name}")
    if (nm != "float2" && nm != "float3" && nm != "float4") return null
    return c
}

def private r2v_unwrap(var e : ExpressionPtr) : ExpressionPtr {
    return e is ExprRef2Value ? (e as ExprRef2Value).subexpr : e
}

// The fold arm's firing gate (and the residual oracle's): a value resolve_lanes can see through.
def private swizzle_decomposable(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return swizzle_decomposable((e as ExprRef2Value).subexpr)
    if (e is ExprConstFloat2 || e is ExprConstFloat3 || e is ExprConstFloat4) return true
    if (e is ExprSwizzle) return float_vec_dim(expr_bt((e as ExprSwizzle).value)) > 0
    return float_ctor_of(e) != null
}

// Per-output-lane provenance of a float-vector expr. Sees through float ctors of any arity,
// vector const literals, nested swizzles, component-read scalar args and Ref2Value wraps; an
// opaque vector contributes (e, 0..d-1). False only when something isn't a typed float vector.
def private resolve_lanes(var e : ExpressionPtr; var out : array<LaneSrc>) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return resolve_lanes((e as ExprRef2Value).subexpr, out)
    let d = float_vec_dim(expr_bt(e))
    if (d == 0) return false
    if (e is ExprConstFloat2) {
        let v = (e as ExprConstFloat2).value
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.x)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.y)))
        return true
    }
    if (e is ExprConstFloat3) {
        let v = (e as ExprConstFloat3).value
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.x)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.y)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.z)))
        return true
    }
    if (e is ExprConstFloat4) {
        let v = (e as ExprConstFloat4).value
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.x)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.y)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.z)))
        out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = v.w)))
        return true
    }
    if (e is ExprSwizzle) {
        var sw = e as ExprSwizzle
        var inner : array<LaneSrc>
        var ok = resolve_lanes(sw.value, inner)
        if (ok) {
            for (f in sw.fields) {
                if (int(f) >= length(inner)) {
                    ok = false
                } else {
                    out |> push(inner[int(f)])
                }
            }
        }
        unsafe {
            delete inner
        }
        return ok && length(sw.fields) == d
    }
    var c = float_ctor_of(e)
    if (c != null) {
        // splat: the one scalar arg fills every lane (emitters clone the duplicate references)
        if (length(c.arguments) == 1 && expr_bt(c.arguments[0]) == Type.tFloat) {
            for (_i in range(d)) {
                out |> push(LaneSrc(lane = -1, e = c.arguments[0]))   // nolint:PERF006 -- ≤4 lanes
            }
            return true
        }
        var total = 0
        for (i in range(length(c.arguments))) {
            var a = c.arguments[i]
            let abt = expr_bt(a)
            if (abt == Type.tFloat) {
                var bk = ""
                var base : ExpressionPtr
                var comp = ""
                if (component_read_of(a, bk, base, comp)) {
                    out |> push(LaneSrc(lane = lane_index(comp), e = base))
                } else {
                    out |> push(LaneSrc(lane = -1, e = a))
                }
                total ++
            } elif (float_vec_dim(abt) > 0) {
                if (!resolve_lanes(a, out)) return false
                total += float_vec_dim(abt)
            } else {
                return false
            }
        }
        return total == d
    }
    // opaque float vector — contributes its own lanes
    for (i in range(d)) {
        out |> push(LaneSrc(lane = i, e = e))   // nolint:PERF006 -- ≤4 lanes
    }
    return true
}

let private SWF_NONE = 0
let private SWF_VALUE = 1      // identity selection — drop the swizzle, keep the value
let private SWF_BASE = 2       // single base, full in-order width — the base itself
let private SWF_SWIZZLE = 3    // single base — one re-masked swizzle
let private SWF_SCALAR = 4     // one scalar lane — the scalar expr
let private SWF_SPLAT = 5      // every lane the same non-const scalar — splat ctor
let private SWF_CTOR = 6       // re-pack whole bases + scalars into a narrower/permuted ctor

// Maximal consecutive same-base groups of a selection ((start, len) each); scalar lanes are
// singleton runs. Shared by classify (validity) and the rewrite (emission) so they cannot drift.
def private swizzle_sel_runs(sel : array<LaneSrc>; var runs : array<int2>) {
    var i = 0
    while (i < length(sel)) {
        if (sel[i].lane < 0) {
            runs |> push(int2(i, 1))
            i ++
        } else {
            let bk = describe(sel[i].e)
            var j = i + 1
            while (j < length(sel) && sel[j].lane >= 0 && describe(sel[j].e) == bk) {
                j ++
            }
            runs |> push(int2(i, j - i))
            i = j
        }
    }
}

// Decide which rewrite a swizzle's resolved selection admits and fill `sel` for the builder.
// Shared by the fold arm and the residual oracle — ONE gate, so a survivor is always a fold miss.
def private swizzle_fold_classify(var that : ExprSwizzle?; var sel : array<LaneSrc>) : int {
    if (that == null || that.fieldFlags.write) return SWF_NONE
    let nOut = length(that.fields)
    if (nOut < 1 || nOut > 4 || !swizzle_decomposable(that.value)) return SWF_NONE
    var lanes : array<LaneSrc>
    var ok = resolve_lanes(that.value, lanes)
    let nLanes = length(lanes)
    if (ok) {
        for (f in that.fields) {
            if (int(f) >= nLanes) {
                ok = false
            }
        }
    }
    if (ok) {
        for (f in that.fields) {
            sel |> push(lanes[int(f)])   // nolint:PERF006 -- ≤4 lanes
        }
    }
    unsafe {
        delete lanes
    }
    if (!ok) return SWF_NONE
    var nScalar = 0
    for (s in sel) {
        if (s.lane < 0) {
            nScalar ++
        }
    }
    // single-base arms: every lane a component of one (describe-equal) base
    if (nScalar == 0) {
        let k0 = describe(sel[0].e)
        var same = true
        for (s in sel) {
            if (describe(s.e) != k0) {
                same = false
            }
        }
        if (same) {
            var ident = nOut == float_vec_dim(expr_bt(sel[0].e))
            for (i in range(nOut)) {
                if (sel[i].lane != i) {
                    ident = false
                }
            }
            return ident ? SWF_BASE : SWF_SWIZZLE
        }
    }
    if (nOut == 1) return SWF_SCALAR    // the single component-lane case returned above
    // identity selection over the whole value — just drop the swizzle node
    var ident = nOut == nLanes
    if (ident) {
        for (i in range(nOut)) {
            if (int(that.fields[i]) != i) {
                ident = false
            }
        }
    }
    if (ident) return SWF_VALUE
    // every lane the same non-const scalar — splat (all-const packs to a literal via SWF_CTOR)
    if (nScalar == nOut) {
        let k0 = describe(sel[0].e)
        var same = true
        for (s in sel) {
            if (describe(s.e) != k0) {
                same = false
            }
        }
        var cv = 0.0
        if (same && !scalar_float_const(sel[0].e, cv)) return SWF_SPLAT
    }
    // ctor re-pack: only whole in-order bases ride free (a partial run would pay fresh extracts)
    if (nOut > nLanes) return SWF_NONE
    var runs : array<int2>
    swizzle_sel_runs(sel, runs)
    var bad = false
    for (r in runs) {
        if (sel[r.x].lane >= 0) {
            if (r.y != float_vec_dim(expr_bt(sel[r.x].e))) {
                bad = true
            } else {
                for (k in range(r.y)) {
                    if (sel[r.x + k].lane != k) {
                        bad = true
                    }
                }
            }
        }
    }
    // rebuilding the value's own arg list is churn, not a fold (e.g. `float2(a, a).yx`)
    if (!bad) {
        var vc = float_ctor_of(r2v_unwrap(that.value))
        if (vc != null && length(vc.arguments) == length(runs)) {
            var same = true
            for (idx in range(length(runs))) {
                if (describe(sel[runs[idx].x].e) != describe(vc.arguments[idx])) {
                    same = false
                }
            }
            bad = same
        }
    }
    delete runs
    return bad ? SWF_NONE : SWF_CTOR
}

// Oracle mirror for the swizzle fold arm (FoldResidualVisitor in flatten.das).
def public is_unfolded_swizzle_lanes(var e : ExprSwizzle?) : bool {
    var sel : array<LaneSrc>
    let k = swizzle_fold_classify(e, sel)
    unsafe {
        delete sel
    }
    return k != SWF_NONE
}

// Build the replacement classify picked. Source nodes are reused (the old tree is discarded
// whole); only a source landing in a second output lane gets cloned.
def public swizzle_fold_rewrite(var that : ExprSwizzle?) : ExpressionPtr {
    var sel : array<LaneSrc>
    let kind = swizzle_fold_classify(that, sel)
    var res : ExpressionPtr
    if (kind == SWF_VALUE) {
        res = that.value
    } elif (kind == SWF_BASE || kind == SWF_SCALAR) {
        res = sel[0].e
    } elif (kind == SWF_SWIZZLE) {
        var mask = ""
        for (s in sel) {
            mask += lane_letter(s.lane)     // nolint:PERF001 (a mask is at most 4 letters)
        }
        res = new ExprSwizzle(at = that.at, value = sel[0].e, mask := mask)
    } elif (kind == SWF_SPLAT) {
        var sc = sel[0].e
        res = make_float_splat(length(sel), that.at, sc)
    } elif (kind == SWF_CTOR) {
        var runs : array<int2>
        swizzle_sel_runs(sel, runs)
        var args : array<ExpressionPtr>
        args |> reserve(length(runs))
        var seen : table<string>
        for (r in runs) {
            var a = sel[r.x].e
            let k = describe(a)
            if (key_exists(seen, k)) {
                a = clone_expression(a)
            } else {
                seen |> insert(k)
            }
            args |> push(a)
        }
        res = make_float_ctor(length(sel), that.at, args)
        delete runs
        delete seen
        unsafe {
            delete args
        }
    }
    unsafe {
        delete sel
    }
    return res
}
