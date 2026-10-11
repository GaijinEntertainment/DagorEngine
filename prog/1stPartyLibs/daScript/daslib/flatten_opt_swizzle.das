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
    lane : int
    e : ExpressionPtr
}

def private lane_letter(i : int) : string {
    if (i == 0) return "x"
    if (i == 1) return "y"
    if (i == 2) return "z"
    return "w"
}

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

def private swizzle_decomposable(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return swizzle_decomposable((e as ExprRef2Value).subexpr)
    if (e is ExprConstFloat2 || e is ExprConstFloat3 || e is ExprConstFloat4) return true
    if (e is ExprSwizzle) return float_vec_dim(expr_bt((e as ExprSwizzle).value)) > 0
    return float_ctor_of(e) != null
}

def private resolve_lanes(var e : ExpressionPtr; var out : array<LaneSrc>) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return resolve_lanes((e as ExprRef2Value).subexpr, out)
    let d = float_vec_dim(expr_bt(e))
    if (d == 0) return false
    let L = const_lanes(e)
    if (L.kind == CK_FLOAT && L.dim >= 2) {
        for (k in range(L.dim)) {
            out |> push(LaneSrc(lane = -1, e = new ExprConstFloat(at = e.at, value = L.f[k])))   // nolint:PERF006 -- ≤4 lanes
        }
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
    if (c != null) return resolve_ctor_lanes(c, d, out)
    for (i in range(d)) {
        out |> push(LaneSrc(lane = i, e = e))   // nolint:PERF006 -- ≤4 lanes
    }
    return true
}

def private resolve_ctor_lanes(var c : ExprCall?; d : int; var out : array<LaneSrc>) : bool {
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

let private SWF_NONE = 0
let private SWF_VALUE = 1
let private SWF_BASE = 2
let private SWF_SWIZZLE = 3
let private SWF_SCALAR = 4
let private SWF_SPLAT = 5
let private SWF_CTOR = 6

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

def private resolve_selection(var that : ExprSwizzle?; var sel : array<LaneSrc>; var nLanes : int&) : bool {
    var lanes : array<LaneSrc>
    var ok = resolve_lanes(that.value, lanes)
    nLanes = length(lanes)
    if (ok) {
        for (f in that.fields) {
            if (int(f) >= nLanes) {
                ok = false
            }
        }
    }
    if (ok) {
        for (f in that.fields) {
            sel |> push(lanes[int(f)])
        }
    }
    unsafe {
        delete lanes
    }
    return ok
}

def private sel_all_same_source(sel : array<LaneSrc>) : bool {
    let k0 = describe(sel[0].e)
    for (s in sel) {
        if (describe(s.e) != k0) return false
    }
    return true
}

def private single_base_kind(sel : array<LaneSrc>; nOut : int) : int {
    var ident = nOut == float_vec_dim(expr_bt(sel[0].e))
    for (i in range(nOut)) {
        if (sel[i].lane != i) {
            ident = false
        }
    }
    return ident ? SWF_BASE : SWF_SWIZZLE
}

def private is_identity_selection(that : ExprSwizzle?; nOut, nLanes : int) : bool {
    if (nOut != nLanes) return false
    for (i in range(nOut)) {
        if (int(that.fields[i]) != i) return false
    }
    return true
}

def private ctor_repack_kind(var that : ExprSwizzle?; sel : array<LaneSrc>; nOut, nLanes : int) : int {
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

def private swizzle_fold_classify(var that : ExprSwizzle?; var sel : array<LaneSrc>) : int {
    if (that == null || that.fieldFlags.write) return SWF_NONE
    let nOut = length(that.fields)
    if (nOut < 1 || nOut > 4 || !swizzle_decomposable(that.value)) return SWF_NONE
    var nLanes = 0
    if (!resolve_selection(that, sel, nLanes)) return SWF_NONE
    var nScalar = 0
    for (s in sel) {
        if (s.lane < 0) {
            nScalar ++
        }
    }
    if (nScalar == 0 && sel_all_same_source(sel)) return single_base_kind(sel, nOut)
    if (nOut == 1) return SWF_SCALAR
    if (is_identity_selection(that, nOut, nLanes)) return SWF_VALUE
    var cv = 0.0
    if (nScalar == nOut && sel_all_same_source(sel) && !scalar_float_const(sel[0].e, cv)) return SWF_SPLAT
    return ctor_repack_kind(that, sel, nOut, nLanes)
}

def public is_unfolded_swizzle_lanes(var e : ExprSwizzle?) : bool {
    var sel : array<LaneSrc>
    let k = swizzle_fold_classify(e, sel)
    unsafe {
        delete sel
    }
    return k != SWF_NONE
}

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
