options gen2

require dastest/testing_boost
require math

// Accuracy and rail-consistency contract for the inline vector transcendentals the JIT emits
// instead of @llvm.<op>.vNf32 (which scalarize to N libm calls on every target of ours) - the
// build_vector_* emitters in daslib/llvm_jit_intrin.das. log2, log and pow take the rail on
// every target; the rest is aarch64-only (`on_aarch64`).
// Two questions per function, and they are different questions:
//   - vs libm: how good is the ALGORITHM. Several of these mirror vecmath estimates that are far
//     from correctly rounded; the bound records what the algorithm actually delivers, it is not
//     a quality target. log2/log/pow are the loose ones and their bounds say so.
//   - vs the interpreter: does the JIT agree with the rail the same program takes without -jit.
//     That is the property the emitters exist to preserve, so it is asserted separately. For
//     sin, cos, tan, exp2, log2, log and pow the bound is ZERO - the emitters reproduce vecmath
//     bit for bit, including where vecmath is wrong. tanh/sinh/cosh cannot reach zero and say why
//     at their cell. Each worker pair - apply_op / apply_op_interp, apply_sincos /
//     apply_sincos_interp - carries the SAME body; only [no_jit] separates the two.
//   - NaN in, NaN out: a clamp or a conversion written with ordered compares replaces a NaN lane
//     with a number, which no accuracy bound above can see. Asserted on both rails per emitter.
// Arguments arrive through runtime arrays on purpose: a compile-time-constant argument is
// const-folded by the AST and never reaches the intrinsic, so a folded test proves nothing.
// The tight bounds need a fused a*b+c on whichever rail runs. aarch64 always fuses; a generic
// x64 build has no FMA and takes the coarse bound, which still catches structural failures.

let private COARSE_REL = 1.0e-2         // unfused / non-polynomial rails: catches sign, branch and NaN failures
let private SINCOS_ABS = 3.0e-7         // measured 1.1920929e-07 (1 ulp) over |x| <= 1e5

// which: the op selector shared by the jitted and the interpreted worker
let private OP_TAN = 0
let private OP_EXP2 = 1
let private OP_LOG2 = 2
let private OP_LOG = 3
let private OP_POW = 4
let private OP_TANH = 5
let private OP_SINH = 6
let private OP_COSH = 7

struct private ErrReport {
    maxAbs : float
    at : float
    maxRel : float
    atRel : float
    nans : int
    lanes : int
}

// written before sampling so the samples cannot fold to constants at compile time
var private g_offset = 0.0

def private on_aarch64 : bool {
    return get_architecture_name() == "arm64"
}

def private op_on_poly_rail(which : int) : bool {
    return on_aarch64() || which == OP_LOG2 || which == OP_LOG || which == OP_POW
}

def private bound(tightValue : float) : float {
    return on_aarch64() ? tightValue : COARSE_REL
}

def private ramp(lo, hi : float; n : int) : array<float4> {
    var xs : array<float4>
    xs |> reserve(n)
    let step = (hi - lo) / float(n)
    for (i in range(n)) {
        let b = lo + float(i) * step + g_offset
        xs |> push(float4(b, b + step * 0.25, b + step * 0.5, b + step * 0.75))
    }
    return <- xs
}

// relFloor keeps lanes whose reference is near zero out of the RELATIVE statistic. That is not a
// convenience: log2 crosses zero at x == 1, so an absolute error of a few ulp there is an
// unbounded relative error and the relative number stops describing the algorithm. Those lanes are
// covered by the absolute bound instead, and both numbers are reported.
def private track(var r : ErrReport; got, ref, x, relFloor : float) {
    r.lanes++
    let e = abs(got - ref)
    if (!(e >= 0.0)) {    // NaN never compares >= 0
        r.nans++
        return
    }
    if (e > r.maxAbs) {
        r.maxAbs = e
        r.at = x
    }
    if (abs(ref) > relFloor) {
        let rel = e / abs(ref)
        if (rel > r.maxRel) {
            r.maxRel = rel
            r.atRel = x
        }
    }
}

def private reference_op(x, y : float; which : int) : float {
    if (which == OP_TAN) return float(sin(double(x)) / cos(double(x)))
    if (which == OP_EXP2) return float(pow(2.0lf, double(x)))
    if (which == OP_LOG2) return float(log(double(x)) / log(2.0lf))
    if (which == OP_LOG) return float(log(double(x)))
    if (which == OP_POW) return float(pow(double(x), double(y)))
    if (which == OP_TANH) return float(tanh(double(x)))
    if (which == OP_SINH) return float(sinh(double(x)))
    return float(cosh(double(x)))
}

def private apply_op(xs, ys : array<float4>; which : int) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v, y in xs, ys) {
        if (which == OP_TAN) {
            r |> push(tan(v))
        } elif (which == OP_EXP2) {
            r |> push(exp2(v))
        } elif (which == OP_LOG2) {
            r |> push(log2(v))
        } elif (which == OP_LOG) {
            r |> push(log(v))
        } elif (which == OP_POW) {
            r |> push(pow(v, y))
        } elif (which == OP_TANH) {
            r |> push(tanh(v))
        } elif (which == OP_SINH) {
            r |> push(sinh(v))
        } else {
            r |> push(cosh(v))
        }
    }
    return <- r
}

// the same body, pinned to the interpreter: [no_jit] keeps the SimNode tree, [never_inline] keeps
// a jitted caller from inlining it and taking the emitted form after all
[no_jit, never_inline]
def private apply_op_interp(xs, ys : array<float4>; which : int) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v, y in xs, ys) {
        if (which == OP_TAN) {
            r |> push(tan(v))
        } elif (which == OP_EXP2) {
            r |> push(exp2(v))
        } elif (which == OP_LOG2) {
            r |> push(log2(v))
        } elif (which == OP_LOG) {
            r |> push(log(v))
        } elif (which == OP_POW) {
            r |> push(pow(v, y))
        } elif (which == OP_TANH) {
            r |> push(tanh(v))
        } elif (which == OP_SINH) {
            r |> push(sinh(v))
        } else {
            r |> push(cosh(v))
        }
    }
    return <- r
}

def private op_error(xs, ys : array<float4>; which : int; relFloor : float = 1.0e-20) : ErrReport {
    var r : ErrReport
    let got <- apply_op(xs, ys, which)
    for (g, x, y in got, xs, ys) {
        for (l in range(4)) {
            track(r, g[l], reference_op(x[l], y[l], which), x[l], relFloor)
        }
    }
    return r
}

def private op_divergence(xs, ys : array<float4>; which : int; relFloor : float) : ErrReport {
    var r : ErrReport
    let got <- apply_op(xs, ys, which)
    let ref <- apply_op_interp(xs, ys, which)
    for (g, e, x in got, ref, xs) {
        for (l in range(4)) {
            track(r, g[l], e[l], x[l], relFloor)
        }
    }
    return r
}

def private report_rel(t : T?; what : string; r : ErrReport; relBound : float; wantLanes : int) {
    t |> equal(r.lanes, wantLanes, "{what}: lanes compared")
    t |> success(r.nans == 0, "{what}: no NaN lanes")
    t |> success(r.maxRel <= relBound, "{what}: max rel error {r.maxRel} at x = {r.atRel} (bound {relBound}); max abs {r.maxAbs} at {r.at}")
}

// the JIT must agree with the rail the same program takes without -jit. Off the polynomial rail
// the two run different algorithms by design (JIT keeps @llvm.*, interp keeps vecmath), and
// without -jit BOTH workers are the interpreter, so the comparison only says something where the
// emitters actually run - everywhere else it is reported as not applicable rather than green.
def private rail_applicable(onRail : bool) : bool {
    return jit_enabled() && onRail
}

def private not_applicable_reason(what : string) : string {
    return jit_enabled() ? "{what} is not on the polynomial rail on {get_architecture_name()}" : "{what}: not jitted"
}

def private report_div(t : T?; what : string; r : ErrReport; relBound : float; wantLanes : int) {
    t |> equal(r.lanes, wantLanes, "{what}: lanes compared")
    t |> success(r.maxRel <= relBound, "{what}: JIT vs interpreter max rel diff {r.maxRel} at x = {r.atRel} (bound {relBound}); max abs {r.maxAbs} at {r.at}")
}

def private report_divergence(t : T?; what : string; xs, ys : array<float4>; which : int; relBound : float; relFloor : float = 1.0e-20) {
    if (!rail_applicable(op_on_poly_rail(which))) {
        t |> success(true, "rail consistency not applicable: {not_applicable_reason(what)}")
        return
    }
    report_div(t, what, op_divergence(xs, ys, which, relFloor), relBound, length(xs) * 4)
}

// ===== sin / cos =====

def private build_sincos_samples : array<float4> {
    var xs : array<float4>
    xs |> reserve(10000 * 5 + 401 * 33 + 20000)
    // decade sweep, four unrelated magnitudes per entry, staying inside |x| <= 1e5
    for (i in range(10000)) {
        let u = float(i) * 2.0e-4 - 1.0
        for (d in range(5)) {
            let s = pow(10.0, float(d))
            xs |> push(float4(u * s, -u * s * 3.7, u * s * 0.31 + g_offset, u * s * 9.13))
        }
    }
    // fine steps across the quadrant boundaries, where the select tree swaps polynomials
    for (k in range(-200, 201)) {
        let b = float(k) * (PI * 0.5)
        for (j in range(-16, 17)) {
            let x = b + float(j) * 1.0e-4
            xs |> push(float4(x, x + 1.0e-6, x - 1.0e-6, -x))
        }
    }
    // the far end of the range, where the range reduction carries the largest quadrant count
    for (i in range(20000)) {
        let b = 99000.0 + float(i) * 5.0e-2
        xs |> push(float4(b, -b, b + 1.0e-2, -b - 1.0e-2))
    }
    return <- xs
}

def private sincos_error(xs : array<float4>; wantCos : bool) : ErrReport {
    var r : ErrReport
    for (v in xs) {
        let got = wantCos ? cos(v) : sin(v)
        for (l in range(4)) {
            let x = v[l]
            track(r, got[l], float(wantCos ? cos(double(x)) : sin(double(x))), x, 1.0e-20)
        }
    }
    return r
}

def private sin_error_narrow(xs : array<float4>; dim : int) : ErrReport {
    var r : ErrReport
    for (v in xs) {
        let got = dim == 2 ? float4(sin(float2(v.x, v.y)), 0.0, 0.0) : float4(sin(float3(v.x, v.y, v.z)), 0.0)
        for (l in range(dim)) {
            track(r, got[l], float(sin(double(v[l]))), v[l], 1.0e-20)
        }
    }
    return r
}

def private report_abs(t : T?; what : string; r : ErrReport; absBound : float; wantLanes : int) {
    t |> equal(r.lanes, wantLanes, "{what}: lanes compared")
    t |> success(r.nans == 0, "{what}: no NaN lanes")
    t |> success(r.maxAbs <= absBound, "{what}: max abs error {r.maxAbs} at x = {r.at} (bound {absBound})")
}

// sin and cos need their own worker pair: sincos_error above compares against libm in one pass, and
// the rail comparison needs the two rails to run the SAME expression. dim picks the emitted vector
// width - the emitter is width-generic, so float2 and float3 are separate emissions, not the float4
// one truncated. Widths below 4 leave the unused lanes at zero and only the first dim are read.
def private apply_sincos(xs : array<float4>; dim : int; wantCos : bool) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v in xs) {
        if (dim == 2) {
            r |> push(float4(wantCos ? cos(float2(v.x, v.y)) : sin(float2(v.x, v.y)), 0.0, 0.0))
        } elif (dim == 3) {
            r |> push(float4(wantCos ? cos(float3(v.x, v.y, v.z)) : sin(float3(v.x, v.y, v.z)), 0.0))
        } else {
            r |> push(wantCos ? cos(v) : sin(v))
        }
    }
    return <- r
}

[no_jit, never_inline]
def private apply_sincos_interp(xs : array<float4>; dim : int; wantCos : bool) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v in xs) {
        if (dim == 2) {
            r |> push(float4(wantCos ? cos(float2(v.x, v.y)) : sin(float2(v.x, v.y)), 0.0, 0.0))
        } elif (dim == 3) {
            r |> push(float4(wantCos ? cos(float3(v.x, v.y, v.z)) : sin(float3(v.x, v.y, v.z)), 0.0))
        } else {
            r |> push(wantCos ? cos(v) : sin(v))
        }
    }
    return <- r
}

def private sincos_divergence(xs : array<float4>; dim : int; wantCos : bool) : ErrReport {
    var r : ErrReport
    let got <- apply_sincos(xs, dim, wantCos)
    let ref <- apply_sincos_interp(xs, dim, wantCos)
    for (g, e, x in got, ref, xs) {
        for (l in range(dim)) {
            track(r, g[l], e[l], x[l], 1.0e-20)
        }
    }
    return r
}

def private report_sincos_divergence(t : T?; what : string; xs : array<float4>; dim : int; wantCos : bool) {
    if (!rail_applicable(on_aarch64())) {
        t |> success(true, "rail consistency not applicable: {not_applicable_reason(what)}")
        return
    }
    report_div(t, what, sincos_divergence(xs, dim, wantCos), 0.0, length(xs) * dim)
}

[test]
def test_vector_sin_matches_libm(t : T?) {
    g_offset = 0.0
    let xs <- build_sincos_samples()
    t |> success(!jit_enabled() || is_jit_function(@@apply_op), "the op worker is jitted")
    t |> success(!jit_enabled() || !is_jit_function(@@apply_op_interp), "the reference worker stays interpreted")
    t |> success(!jit_enabled() || is_jit_function(@@sincos_error), "the sin/cos libm worker is jitted")
    report_abs(t, "sin(float4)", sincos_error(xs, false), bound(SINCOS_ABS), length(xs) * 4)
}

// sin and cos mirror v_sincos instruction for instruction at every width the emitter serves, so the
// two rails agree exactly - the zero bound tan carries, at float4, float3 and float2.
[test]
def test_vector_sin_cos_rail_consistency(t : T?) {
    g_offset = 0.0
    let xs <- build_sincos_samples()
    t |> success(!jit_enabled() || is_jit_function(@@apply_sincos), "the sin/cos rail worker is jitted")
    t |> success(!jit_enabled() || !is_jit_function(@@apply_sincos_interp), "the sin/cos reference worker stays interpreted")
    report_sincos_divergence(t, "sin(float4)", xs, 4, false)
    report_sincos_divergence(t, "cos(float4)", xs, 4, true)
    report_sincos_divergence(t, "sin(float3)", xs, 3, false)
    report_sincos_divergence(t, "cos(float3)", xs, 3, true)
    report_sincos_divergence(t, "sin(float2)", xs, 2, false)
    report_sincos_divergence(t, "cos(float2)", xs, 2, true)
}

[test]
def test_vector_cos_matches_libm(t : T?) {
    g_offset = 0.0
    let xs <- build_sincos_samples()
    report_abs(t, "cos(float4)", sincos_error(xs, true), bound(SINCOS_ABS), length(xs) * 4)
}

[test]
def test_vector_sin_narrow_widths(t : T?) {
    g_offset = 0.0
    let xs <- build_sincos_samples()
    report_abs(t, "sin(float2)", sin_error_narrow(xs, 2), bound(SINCOS_ABS), length(xs) * 2)
    report_abs(t, "sin(float3)", sin_error_narrow(xs, 3), bound(SINCOS_ABS), length(xs) * 3)
}

// ===== tan =====

// relative error is the meaningful measure for tan: near a pole the value is huge, so the absolute
// error scales with it. The relative error is NOT flat either - the reduced argument carries a
// roughly fixed absolute error, and near a pole tan ~ 1/xr turns that into a relative error that
// grows as the pole is approached. Three bands measure that growth; the bounds are what each band
// actually delivers, not a target.
def private tan_away_from_poles : array<float4> {
    var xs : array<float4>
    xs |> reserve(601 * 80)
    for (k in range(-300, 301)) {
        let c = float(k) * PI                       // tan is 0 here, poles sit at +-pi/2 away
        for (j in range(80)) {
            let d = (float(j) - 39.5) * 0.035 + g_offset    // |d| <= 1.4, so >= 0.17 from a pole
            xs |> push(float4(c + d, c - d, c + d * 0.5, c - d * 0.5))
        }
    }
    return <- xs
}

[test]
def test_vector_tan(t : T?) {
    g_offset = 0.0
    let ys <- ramp(-4.0, 4.0, 50000)
    let clean <- tan_away_from_poles()
    let cys <- ramp(-4.0, 4.0, length(clean))
    report_rel(t, "tan(float4) >0.17 from a pole", op_error(clean, cys, OP_TAN), bound(5.0e-7), length(clean) * 4)
    let xs <- ramp(-1000.0, 1000.0, 50000)
    report_rel(t, "tan(float4) unfiltered |x|<1000", op_error(xs, ys, OP_TAN), bound(3.0e-6), length(xs) * 4)
    // vecmath's v_tan is mirrored instruction for instruction, so the two rails agree exactly
    report_divergence(t, "tan(float4) >0.17 from a pole", clean, cys, OP_TAN, 0.0)
    report_divergence(t, "tan(float4) unfiltered |x|<1000", xs, ys, OP_TAN, 0.0)
}

[test]
def test_vector_tan_near_poles(t : T?) {
    g_offset = 0.0
    // straddle 400 poles at +-1e-5 .. +-1e-3, where |tan| runs from 1e3 to 1e5
    var xs : array<float4>
    xs |> reserve(400 * 40)
    for (k in range(-200, 200)) {
        let p = (float(k) + 0.5) * PI
        for (j in range(40)) {
            let d = 1.0e-5 * pow(1.12, float(j)) + g_offset
            xs |> push(float4(p + d, p - d, p + d * 3.0, p - d * 3.0))
        }
    }
    let ys <- ramp(-4.0, 4.0, length(xs))
    report_rel(t, "tan(float4) within 1e-3 of a pole", op_error(xs, ys, OP_TAN), bound(1.0e-5), length(xs) * 4)
    report_divergence(t, "tan(float4) within 1e-3 of a pole", xs, ys, OP_TAN, 0.0)
}

// ===== exp2 / log2 / log =====

[test]
def test_vector_exp2(t : T?) {
    g_offset = 0.0
    let xs <- ramp(-125.0, 128.0, 50000)
    let ys <- ramp(-4.0, 4.0, 50000)
    report_rel(t, "exp2(float4)", op_error(xs, ys, OP_EXP2), bound(4.0e-7), length(xs) * 4)
    report_divergence(t, "exp2(float4)", xs, ys, OP_EXP2, 0.0)
}

// v_log2_est_p5's error is a function of the MANTISSA, so it repeats identically in every binade
// and a linear ramp over a wide range samples it coarsely. Sweep the mantissa instead: 2^k * m for
// m across [1,2), which is where the polynomial's worst case actually lives.
def private log_mantissa_sweep : array<float4> {
    var xs : array<float4>
    xs |> reserve(40 * 2000)
    for (k in range(-20, 20)) {
        let s = pow(2.0, float(k))
        for (j in range(2000)) {
            let m = 1.0 + float(j) * 5.0e-4 + g_offset
            xs |> push(float4(m * s, m * s * 1.0001, (2.0 - float(j) * 5.0e-4) * s, m * s * 0.5))
        }
    }
    return <- xs
}

// log2/log mirror v_log2_est_p5 - an ESTIMATE, and a loose one: 3.7e-5 relative at its worst,
// roughly 300x a correctly rounded float log2, with the worst case sitting just below x == 1
// where the p*(m-1) + e combine cancels. The interpreter and AOT have always returned exactly
// this; the bounds record what it delivers rather than endorse it.
[test]
def test_vector_log2_and_log(t : T?) {
    g_offset = 0.0
    let xs <- log_mantissa_sweep()
    let ys <- ramp(-4.0, 4.0, length(xs))
    // absolute first, because it is the bound that holds across the zero crossing at x == 1
    report_abs(t, "log2(float4)", op_error(xs, ys, OP_LOG2), bound(3.0e-5), length(xs) * 4)
    report_abs(t, "log(float4)", op_error(xs, ys, OP_LOG), bound(3.0e-5), length(xs) * 4)
    report_rel(t, "log2(float4) |log2| > 0.1", op_error(xs, ys, OP_LOG2, 0.1), bound(8.0e-5), length(xs) * 4)
    report_rel(t, "log(float4) |log| > 0.1", op_error(xs, ys, OP_LOG, 0.1), bound(8.0e-5), length(xs) * 4)
    report_divergence(t, "log2(float4)", xs, ys, OP_LOG2, 0.0)
    report_divergence(t, "log(float4)", xs, ys, OP_LOG, 0.0)
}

// v_log2_est_p5 reads the exponent through a mask, so it never yields -inf or NaN: log2(0) is
// -127, log2(-x) == log2(|x|). Those are the interpreter's answers and the JIT must repeat them
// exactly, however unlike libm they look.
[test]
def test_vector_log_special_values(t : T?) {
    g_offset = 0.0
    let xs <- [float4(0.0, -0.0, -4.0, -1.0),           // zero, negative zero, negatives
        float4(1.0, 2.0, 1.0e-40, -1.0e-40),            // exact powers of two, subnormals
        float4(1.0e-38, 3.4e38, 1.0e-45, 8.0)]          // the ends of the normal range
    let ys <- ramp(-4.0, 4.0, length(xs))
    let got <- apply_op(xs, ys, OP_LOG2)
    t |> equal(got[0][0], -127.0, "log2(0) is the exponent floor, not -inf, on every tier and target")
    t |> equal(got[0][2], 2.0, "log2(-4) reads the magnitude")
    t |> equal(got[1][0], 0.0, "log2(1) is exactly 0")
    t |> equal(got[1][1], 1.0, "log2(2) is exactly 1")
    report_divergence(t, "log2(float4) special values", xs, ys, OP_LOG2, 0.0)
    report_divergence(t, "log(float4) special values", xs, ys, OP_LOG, 0.0)
}

// the emitter is width-generic, so float2 and float3 are emissions of their own; each pair below
// carries the SAME body, only [no_jit] separates the two
def private apply_log_narrow(xs, ys : array<float4>; which, dim : int) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v, y in xs, ys) {
        if (dim == 2) {
            let a = float2(v.x, v.y)
            let b = which == OP_LOG2 ? log2(a) : which == OP_LOG ? log(a) : pow(a, float2(y.x, y.y))
            r |> push(float4(b.x, b.y, 0.0, 0.0))
        } else {
            let a = float3(v.x, v.y, v.z)
            let b = which == OP_LOG2 ? log2(a) : which == OP_LOG ? log(a) : pow(a, float3(y.x, y.y, y.z))
            r |> push(float4(b.x, b.y, b.z, 0.0))
        }
    }
    return <- r
}

[no_jit, never_inline]
def private apply_log_narrow_interp(xs, ys : array<float4>; which, dim : int) : array<float4> {
    var r : array<float4>
    r |> reserve(length(xs))
    for (v, y in xs, ys) {
        if (dim == 2) {
            let a = float2(v.x, v.y)
            let b = which == OP_LOG2 ? log2(a) : which == OP_LOG ? log(a) : pow(a, float2(y.x, y.y))
            r |> push(float4(b.x, b.y, 0.0, 0.0))
        } else {
            let a = float3(v.x, v.y, v.z)
            let b = which == OP_LOG2 ? log2(a) : which == OP_LOG ? log(a) : pow(a, float3(y.x, y.y, y.z))
            r |> push(float4(b.x, b.y, b.z, 0.0))
        }
    }
    return <- r
}

def private log_divergence_narrow(xs, ys : array<float4>; which, dim : int) : ErrReport {
    var r : ErrReport
    let got <- apply_log_narrow(xs, ys, which, dim)
    let ref <- apply_log_narrow_interp(xs, ys, which, dim)
    for (g, e, x in got, ref, xs) {
        for (l in range(dim)) {
            track(r, g[l], e[l], x[l], 1.0e-20)
        }
    }
    return r
}

[test]
def test_vector_log_narrow_widths(t : T?) {
    g_offset = 0.0
    let xs <- log_mantissa_sweep()
    let ys <- ramp(-4.0, 4.0, length(xs))
    for (which, name in [OP_LOG2, OP_LOG, OP_POW], ["log2", "log", "pow"]) {
        for (dim in [2, 3]) {
            let what = "{name}(float{dim})"
            if (!rail_applicable(op_on_poly_rail(which))) {
                t |> success(true, "rail consistency not applicable: {not_applicable_reason(what)}")
                continue
            }
            report_div(t, what, log_divergence_narrow(xs, ys, which, dim), 0.0, length(xs) * dim)
        }
    }
}

// ===== pow =====

[test]
def test_vector_pow(t : T?) {
    g_offset = 0.0
    let xs <- log_mantissa_sweep()
    let ys <- ramp(-4.0, 4.0, length(xs))
    // pow is exp2(log2_est(x) * y): it carries the log2 estimate's error scaled by |y|
    report_rel(t, "pow(float4)", op_error(xs, ys, OP_POW), bound(6.0e-5), length(xs) * 4)
    report_divergence(t, "pow(float4)", xs, ys, OP_POW, 0.0)
}

[test]
def test_vector_pow_zero_exponent(t : T?) {
    g_offset = 0.0
    var xs : array<float4>
    xs |> push(float4(0.0, 1.0, -3.0, 1.0e30))
    var ys : array<float4>
    ys |> push(float4(0.0, 0.0, 0.0, 0.0))
    let got <- apply_op(xs, ys, OP_POW)
    for (l in range(4)) {
        t |> equal(got[0][l], 1.0, "pow(x, 0) is 1 - vecmath's y == 0 guard, lane {l}")
    }
    report_divergence(t, "pow(float4) zero exponent", xs, ys, OP_POW, 0.0)
}

[test]
def test_vector_pow_negative_base(t : T?) {
    g_offset = 0.0
    var xs : array<float4>
    xs |> push(float4(-2.0, -8.0, -0.5, -3.0))
    var ys : array<float4>
    ys |> push(float4(3.0, 2.0, 2.0, -3.0))
    let got <- apply_op(xs, ys, OP_POW)
    let ref <- apply_op_interp(xs, ys, OP_POW)
    for (l in range(4)) {
        t |> equal(got[0][l], ref[0][l], "pow(negative base) agrees lane for lane, lane {l}")
    }
    t |> equal(ref[0][0], -8.0, "pow(-2, 3) is -8 - the odd exponent flips the sign")
    t |> equal(ref[0][1], 64.0, "an even exponent leaves the sign alone")
    t |> equal(ref[0][2], 0.25, "pow(-0.5, 2) is 0.25")
    t |> success(ref[0][3] < 0.0, "a negative odd exponent flips too, got {ref[0][3]}")
    report_divergence(t, "pow(float4) negative base", xs, ys, OP_POW, 0.0)
}

// ===== NaN lanes =====

// g_offset is 0.0 at every cell, so this is a runtime NaN the AST cannot fold away
def private nan_samples : array<float4> {
    let q = 0.0 / g_offset
    return <- [ float4(q, 1.0, -q, -2.5),
        float4(0.5, q, 100.0, -q)]
}

def private report_nan_lanes(t : T?; what : string; got, ref, xs : array<float4>) {
    for (g, e, x in got, ref, xs) {
        for (l in range(4)) {
            t |> equal(is_nan(g[l]), is_nan(x[l]), "{what}: the JIT carries NaN at x = {x[l]}, got {g[l]}")
            t |> equal(is_nan(e[l]), is_nan(x[l]), "{what}: the interpreter carries NaN at x = {x[l]}, got {e[l]}")
        }
    }
}

def private nan_lane_cell(t : T?; what : string; xs, ys : array<float4>; which : int) {
    let got <- apply_op(xs, ys, which)
    let ref <- apply_op_interp(xs, ys, which)
    report_nan_lanes(t, what, got, ref, xs)
}

def private nan_lane_cell_sincos(t : T?; what : string; xs : array<float4>; wantCos : bool) {
    let got <- apply_sincos(xs, 4, wantCos)
    let ref <- apply_sincos_interp(xs, 4, wantCos)
    report_nan_lanes(t, what, got, ref, xs)
}

// A clamp or a float-to-int conversion written with ORDERED compares silently replaces a NaN lane -
// tanh's [-9,9] clamp answered 1.0 for NaN that way - and the replacement is invisible to every
// accuracy bound above, because a bound only looks at lanes that produced a number. NaN in, NaN out
// is asserted on both rails instead, and it holds on every target: off the polynomial rail the JIT
// takes libm, which propagates NaN too. exp is the one member of the rail that cannot be here -
// the interpreter answers inf for exp(NaN) while the JIT answers NaN (ARCHITECTURE.md sec.8.1).
[test]
def test_vector_nan_lanes(t : T?) {
    g_offset = 0.0
    let xs <- nan_samples()
    let ys <- ramp(-4.0, 4.0, length(xs))
    nan_lane_cell(t, "tanh(float4)", xs, ys, OP_TANH)
    nan_lane_cell(t, "sinh(float4)", xs, ys, OP_SINH)
    nan_lane_cell(t, "cosh(float4)", xs, ys, OP_COSH)
    nan_lane_cell(t, "tan(float4)", xs, ys, OP_TAN)
    nan_lane_cell_sincos(t, "sin(float4)", xs, false)
    nan_lane_cell_sincos(t, "cos(float4)", xs, true)
}

// ===== tanh / sinh / cosh =====

// These three have NO interpreter twin: vecmath carries no vector hyperbolics, so the interpreter
// and AOT call libm once per lane while the JIT runs the exp polynomial. The divergence bound
// below is therefore a real behaviour difference between the rails, not rounding noise - it is
// asserted so the size of that difference stays pinned.
[test]
def test_vector_tanh(t : T?) {
    g_offset = 0.0
    let xs <- ramp(-20.0, 20.0, 50000)
    let ys <- ramp(-4.0, 4.0, 50000)
    report_rel(t, "tanh(float4)", op_error(xs, ys, OP_TANH), bound(8.0e-7), length(xs) * 4)
    report_divergence(t, "tanh(float4)", xs, ys, OP_TANH, bound(8.0e-7))
    let near <- ramp(-0.5, 0.5, 50000)
    report_rel(t, "tanh(float4) near zero", op_error(near, ys, OP_TANH), bound(8.0e-7), length(near) * 4)
}

[test]
def test_vector_sinh_cosh(t : T?) {
    g_offset = 0.0
    let xs <- ramp(-20.0, 20.0, 50000)
    let ys <- ramp(-4.0, 4.0, 50000)
    report_rel(t, "sinh(float4)", op_error(xs, ys, OP_SINH), bound(8.0e-7), length(xs) * 4)
    report_rel(t, "cosh(float4)", op_error(xs, ys, OP_COSH), bound(8.0e-7), length(xs) * 4)
    report_divergence(t, "sinh(float4)", xs, ys, OP_SINH, bound(8.0e-7))
    report_divergence(t, "cosh(float4)", xs, ys, OP_COSH, bound(8.0e-7))
    let near <- ramp(-0.5, 0.5, 50000)
    report_rel(t, "sinh(float4) near zero", op_error(near, ys, OP_SINH), bound(8.0e-7), length(near) * 4)
}

// The clamp inside the tanh emitter must saturate rather than let exp(2x) overflow to inf and turn
// the quotient into NaN. The two rails saturate to different floats and that is expected: libm
// returns the largest float below 1 at x = 9, the emitter's (t-1)/(t+1) rounds to exactly 1. Being
// within one ulp of +-1 is asserted on every rail; exactness only where the emitter runs.
[test]
def test_vector_tanh_saturates(t : T?) {
    g_offset = 0.0
    let xs <- [float4(9.0, -9.0, 20.0, -20.0),          // the clamp edge and just past it
        float4(88.0, -88.0, 1.0e30, -1.0e30)]           // past where exp(2x) would overflow
    let ys <- ramp(-4.0, 4.0, length(xs))
    let got <- apply_op(xs, ys, OP_TANH)
    let emitted = rail_applicable(on_aarch64())
    for (g, x in got, xs) {
        for (l in range(4)) {
            let a = abs(g[l])
            t |> success(a <= 1.0 && a >= 1.0 - 1.2e-7, "tanh saturates to within a ulp of +-1 at x = {x[l]}, got {g[l]}")
            t |> success(!emitted || a == 1.0, "the emitted tanh saturates to exactly +-1 at x = {x[l]}, got {g[l]}")
            t |> equal(g[l] > 0.0, x[l] > 0.0, "tanh keeps the sign at x = {x[l]}")
        }
    }
}
