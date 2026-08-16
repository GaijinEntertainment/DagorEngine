options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_common shared private

//! Shared helpers for the daslib/flatten optimizer: the structural AST walk, type/const/ctor/chain helpers, and usage/mutability analysis. Required by every themed pass module.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require strings


// ===== Shared structural walk over the closed post-flatten node set =====
// Most optimization passes recurse the SAME bounded expression-node set (operators,
// calls, the typer's ref/value wrappers, index/field/swizzle, tuple/struct ctors).
// These primitives spell that child enumeration ONCE so a walker keeps only its head
// logic (per-leaf classification + the conservative unknown-node default). Only walkers
// that recurse the FULL set use these — passes with a deliberately narrower node set
// (is_all_const, pex_eligible, select_arm_eval_safe, …) stay explicit, since widening
// them here would silently change a mirror-paired transform/oracle gate.

def public is_walkable_node(e : ExpressionPtr) : bool {
    return (e is ExprOp1 || e is ExprOp2 || e is ExprOp3 || e is ExprCall ||
        e is ExprField || e is ExprSwizzle || e is ExprRef2Value || e is ExprCast ||
        e is ExprPtr2Ref || e is ExprAt || e is ExprMakeTuple || e is ExprMakeStruct ||
        e is ExprMakeArray)
}

// Hard guard for a REAL OPTIMIZER GAP: the walk hit a node it doesn't recognize (not walkable, not
// a leaf) and would silently under-traverse it. COMMENT OUT the guard_unhandled_node call in
// rewrite_children to unlock your work (fail-safe: a missed opt, never a miscompile), then report it.
def public guard_unhandled_node(e : ExpressionPtr) : void {
    if (e == null || e is ExprVar || e |> is_expr_const()) return   // legit leaves — not a gap
    macro_error(compiling_program(), e.at, "flatten optimizer reached an unhandled expression node '{e.__rtti}': this is a real gap in the optimizer (the pass silently skips it, so a uniform/mutable subtree inside escapes hoisting / CSE / safety analysis). Comment out guard_unhandled_node in daslib/flatten_opt_common.das to unlock your current work, then please report this node kind.")
}

// Replace each child slot of `e` in place with `fn(child)`; a known leaf is left alone, and an
// UNRECOGNIZED node trips guard_unhandled_node (a real optimizer gap — see above).
def public rewrite_children(var e : ExpressionPtr; fn : block<(var c : ExpressionPtr) : ExpressionPtr>) : void {
    if (e is ExprOp1) {
        var o = e as ExprOp1
        o.subexpr = invoke(fn, o.subexpr)
    } elif (e is ExprOp2) {
        var o = e as ExprOp2
        o.left = invoke(fn, o.left)
        o.right = invoke(fn, o.right)
    } elif (e is ExprOp3) {
        var o = e as ExprOp3
        o.subexpr = invoke(fn, o.subexpr)
        o.left = invoke(fn, o.left)
        o.right = invoke(fn, o.right)
    } elif (e is ExprCall) {
        var c = e as ExprCall
        for (i in range(length(c.arguments))) {
            c.arguments[i] = invoke(fn, c.arguments[i])
        }
    } elif (e is ExprField) {
        var o = e as ExprField
        o.value = invoke(fn, o.value)
    } elif (e is ExprSwizzle) {
        var o = e as ExprSwizzle
        o.value = invoke(fn, o.value)
    } elif (e is ExprRef2Value) {
        var o = e as ExprRef2Value
        o.subexpr = invoke(fn, o.subexpr)
    } elif (e is ExprCast) {
        var o = e as ExprCast
        o.subexpr = invoke(fn, o.subexpr)
    } elif (e is ExprPtr2Ref) {
        var o = e as ExprPtr2Ref
        o.subexpr = invoke(fn, o.subexpr)
    } elif (e is ExprAt) {
        var o = e as ExprAt
        o.subexpr = invoke(fn, o.subexpr)
        o.index = invoke(fn, o.index)
    } elif (e is ExprMakeTuple) {
        var o = e as ExprMakeTuple
        for (vi in range(length(o.values))) {
            o.values[vi] = invoke(fn, o.values[vi])
        }
    } elif (e is ExprMakeStruct) {
        var ms = e as ExprMakeStruct
        for (si in range(length(ms.structs))) {
            for (fld in *(ms.structs[si])) {
                fld.value = invoke(fn, fld.value)
            }
        }
    } elif (e is ExprMakeArray) {
        var o = e as ExprMakeArray
        for (vi in range(length(o.values))) {
            o.values[vi] = invoke(fn, o.values[vi])
        }
    } else {
        guard_unhandled_node(e)
    }
}

// OR over `e`'s children (short-circuit); vacuous false on a leaf / unknown node.
def public fold_children_or(e : ExpressionPtr; pred : block<(c : ExpressionPtr) : bool>) : bool {
    if (e is ExprOp1) {
        return invoke(pred, (e as ExprOp1).subexpr)
    } elif (e is ExprOp2) {
        let o = e as ExprOp2
        return invoke(pred, o.left) || invoke(pred, o.right)
    } elif (e is ExprOp3) {
        let o = e as ExprOp3
        return invoke(pred, o.subexpr) || invoke(pred, o.left) || invoke(pred, o.right)
    } elif (e is ExprCall) {
        for (a in (e as ExprCall).arguments) {
            if (invoke(pred, a)) return true
        }
    } elif (e is ExprField) {
        return invoke(pred, (e as ExprField).value)
    } elif (e is ExprSwizzle) {
        return invoke(pred, (e as ExprSwizzle).value)
    } elif (e is ExprRef2Value) {
        return invoke(pred, (e as ExprRef2Value).subexpr)
    } elif (e is ExprCast) {
        return invoke(pred, (e as ExprCast).subexpr)
    } elif (e is ExprPtr2Ref) {
        return invoke(pred, (e as ExprPtr2Ref).subexpr)
    } elif (e is ExprAt) {
        let o = e as ExprAt
        return invoke(pred, o.subexpr) || invoke(pred, o.index)
    } elif (e is ExprMakeTuple) {
        for (v in (e as ExprMakeTuple).values) {
            if (invoke(pred, v)) return true
        }
    } elif (e is ExprMakeStruct) {
        var ms = e as ExprMakeStruct
        for (si in range(length(ms.structs))) {
            for (fld in *(ms.structs[si])) {
                if (invoke(pred, fld.value)) return true
            }
        }
    } elif (e is ExprMakeArray) {
        for (v in (e as ExprMakeArray).values) {
            if (invoke(pred, v)) return true
        }
    }
    return false
}

def public is_flat_temp(name : das_string) : bool {
    return name |> starts_with("__flat_")
}

// An inlined-helper local (`__flat_loc_N`). Unlike the loop/live masks it must be SSA-renamed like
// a user accumulator — else its self-referential `var` loses its live init to DSE, leaving a
// null-init self-ref the shader backend rejects.
def public is_flat_inlined_local(name : das_string) : bool {
    return name |> starts_with("__flat_loc_")
}

// ===== Copy propagation of single-assignment flatten temps =====
// A single-def `__flat_*` temp whose source reads no later-reassigned var is a pure forward copy:
// inline the source at every use, drop the temp (a risky call/index/divide source only at one use).

class public FlatVarRef : AstVisitor {
    name  : string
    count : int
    def override preVisitExprVar(expr : ExprVar?) : void {
        // das_string compares with string directly — never build a temp string here:
        // count_refs walks the whole tree per candidate, so a per-ExprVar allocation
        // is O(n^2) persistent strings on large bodies (the heap-overflow amplifier).
        if (expr.name == name) count ++
    }
}

def public count_refs(var e : ExpressionPtr; name : string) : int {
    if (e == null) return 0
    var v = new FlatVarRef(name = name, count = 0)
    make_visitor(*v) $(adapter) {
        visit(e, adapter)
    }
    let c = v.count
    unsafe {
        delete v
    }
    return c
}

// Per-pass reference index: name-hash -> referencing statement indices (deduped) + total count.
// Built once per copy_prop pass, replacing the old per-candidate count_refs sweep that was
// O(n^3) on large bodies.
class public FlatUsesBuilder : AstVisitor {
    nameUses : table<uint64; array<int>>
    nameCount : table<uint64; int>
    seen : table<uint64>
    j : int
    def override preVisitExprVar(expr : ExprVar?) : void {
        let h = hash(expr.name)
        nameCount[h] ++
        if (!key_exists(seen, h)) {
            seen |> insert(h)
            nameUses[h] |> push(j)
        }
    }
}

// ===== Numeric-const grid: typed-lane carrier =====
// One extractor + one builder centralize the per-(element,width) ExprConst* ladder. Scope: the
// 32-bit grid (float/int/uint, 1..4); int8/16/64, double, bool stay on the scalar predicates below.

let public CK_NONE = 0
let public CK_FLOAT = 1
let public CK_INT = 2
let public CK_UINT = 3

struct public ConstLanes {
    kind : int       // CK_*
    dim  : int       // 1..4
    f    : float4    // float lanes (CK_FLOAT)
    i    : int4      // int/uint lanes (CK_INT signed; CK_UINT value as int)
}

//! Extract a 32-bit-grid numeric const (float/int/uint scalar or 2/3/4 vector) into typed lanes; kind = CK_NONE otherwise.
def public const_lanes(e : ExpressionPtr) : ConstLanes {
    var r : ConstLanes
    if (e == null) return r
    if (e is ExprConstFloat)  { r.kind = CK_FLOAT; r.dim = 1; r.f = float4((e as ExprConstFloat).value, 0.0, 0.0, 0.0); return r }
    if (e is ExprConstFloat2) { let v = (e as ExprConstFloat2).value; r.kind = CK_FLOAT; r.dim = 2; r.f = float4(v.x, v.y, 0.0, 0.0); return r }
    if (e is ExprConstFloat3) { let v = (e as ExprConstFloat3).value; r.kind = CK_FLOAT; r.dim = 3; r.f = float4(v.x, v.y, v.z, 0.0); return r }
    if (e is ExprConstFloat4) { r.kind = CK_FLOAT; r.dim = 4; r.f = (e as ExprConstFloat4).value; return r }
    if (e is ExprConstInt)    { r.kind = CK_INT;  r.dim = 1; r.i = int4((e as ExprConstInt).value, 0, 0, 0); return r }
    if (e is ExprConstInt2)   { let v = (e as ExprConstInt2).value; r.kind = CK_INT; r.dim = 2; r.i = int4(v.x, v.y, 0, 0); return r }
    if (e is ExprConstInt3)   { let v = (e as ExprConstInt3).value; r.kind = CK_INT; r.dim = 3; r.i = int4(v.x, v.y, v.z, 0); return r }
    if (e is ExprConstInt4)   { r.kind = CK_INT;  r.dim = 4; r.i = (e as ExprConstInt4).value; return r }
    if (e is ExprConstUInt)   { r.kind = CK_UINT; r.dim = 1; r.i = int4(int((e as ExprConstUInt).value), 0, 0, 0); return r }
    if (e is ExprConstUInt2)  { let v = (e as ExprConstUInt2).value; r.kind = CK_UINT; r.dim = 2; r.i = int4(int(v.x), int(v.y), 0, 0); return r }
    if (e is ExprConstUInt3)  { let v = (e as ExprConstUInt3).value; r.kind = CK_UINT; r.dim = 3; r.i = int4(int(v.x), int(v.y), int(v.z), 0); return r }
    if (e is ExprConstUInt4)  { let v = (e as ExprConstUInt4).value; r.kind = CK_UINT; r.dim = 4; r.i = int4(int(v.x), int(v.y), int(v.z), int(v.w)); return r }
    return r
}

//! Build a 32-bit-grid numeric const from typed lanes (the inverse of const_lanes); null if kind = CK_NONE.
def public make_const_lanes(L : ConstLanes; at : LineInfo) : ExpressionPtr {
    if (L.kind == CK_FLOAT) {
        if (L.dim == 1) return new ExprConstFloat(at = at, value = L.f.x)
        if (L.dim == 2) return new ExprConstFloat2(at = at, value = float2(L.f.x, L.f.y))
        if (L.dim == 3) return new ExprConstFloat3(at = at, value = float3(L.f.x, L.f.y, L.f.z))
        return new ExprConstFloat4(at = at, value = L.f)
    } elif (L.kind == CK_INT) {
        if (L.dim == 1) return new ExprConstInt(at = at, value = L.i.x)
        if (L.dim == 2) return new ExprConstInt2(at = at, value = int2(L.i.x, L.i.y))
        if (L.dim == 3) return new ExprConstInt3(at = at, value = int3(L.i.x, L.i.y, L.i.z))
        return new ExprConstInt4(at = at, value = L.i)
    } elif (L.kind == CK_UINT) {
        if (L.dim == 1) return new ExprConstUInt(at = at, value = uint(L.i.x))
        if (L.dim == 2) return new ExprConstUInt2(at = at, value = uint2(uint(L.i.x), uint(L.i.y)))
        if (L.dim == 3) return new ExprConstUInt3(at = at, value = uint3(uint(L.i.x), uint(L.i.y), uint(L.i.z)))
        return new ExprConstUInt4(at = at, value = uint4(uint(L.i.x), uint(L.i.y), uint(L.i.z), uint(L.i.w)))
    }
    return null
}

// A scalar zero/default literal (int 0, float 0.0, bool false, …). A `var x = <this>` init is
// redundant with daslang's zero-init of a bare `var x : T`, so it can be dropped. Non-scalar
// inits (vector/struct ctors) are not literals here and left alone.
def public is_zero_const(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprConstInt) return (e as ExprConstInt).value == 0
    if (e is ExprConstInt8) return (e as ExprConstInt8).value == int8(0)
    if (e is ExprConstInt16) return (e as ExprConstInt16).value == int16(0)
    if (e is ExprConstInt64) return (e as ExprConstInt64).value == 0l
    if (e is ExprConstUInt) return (e as ExprConstUInt).value == 0u
    if (e is ExprConstUInt8) return (e as ExprConstUInt8).value == uint8(0)
    if (e is ExprConstUInt16) return (e as ExprConstUInt16).value == uint16(0)
    if (e is ExprConstUInt64) return (e as ExprConstUInt64).value == 0ul
    if (e is ExprConstFloat) return (e as ExprConstFloat).value == 0.0f
    if (e is ExprConstDouble) return (e as ExprConstDouble).value == 0.0lf
    if (e is ExprConstBool) return !(e as ExprConstBool).value
    return false
}

// Extract an integer constant's value (widened to int64) and whether its type is
// signed. Float/double/bool consts are NOT integers and return false — the algebraic
// identities only hold for integer arithmetic.
def public int_const(e : ExpressionPtr; var v : int64&; var signed : bool&) : bool {
    if (e == null) return false
    if (e is ExprConstInt) { v = int64((e as ExprConstInt).value); signed = true; return true }
    if (e is ExprConstInt8) { v = int64((e as ExprConstInt8).value); signed = true; return true }
    if (e is ExprConstInt16) { v = int64((e as ExprConstInt16).value); signed = true; return true }
    if (e is ExprConstInt64) { v = (e as ExprConstInt64).value; signed = true; return true }
    if (e is ExprConstUInt) { v = int64((e as ExprConstUInt).value); signed = false; return true }
    if (e is ExprConstUInt8) { v = int64((e as ExprConstUInt8).value); signed = false; return true }
    if (e is ExprConstUInt16) { v = int64((e as ExprConstUInt16).value); signed = false; return true }
    if (e is ExprConstUInt64) { v = int64((e as ExprConstUInt64).value); signed = false; return true }
    return false
}

def public is_int_val(e : ExpressionPtr; want : int64) : bool {
    var v = 0l
    var signed = false
    return int_const(e, v, signed) && v == want
}

def public is_int_neg_one(e : ExpressionPtr) : bool {
    // Signed only — an unsigned "-1" is a large positive, not a negation factor.
    var v = 0l
    var signed = false
    return int_const(e, v, signed) && signed && v == -1l
}

// A scalar float/double const equal to `want`. `want` is exact (0/1/-1) in both
// float and double, so the widening compare is exact. Vector consts (ExprConstFloat3
// …) are NOT scalars and return false — is_real_vec_val (below) is the vector form.
def public is_real_val(e : ExpressionPtr; want : double) : bool {
    if (e == null) return false
    if (e is ExprConstFloat) return double((e as ExprConstFloat).value) == want
    if (e is ExprConstDouble) return (e as ExprConstDouble).value == want
    return false
}

// An all-lanes-equal float vector const (ExprConstFloat2/3/4) equal to `want`. Extends scalar
// is_real_val so the float identities fire for vector-const operands (`v * float3(1)`); `want`
// is exact (0/1/-1) so the widening compare is exact.
def public is_real_vec_val(e : ExpressionPtr; want : double) : bool {
    let L = const_lanes(e)
    if (L.kind != CK_FLOAT || L.dim < 2) return false
    for (k in range(L.dim)) {
        if (double(L.f[k]) != want) return false
    }
    return true
}

// An all-lanes-equal int OR uint vector const equal to `want` (want >= 0). Mirrors is_int_val
// covering both signed and unsigned scalar consts — uint vectors fold for 0/1 but not -1.
def public is_int_vec_val(e : ExpressionPtr; want : int64) : bool {
    let L = const_lanes(e)
    if ((L.kind != CK_INT && L.kind != CK_UINT) || L.dim < 2) return false
    for (k in range(L.dim)) {
        if (int64(L.i[k]) != want) return false
    }
    return true
}

// An all-lanes -1 signed int vector const (ExprConstInt2/3/4 only). Mirrors is_int_neg_one —
// an unsigned vector "-1" is a large positive, never a negation factor (the "uint ≈ int,
// almost" boundary).
def public is_int_vec_neg_one(e : ExpressionPtr) : bool {
    let L = const_lanes(e)
    if (L.kind != CK_INT || L.dim < 2) return false
    for (k in range(L.dim)) {
        if (L.i[k] != -1) return false
    }
    return true
}

// Combined const-operand predicates for the algebraic identities: scalar OR all-lanes-equal
// vector const. `*0`/`+0`/`-0` use is_zero_const_op, `*1` uses is_one_const_op, `*-1` uses
// is_neg_one_const_op (signed/float only).
def public is_zero_const_op(e : ExpressionPtr) : bool {
    return is_int_val(e, 0l) || is_real_val(e, 0.0lf) || is_int_vec_val(e, 0l) || is_real_vec_val(e, 0.0lf)
}

def public is_one_const_op(e : ExpressionPtr) : bool {
    return is_int_val(e, 1l) || is_real_val(e, 1.0lf) || is_int_vec_val(e, 1l) || is_real_vec_val(e, 1.0lf)
}

def public is_neg_one_const_op(e : ExpressionPtr) : bool {
    return is_int_neg_one(e) || is_real_val(e, -1.0lf) || is_int_vec_neg_one(e) || is_real_vec_val(e, -1.0lf)
}

// A const bool equal to `want`. Backs the boolean algebraic identities
// (`true && x → x`, `c ? true : false → c`, `!false → true`, …).
def public is_bool_val(e : ExpressionPtr; want : bool) : bool {
    if (e == null) return false
    if (e is ExprConstBool) return (e as ExprConstBool).value == want
    return false
}

def public negate(var e : ExpressionPtr) : ExpressionPtr {
    return new ExprOp1(at = e.at, op := "-", subexpr = e)
}

// A fresh zero literal of `t` (the RESULT type of `X * 0`; also the lowering's materialized
// implicit zero-init on bare decls), over the numeric scalar + vector family; null otherwise.
// Keying on the result type lets both `v * 0.0` and `v * float3(0)` fold width-matched.
def public zero_const_of(t : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    if (t == null) return null
    match (t.baseType) {
        if (Type.tInt) { return new ExprConstInt(at = at, value = 0) }
        if (Type.tInt8) { return new ExprConstInt8(at = at, value = int8(0)) }
        if (Type.tInt16) { return new ExprConstInt16(at = at, value = int16(0)) }
        if (Type.tInt64) { return new ExprConstInt64(at = at, value = 0l) }
        if (Type.tUInt) { return new ExprConstUInt(at = at, value = 0u) }
        if (Type.tUInt8) { return new ExprConstUInt8(at = at, value = uint8(0)) }
        if (Type.tUInt16) { return new ExprConstUInt16(at = at, value = uint16(0)) }
        if (Type.tUInt64) { return new ExprConstUInt64(at = at, value = 0ul) }
        if (Type.tFloat) { return new ExprConstFloat(at = at, value = 0.0f) }
        if (Type.tDouble) { return new ExprConstDouble(at = at, value = 0.0lf) }
        if (Type.tFloat2) { return new ExprConstFloat2(at = at, value = float2(0.0)) }
        if (Type.tFloat3) { return new ExprConstFloat3(at = at, value = float3(0.0)) }
        if (Type.tFloat4) { return new ExprConstFloat4(at = at, value = float4(0.0)) }
        if (Type.tInt2) { return new ExprConstInt2(at = at, value = int2(0)) }
        if (Type.tInt3) { return new ExprConstInt3(at = at, value = int3(0)) }
        if (Type.tInt4) { return new ExprConstInt4(at = at, value = int4(0)) }
        if (Type.tUInt2) { return new ExprConstUInt2(at = at, value = uint2(0u)) }
        if (Type.tUInt3) { return new ExprConstUInt3(at = at, value = uint3(0u)) }
        if (Type.tUInt4) { return new ExprConstUInt4(at = at, value = uint4(0u)) }
    }
    return null
}

def public expr_bt(e : ExpressionPtr) : Type {
    return (e == null || e._type == null) ? Type.none : e._type.baseType
}

// Scalar element of a vector baseType (tFloat3 → tFloat); Type.none for non-vectors.
def public vec_elem_bt(bt : Type) : Type {
    if (bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4) return Type.tFloat
    if (bt == Type.tInt2 || bt == Type.tInt3 || bt == Type.tInt4) return Type.tInt
    if (bt == Type.tUInt2 || bt == Type.tUInt3 || bt == Type.tUInt4) return Type.tUInt
    return Type.none
}

def public is_float_family(bt : Type) : bool {
    return bt == Type.tFloat || bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4
}

// Float-valued (float/double scalar or float vector) — the fast-math gates key on this:
// integer/bool identities are exact, floating-point ones change inf/NaN/rounding behavior.
def public is_float_valued(e : ExpressionPtr) : bool {
    let bt = expr_bt(e)
    return is_float_family(bt) || bt == Type.tDouble
}

// Full-arity float/int/uint vector ctor call (`float3(a,b,c)` — one scalar arg per lane):
// returns the lane count; 0 for splats, mixed-arity ctors (`float3(v2,z)`) and non-ctor calls.
def public ctor_dim_by_name(c : ExprCall?) : int {
    if (c == null) return 0
    let nm = simple_name("{c.name}")
    var d = 0
    if (nm == "float2" || nm == "int2" || nm == "uint2") {
        d = 2
    } elif (nm == "float3" || nm == "int3" || nm == "uint3") {
        d = 3
    } elif (nm == "float4" || nm == "int4" || nm == "uint4") {
        d = 4
    }
    return length(c.arguments) == d ? d : 0
}

def public is_float_vec(bt : Type) : bool {
    return bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4
}

// The scalar argument of a float splat ctor (`floatN(s)`); null otherwise.
def public splat_scalar_arg(e : ExpressionPtr) : ExpressionPtr {
    if (e == null || !(e is ExprCall)) return null
    var c = e as ExprCall
    let nm = simple_name("{c.name}")
    if ((nm != "float2" && nm != "float3" && nm != "float4") || length(c.arguments) != 1) return null
    return expr_bt(c.arguments[0]) == Type.tFloat ? c.arguments[0] : null
}

// Bitmask of zero lanes of a const float/int/uint vector literal; 0 when none (or not one).
def public const_vec_zero_mask(e : ExpressionPtr) : int {
    let L = const_lanes(e)
    if (L.kind == CK_NONE || L.dim < 2) return 0
    var m = 0
    for (k in range(L.dim)) {
        let isZero = L.kind == CK_FLOAT ? (L.f[k] == 0.0) : (L.i[k] == 0)
        if (isZero) {
            m |= (1 << k)
        }
    }
    return m
}


// The (T,T,T) element family das `mad` covers: float/int/uint scalar+vector, double scalar.
def public is_mad_elem(bt : Type) : bool {
    return (is_float_family(bt) || bt == Type.tDouble ||
        bt == Type.tInt || bt == Type.tInt2 || bt == Type.tInt3 || bt == Type.tInt4 ||
        bt == Type.tUInt || bt == Type.tUInt2 || bt == Type.tUInt3 || bt == Type.tUInt4)
}

// `mad(a, b, c)` overload shapes (das math): (T,T,T) over the numeric family, or the
// vec-scalar-vec broadcast form (MadS). The single gate for expand, fuse AND residual.
def public mad_shape(ta, tb, tc : Type) : bool {
    if (ta == tb && tb == tc) return is_mad_elem(ta)
    return ta == tc && tb != Type.none && vec_elem_bt(ta) == tb
}

// `lerp(a, b, t)` shapes safe to expand AND re-fuse: float family with scalar `t` only — the
// one shape every provider has (das math AND the scalar-`t` backend DSL stubs). A vector-`t`
// re-fusion of raw `(b-a)*tv + a` arithmetic would emit a call a DSL-only module can't resolve.
def public lerp_shape(ta, tb, tt : Type) : bool {
    return ta == tb && is_float_family(ta) && tt == Type.tFloat
}

// True if `e` is built only from const literals and pure ops/builtins — no var/global read,
// no user call. eval_single_expression runs in a bare context (no globals, no user functions),
// so the tree must bottom out in literals and pure builtins. Unknown node → false.
def public is_all_const(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e |> is_expr_const()) return true
    if (e is ExprVar) return false
    if (e is ExprOp1) return is_all_const((e as ExprOp1).subexpr)
    if (e is ExprOp2) {
        let o = e as ExprOp2
        return is_all_const(o.left) && is_all_const(o.right)
    }
    if (e is ExprOp3) {
        let o = e as ExprOp3
        return is_all_const(o.subexpr) && is_all_const(o.left) && is_all_const(o.right)
    }
    if (e is ExprCall) {
        let c = e as ExprCall
        // Reject a user/unresolved call (not evaluable) or any side-effecting builtin.
        if (c.func == null || !c.func.flags.builtIn || has_sideeffects(e)) return false
        for (a in c.arguments) {
            if (!is_all_const(a)) return false
        }
        return true
    }
    return false
}

// True if a constant of this type can be rebuilt as a literal by eval_to_const — the
// float4-based evaluator covers the float/int/uint scalar + vector family. Gates the cycle-1
// const-builtin allowance so only foldable-type calls bypass the whitelist.
def public eval_to_const_supported(t : TypeDeclPtr) : bool {
    if (t == null) return false
    let bt = t.baseType
    return (bt == Type.tFloat || bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4 ||
        bt == Type.tInt || bt == Type.tInt2 || bt == Type.tInt3 || bt == Type.tInt4 ||
        bt == Type.tUInt || bt == Type.tUInt2 || bt == Type.tUInt3 || bt == Type.tUInt4)
}

// True if every operator/call node in the subtree carries a settled type. eval_single_expression
// assumes inference is complete (an un-inferred operator has null `.func` → crash), so this guards
// eval against a not-yet-re-inferred subtree (a fold-built operand under an inferred all-const root).
def public fully_inferred(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprOp1) {
        return e._type != null && fully_inferred((e as ExprOp1).subexpr)
    }
    if (e is ExprOp2) {
        let o = e as ExprOp2
        return e._type != null && fully_inferred(o.left) && fully_inferred(o.right)
    }
    if (e is ExprOp3) {
        let o = e as ExprOp3
        return e._type != null && fully_inferred(o.subexpr) && fully_inferred(o.left) && fully_inferred(o.right)
    }
    if (e is ExprCall) {
        if (e._type == null) {
            return false
        }
        for (a in (e as ExprCall).arguments) {
            if (!fully_inferred(a)) {
                return false
            }
        }
        return true
    }
    return true
}

// Evaluate a literal-only (is_all_const) inferred expression to one const literal of
// its type, via the exact runtime evaluator. Returns `e` unchanged if eval fails or
// the type is unsupported. The fresh const is un-inferred; the macro re-infers it.
def public eval_to_const(var e : ExpressionPtr) : ExpressionPtr {
    if (e == null || e._type == null || !fully_inferred(e)) return e
    var ok = false
    var val : float4
    unsafe {
        val = eval_single_expression(e, ok)
    }
    if (!ok) return e
    let bt = e._type.baseType
    if (bt == Type.tFloat) return new ExprConstFloat(at = e.at, value = val.x)
    if (bt == Type.tFloat2) return new ExprConstFloat2(at = e.at, value = float2(val.x, val.y))
    if (bt == Type.tFloat3) return new ExprConstFloat3(at = e.at, value = float3(val.x, val.y, val.z))
    if (bt == Type.tFloat4) return new ExprConstFloat4(at = e.at, value = val)
    unsafe {
        if (bt == Type.tInt) return new ExprConstInt(at = e.at, value = reinterpret<int>(val.x))
        if (bt == Type.tInt2) return new ExprConstInt2(at = e.at, value = int2(reinterpret<int>(val.x), reinterpret<int>(val.y)))
        if (bt == Type.tInt3) return new ExprConstInt3(at = e.at, value = int3(reinterpret<int>(val.x), reinterpret<int>(val.y), reinterpret<int>(val.z)))
        if (bt == Type.tInt4) return new ExprConstInt4(at = e.at, value = int4(reinterpret<int>(val.x), reinterpret<int>(val.y), reinterpret<int>(val.z), reinterpret<int>(val.w)))
        if (bt == Type.tUInt) return new ExprConstUInt(at = e.at, value = reinterpret<uint>(val.x))
        if (bt == Type.tUInt2) return new ExprConstUInt2(at = e.at, value = uint2(reinterpret<uint>(val.x), reinterpret<uint>(val.y)))
        if (bt == Type.tUInt3) return new ExprConstUInt3(at = e.at, value = uint3(reinterpret<uint>(val.x), reinterpret<uint>(val.y), reinterpret<uint>(val.z)))
        if (bt == Type.tUInt4) return new ExprConstUInt4(at = e.at, value = uint4(reinterpret<uint>(val.x), reinterpret<uint>(val.y), reinterpret<uint>(val.z), reinterpret<uint>(val.w)))
    }
    return e
}

// ===== Generalized const-propagation of single-def user locals =====
// Post-SSA-rename every user local is single-def, so a `var x = <const>` substitutes at all uses and
// drops. One forward pass folds a whole accumulator chain; masks are copy_prop's job, globals never.

def public reads_any(var e : ExpressionPtr; names : table<string; ExpressionPtr>) : bool {
    for (nm in keys(names)) {
        if (count_refs(e, nm) > 0) return true
    }
    return false
}

def public subst_consts(var e : ExpressionPtr; constmap : table<string; ExpressionPtr>) : ExpressionPtr {
    if (e == null) return null
    var tmpl : Template
    for (nm in keys(constmap)) {
        // ?[] yields a const view; const-strip the raw pointer (apply_template clones per use anyway).
        var ex = unsafe(reinterpret<Expression?>(constmap?[nm] ?? null))
        tmpl |> replaceVariable(nm, ex)
    }
    var res = apply_template(tmpl, e.at, e, false)
    unsafe { delete tmpl }
    return res
}

// ===== Constant reassociation =====
// Gather scattered consts in `+`/`-`/`*` chains into one adjacent group (`0.5 + a + 0.6` → `a +
// (0.5+0.6)`) for the all-const arm to fold; also sort var terms into a canonical order (for a later CSE).

struct public ReTerm {
    neg : bool          // subtracted (additive chains); always false for the `*` monoid
    e : ExpressionPtr
}

// Flatten a maximal additive chain into signed terms (left-to-right = source order). Descends
// `+`/`-` ExprOp2 and unary `-`; everything else is a leaf. No same-base-type gate (vector±scalar
// does not compile, so an additive chain is always same-type).
def public collect_add(var e : ExpressionPtr; neg : bool; var terms : array<ReTerm>) {
    if (e == null) return
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if (o.op == "+" || o.op == "-") {
            collect_add(o.left, neg, terms)
            collect_add(o.right, o.op == "-" ? !neg : neg, terms)
            return
        }
    } elif (e is ExprOp1) {
        var o = e as ExprOp1
        if (o.op == "-") {
            collect_add(o.subexpr, !neg, terms)
            return
        }
    }
    terms |> push(ReTerm(neg = neg, e = e))
}

// Flatten a maximal `*` chain into factors (no `/` in v1). Descends `*` ExprOp2 only.
def public collect_mul(var e : ExpressionPtr; var terms : array<ReTerm>) {
    if (e == null) return
    if (e is ExprOp2) {
        var o = e as ExprOp2
        if (o.op == "*") {
            collect_mul(o.left, terms)
            collect_mul(o.right, terms)
            return
        }
    }
    terms |> push(ReTerm(neg = false, e = e))
}

// Build a left-leaning `<t0> op <t1> …` tree from non-empty `terms`, REUSING the operand nodes
// (each leaf appears once, so it lands in one slot; the old chain wrappers drop when the visitor
// replaces `that`). A negative first term becomes unary `-`; later terms pick addOp/subOp by sign.
def public build_chain(var terms : array<ReTerm>; at : LineInfo; addOp, subOp : string) : ExpressionPtr {
    var acc = terms[0].neg ? negate(terms[0].e) : terms[0].e
    for (i in range(1, length(terms))) {
        acc = new ExprOp2(at = at, op := terms[i].neg ? subOp : addOp, left = acc, right = terms[i].e)
    }
    return acc
}

// ===== Same-condition select-accumulation fusion =====
// An invariant-guarded unrolled accumulation emits N selects on ONE condition `c`; hoist `c` out
// (strip all but the last select; last's false arm := base). Op tree preserved → bit-exact, not fast-math.

def public sel3(e : ExpressionPtr) : ExprOp3? {
    return (e != null && e is ExprOp3) ? (e as ExprOp3) : null   // ExprOp3 is the ternary `?:`
}

// Bare ExprVar name under an optional ExprRef2Value wrap (a value-context local read), else "".
def public ref_var_name(e : ExpressionPtr) : string {
    var x = e
    if (x != null && x is ExprRef2Value) {
        x = (x as ExprRef2Value).subexpr
    }
    return (x != null && x is ExprVar) ? "{(x as ExprVar).name}" : ""
}

// ===== Preshader extraction + CSE =====
// Two pure-AST passes on the converged body: hoist maximal globals-only subtrees to top-of-body
// `_preshader_N` lets, then CSE-dedup repeated subtrees. See flatten.rst / the plan for the why.

def public simple_name(fullname : string) : string {
    let t1 = find(fullname, 96, 0)
    if (t1 < 0) return fullname
    let t2 = find(fullname, 96, t1 + 1)
    let endp = t2 < 0 ? length(fullname) : t2
    return slice(fullname, t1 + 1, endp)
}

// ===== CSE (local value numbering) =====
// The body is one basic block, so LVN is complete. Value-number pure subtrees by `describe()`, dedup
// any computed >=2x into one shared let before its first use; uniform dups route to the preshader.

// The base ExprVar of a store target, digging through index/field/swizzle/deref wrappers —
// `G[i] = v` makes G's reads value-unstable exactly like `G = v` does.
def public store_base(e : Expression const?) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprAt) return store_base((e as ExprAt).subexpr)
    if (e is ExprField) return store_base((e as ExprField).value)
    if (e is ExprSwizzle) return store_base((e as ExprSwizzle).value)
    if (e is ExprPtr2Ref) return store_base((e as ExprPtr2Ref).subexpr)
    if (e is ExprRef2Value) return store_base((e as ExprRef2Value).subexpr)
    return e is ExprVar ? unsafe(reinterpret<Expression?>(e)) : null
}

// Names reassigned somewhere in the block (live/loop masks `__flat_*`, any reassigned user `var`).
// A subtree reading one of these is NOT value-stable — CSE must not share it (its value differs
// per occurrence), so it is excluded from value numbering. Single-def `__ssa_*` / `let` are safe.
class public MutCollect : AstVisitor {
    names : table<string>
    def override preVisitExprCopy(expr : ExprCopy?) : void {
        let base = store_base(expr.left)
        if (base != null) {
            names |> insert("{(base as ExprVar).name}")
        }
    }
}

def public collect_mutable(blk : ExprBlock?) : table<string> {
    var mc = new MutCollect()
    make_visitor(*mc) $(a) {
        visit(blk, a)
    }
    var r : table<string>
    for (n in keys(mc.names)) {
        r |> insert(n)
    }
    unsafe {
        delete mc
    }
    return <- r
}

// One structural walk (O(size)) over the closed post-flatten node set, replacing the old
// O(#mutableVars * size) per-name count_refs loop. An unknown node kind is conservatively treated
// as reading a mutable (never CSE through it) — correctness over completeness for this safety gate.
def public reads_mutable(e : ExpressionPtr; mutableVars : table<string>) : bool {
    if (e == null || empty(mutableVars) || e |> is_expr_const()) return false
    if (e is ExprVar) return key_exists(mutableVars, "{(e as ExprVar).name}")
    if (!is_walkable_node(e)) return true   // unknown node → conservatively reads a mutable
    return fold_children_or(e) $(c) => reads_mutable(c, mutableVars)
}

// ===== mad/lerp re-fusion (PHASE_FUSED finishing pass) =====
// The fold DISASSEMBLES lerp/mad into raw arithmetic (identities fold, reassoc/CSE see through);
// this re-packs survivors: `a*b ± c` → mad, then `mad(b - a, t, a)` → lerp — one node each.

// A reciprocal copy of a scalar/vector float const literal (`C` → `1/C`); null for any other
// node or when any lane is zero (no inf literals). Backs the `x / C → x * (1/C)` fold arm.
def public recip_const(e : ExpressionPtr) : ExpressionPtr {
    if (e is ExprConstDouble) {
        let v = (e as ExprConstDouble).value
        if (v == 0.0lf) return null
        return new ExprConstDouble(at = e.at, value = 1.0lf / v)
    }
    var L = const_lanes(e)
    if (L.kind != CK_FLOAT) return null
    for (k in range(L.dim)) {
        if (L.f[k] == 0.0) return null   // a zero lane keeps the div (no inf literal)
    }
    L.f = float4(1.0, 1.0, 1.0, 1.0) / L.f   // padding lanes (k >= dim) go inf but make_const_lanes drops them
    return make_const_lanes(L, e.at)
}

// All-const lanes become a const VECTOR LITERAL (the fold phase is over by fuse time, so an
// all-const ctor CALL would survive un-folded and trip the fold-residual oracle).
def public make_float_ctor(d : int; at : LineInfo; var lanes : array<ExpressionPtr>) : ExpressionPtr {
    var c0 = 0.0
    var c1 = 0.0
    var c2 = 0.0
    var c3 = 0.0
    // const-collapse needs one scalar arg per lane — the swizzle re-pack passes mixed-arity lists
    if (length(lanes) == d && scalar_float_const(lanes[0], c0) && scalar_float_const(lanes[1], c1) &&
            (d < 3 || scalar_float_const(lanes[2], c2)) && (d < 4 || scalar_float_const(lanes[3], c3))) {
        if (d == 2) return new ExprConstFloat2(at = at, value = float2(c0, c1))
        if (d == 3) return new ExprConstFloat3(at = at, value = float3(c0, c1, c2))
        return new ExprConstFloat4(at = at, value = float4(c0, c1, c2, c3))
    }
    let nm = d == 2 ? "float2" : (d == 3 ? "float3" : "float4")
    var c = new ExprCall(at = at, name := nm)
    for (l in lanes) {
        emplace(c.arguments, l)
    }
    return c
}

// Single-component read (`b.x` … `b.w`) of a float vector — ExprSwizzle post-infer, ExprField
// when freshly built. Yields the base, its describe key, and the component letter.
def public component_read_of(var e : ExpressionPtr; var baseKey : string&; var base : ExpressionPtr&; var comp : string&) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return component_read_of((e as ExprRef2Value).subexpr, baseKey, base, comp)
    var v : ExpressionPtr
    var nm = ""
    match (e) {
        if (ExprSwizzle(mask = $v(swMask), value = $v(swValue))) {
            nm = "{swMask}"
            v = swValue
        }
        if (ExprField(name = $v(flName), value = $v(flValue))) {
            nm = "{flName}"
            v = flValue
        }
    }
    if (v == null ||
        (nm != "x" && nm != "y" && nm != "z" && nm != "w") ||
        !is_float_vec(expr_bt(v)) || expr_bt(e) != Type.tFloat) return false
    base = v
    baseKey = describe(v)
    comp = nm
    return true
}

def public scalar_float_const(e : ExpressionPtr; var v : float&) : bool {
    let L = const_lanes(e)
    if (L.kind != CK_FLOAT || L.dim != 1) return false
    v = L.f.x
    return true
}

// The ctor re-pack PHASE_FUSED applies (CLONE-built so the residual oracle probes with it
// non-destructively): re-vectorize lanes / extract a shared factor or divisor (exact), or
// compensate a majority const factor (fast-math, only when the multiply count strictly drops).
def public make_float_splat(d : int; at : LineInfo; var sc : ExpressionPtr) : ExpressionPtr {
    let nm = d == 2 ? "float2" : (d == 3 ? "float3" : "float4")
    var c = new ExprCall(at = at, name := nm)
    emplace(c.arguments, sc)
    return c
}

// ===== horizontal add → dot re-pack =====
// `v.x + v.y (+ v.z + v.w)` — and weighted/negated lanes `v.x * c` — collapse into `dot(v, mask)`: one
// node where every lane read is an instruction. Fast-math (reorders adds, ×0 a dead-lane inf/NaN).

def public float_vec_dim(bt : Type) : int {
    if (bt == Type.tFloat2) return 2
    if (bt == Type.tFloat3) return 3
    if (bt == Type.tFloat4) return 4
    return 0
}

def public lane_index(comp : string) : int {
    if (comp == "x") return 0
    if (comp == "y") return 1
    if (comp == "z") return 2
    return 3
}

// Partition M lanes into groups of width [minWidth..4]. A tail smaller than minWidth is NOT grouped (the caller leaves those lanes unpacked) — so sum(result) may be < M. A too-small tail is rebalanced by shrinking the previous take when that keeps both groups >= minWidth (so M=5,minWidth=2 -> [3,2], not [4]+1). Shared by the min/max reduction pack (minWidth=2) and the hadd fold (minWidth 2 or 3).
def public plan_groups(M : int; minWidth : int = 2) : array<int> {
    assert(minWidth >= 2 && minWidth <= 4, "plan_groups expects minWidth in [2..4]")   // minWidth<=0 would divide by zero below / never terminate
    var g : array<int>
    g |> reserve((M + minWidth - 1) / minWidth)
    var rem = M
    while (rem >= minWidth) {
        var take = rem >= 4 ? 4 : rem   // nolint:PERF015 — math.min not required in this module
        let tail = rem - take
        if (tail > 0 && tail < minWidth && take - (minWidth - tail) >= minWidth) {
            take = take - (minWidth - tail)
        }
        g |> push(take)
        rem = rem - take
    }
    return <- g
}
