options gen2
require dastest/testing_boost

require jit

require daslib/fuzzer
require daslib/faker
require daslib/math
require daslib/constant_expression

def jit_roundi(x) {
    return roundi(x)
}

def test_roundi(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_roundi))
    jit_roundi(a)
}
    // precision mismatch???
    // t |> numericEqual ( roundi(a), jit_roundi(a), "roundi(a)" )

[test]
def test_intrin_roundi(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_roundi")
    }
}

def jit_trunci(x) {
    return trunci(x)
}

def test_trunci(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_trunci))
    t |> strictEqual(trunci(a), jit_trunci(a), "trunci(a)")
}

[test]
def test_intrin_trunci(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_trunci")
    }
}

def jit_floor(x) {
    return floor(x)
}

def test_floor(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_floor))
    t |> strictEqual(floor(a), jit_floor(a), "floor(a)")
}

[test]
def test_intrin_floor(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_floor")
    }
}

[sideeffects]
def jit_ceil(x) {
    return ceil(x)
}

def test_ceil(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_ceil))
    t |> numericEqual(ceil(a), jit_ceil(a), "ceil(a)")
}

[test]
def test_intrin_ceil(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_ceil")
    }
}

def jit_sqrt(x) {
    return sqrt(x)
}

def test_sqrt(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_sqrt))
    t |> strictEqual(sqrt(a), jit_sqrt(a), "sqrt(a)")
}

[test]
def test_intrin_sqrt(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op1(t, fake, "test_sqrt")
    }
}

[sideeffects]
def jit_rcp(x) {
    return rcp(x)
}

def test_rcp(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_rcp))
    jit_rcp(a)
}
    // note: we can't really compare, because precision is different
    //t |> numericEqual ( rcp(a), jit_rcp(a), "rcp(a)" )

[test]
def test_intrin_rcp(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_rcp")
    }
}

[sideeffects]
def jit_rcp_est(x) {
    return rcp_est(x)
}

def test_rcp_est(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_rcp_est))
    jit_rcp_est(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( rsqrt(a), jit_rsqrt(a), "rsqrt(a)" )

[test]
def test_intrin_rcp_est(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_rcp_est")
    }
}

[sideeffects]
def jit_rsqrt(x) {
    return rsqrt(x)
}

def test_rsqrt(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_rsqrt))
    jit_rsqrt(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( rsqrt(a), jit_rsqrt(a), "rsqrt(a)" )

[test]
def test_intrin_rsqrt(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_rsqrt")
    }
}

[sideeffects]
def jit_rsqrt_est(x) {
    return rsqrt_est(x)
}

def test_rsqrt_est(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_rsqrt_est))
    jit_rsqrt_est(a)
}
    // note: we can't really compare, because precision is different
    //t |> numericEqual ( rsqrt_est(a), jit_rsqrt_est(a), "rsqrt_est(a)" )

[test]
def test_intrin_rsqrt_est(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_rsqrt_est")
    }
}

[sideeffects]
def jit_exp(x) {
    return exp(x)
}

def test_exp(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_exp))
    jit_exp(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( exp(a), jit_exp(a), "exp(a)" )

[test]
def test_intrin_exp(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op1(t, fake, "test_exp")
    }
}

[sideeffects]
def jit_exp2(x) {
    return exp2(x)
}

def test_exp2(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_exp2))
    jit_exp2(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( exp2(a), jit_exp2(a), "exp2(a)" )

[test]
def test_intrin_exp2(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op1(t, fake, "test_exp2")
    }
}

[sideeffects]
def jit_log(x) {
    return log(x)
}

def test_log(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_log))
    jit_log(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( log(a), jit_log(a), "log(a)" )

[test]
def test_intrin_log(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op1(t, fake, "test_log")
    }
}

[sideeffects]
def jit_log2(x) {
    return log2(x)
}

def test_log2(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_log2))
    jit_log2(a)
}
    // note: we can't really compare, because precision is different
    // t |> numericEqual ( log2(a), jit_log2(a), "log2(a)" )

[test]
def test_intrin_log2(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op1(t, fake, "test_log2")
    }
}

def jit_popcount(x) {
    return popcnt(x)
}

def test_popcount(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_popcount))
    t |> equal(popcnt(a), jit_popcount(a), "popcnt(a)")
}

[test]
def test_popcount(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_all_unsigned_ints_op1(t, fake, "test_popcount")
    }
}

def jit_ctz(x) {
    return ctz(x)
}

def test_ctz(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_ctz))
    t |> equal(ctz(a), jit_ctz(a), "ctz(a)")
}

[test]
def test_ctz(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_all_unsigned_ints_op1(t, fake, "test_ctz")
    }
}

def jit_clz(x) {
    return clz(x)
}

def test_clz(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_clz))
    t |> equal(clz(a), jit_clz(a), "clz(a)")
}

[test]
def test_clz(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_all_unsigned_ints_op1(t, fake, "test_clz")
    }
}

def jit_min(a, b) {
    return min(a, b)
}

def test_min(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a, b : TT) : void > jit_min))
    t |> strictEqual(min(a, b), jit_min(a, b), "min(a,b)")
}

[test]
def test_min(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_vector_op2(t, fake, "test_min")
    }
}

def jit_max(a, b) {
    return max(a, b)
}

def test_max(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a, b : TT) : void > jit_max))
    t |> strictEqual(max(a, b), jit_max(a, b), "max(a,b)")
}

[test]
def test_max(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_vector_op2(t, fake, "test_max")
    }
}

def jit_abs(a) {
    return abs(a)
}

def test_abs(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : TT > jit_abs))
    t |> strictEqual(abs(a), jit_abs(a), "abs(a)")
}

[test]
def test_abs(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_vector_signed_op1(t, fake, "test_abs")
    }
}

def jit_saturate(a) {
    return saturate(a)
}

def test_saturate(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : TT > jit_saturate))
    t |> strictEqual(saturate(a), jit_saturate(a), "saturate(a)")
}

[test]
def test_saturate(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_or_float_vec_op1(t, fake, "test_saturate")
    }
}

[sideeffects]
def jit_pow(x, y) {
    return pow(x, y)
}

def test_pow(t : T?; a, b : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a, b : TT) : void > jit_pow))
    jit_pow(a, b)
}
    // note: mismatching precision
    // t |> numericEqual ( pow(a,b), jit_pow(a,b), "pow(a,b)" )

[test]
def test_intrin_pow(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op2(t, fake, "test_pow")
    }
}

[sideeffects]
def jit_mad(a, b, c) {
    return mad(a, b, c)
}

def test_mad(t : T?; a : auto(TT); b : auto(QQ); c : TT) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : QQ; c : TT) : TT > jit_mad))
    jit_mad(a, b, c)
}
    // note: mismatching precision
    // t |> numericEqual ( mad(a,b,c), jit_mad(a,b,c), "mad(a,b,c)" )

[test]
def test_mad(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_op3(t, fake, "test_mad")
    }
    fuzz() {
        fuzz_vec_op3(t, fake, "test_mad")
    }
    fuzz() {
        fuzz_vec_mad_op3(t, fake, "test_mad")
    }
}

[sideeffects]
def jit_lerp(a, b, c) {
    return lerp(a, b, c)
}

def test_lerp(t : T?; a : auto(TT); b : auto(QQ); c : TT) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : QQ; c : TT) : TT > jit_mad))
    jit_lerp(a, b, c)
}
    // note: mismatching precision
    // t |> numericEqual ( lerp(a,b,c), jit_lerp(a,b,c), "lerp({a},{b},{c})" )

[sideeffects]
def jit_normalize_zero() {
    return normalize(float2(0.0, 0.0))
}

[sideeffects]
def jit_normalize_nonzero() {
    return normalize(float2(2.0, 2.0))
}

[test]
def test_normalize(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@ < () : float2 > jit_normalize_zero))
    t |> success(!jit_enabled() || is_jit_function(@@ < () : float2 > jit_normalize_nonzero))
    t |> equal(jit_normalize_nonzero(), float2(1.0 / sqrt(2.0), 1.0 / sqrt(2.0)))
    t |> equal(jit_normalize_zero(), float2(0.0, 0.0))
}

[sideeffects]
def float3_lerp(from, to : float3; t : float) : float3 {
    return lerp(from, to, t)
}

[test]
def test_lerp(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : float3; b : float3; c : float) : float3 > float3_lerp))
    var fake <- Faker()
    fuzz() {
        fuzz_float_double_or_float_vec_op3(t, fake, "test_lerp")
    }
}


[sideeffects]
def jit_clamp(a, b, c) {
    return clamp(a, b, c)
}

def test_clamp(t : T?; a : auto(TT); _b : auto(QQ); c : TT) {
    let b = a + _b
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT; b : QQ; c : TT) : TT > jit_mad))
    jit_clamp(a, b, c)
}
    // note: mismatching precision
    // t |> numericEqual ( clamp(a,b,c), jit_clamp(a,b,c), "clamp({a},{b},{c})" )

[test]
def test_clamp(t : T?) {
    t |> equal(3, jit_clamp(0, 3, 4))
    t |> equal(4, jit_clamp(10, 3, 4))
    t |> equal(3, jit_clamp(2, 3, 4))
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_op3(t, fake, "test_clamp")
    }
    fuzz() {
        fuzz_vec_op3(t, fake, "test_clamp")
    }
}

[sideeffects]
def get_m_3x3 {
    return float3x3()
}

[sideeffects]
def get_m_3x4 {
    return float3x4()
}

[sideeffects]
def get_m_4x4 {
    return float4x4()
}

[test]
def test_mat_ctor(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@get_m_3x3))
    t |> success(!jit_enabled() || is_jit_function(@@get_m_3x4))
    t |> success(!jit_enabled() || is_jit_function(@@get_m_4x4))

    var m1 = get_m_3x3()
    for (x in 0..2) {
        for (y in 0..2) {
            t |> equal(0., m1[x][y])
        }
    }
    var m2 = get_m_3x4()
    for (x in 0..2) {
        for (y in 0..3) {
            t |> equal(0., m2[x][y])
        }
    }
    var m3 = get_m_4x4()
    for (x in 0..3) {
        for (y in 0..3) {
            t |> equal(0., m3[x][y])
        }
    }
}

[sideeffects]
def jit_sign(x) {
    return sign(x)
}

[test]
def test_sign(t : T?) {
    // int
    t |> equal(1, jit_sign(1))
    t |> equal(1, jit_sign(2))
    t |> equal(1, jit_sign(100))
    t |> equal(-1, jit_sign(-1))
    t |> equal(-1, jit_sign(-2))
    t |> equal(-1, jit_sign(-100))
    t |> equal(0, jit_sign(0))
    // uint
    t |> equal(1u, jit_sign(1u))
    t |> equal(1u, jit_sign(2u))
    t |> equal(1u, jit_sign(100u))
    t |> equal(0u, jit_sign(0u))
    // float
    t |> equal(1., jit_sign(1.))
    t |> equal(1., jit_sign(2.))
    t |> equal(1., jit_sign(100.))
    t |> equal(-1., jit_sign(-1.))
    t |> equal(-1., jit_sign(-2.))
    t |> equal(-1., jit_sign(-100.))
    t |> equal(0., jit_sign(0.))
    // int3 vec
    t |> equal(int3(1, 1, 1), jit_sign(int3(1, 1, 1)))
    t |> equal(int3(1, 1, 1), jit_sign(int3(2, 2, 2)))
    t |> equal(int3(1, 1, 1), jit_sign(int3(100, 100, 100)))
    t |> equal(int3(-1, -1, -1), jit_sign(int3(-1, -1, -1)))
    t |> equal(int3(-1, -1, -1), jit_sign(int3(-2, -2, -2)))
    t |> equal(int3(-1, -1, -1), jit_sign(int3(-100, -100, -100)))
    t |> equal(int3(0, 0, 0), jit_sign(int3(0, 0, 0)))
    // uint2 vec
    t |> equal(uint2(1u, 1u), jit_sign(uint2(1u, 1u)))
    t |> equal(uint2(1u, 1u), jit_sign(uint2(2u, 2u)))
    t |> equal(uint2(1u, 1u), jit_sign(uint2(100u, 100u)))
    t |> equal(uint2(0u, 0u), jit_sign(uint2(0u, 0u)))
    // float4 vec
    t |> equal(float4(1., 1., 1., 1.), jit_sign(float4(1., 1., 1., 1.)))
    t |> equal(float4(1., 1., 1., 1.), jit_sign(float4(2., 2., 2., 2.)))
    t |> equal(float4(1., 1., 1., 1.), jit_sign(float4(100., 100., 100., 100.)))
    t |> equal(float4(-1., -1., -1., -1.), jit_sign(float4(-1., -1., -1., -1.)))
    t |> equal(float4(-1., -1., -1., -1.), jit_sign(float4(-2., -2., -2., -2.)))
    t |> equal(float4(-1., -1., -1., -1.), jit_sign(float4(-100., -100., -100., -100.)))
    t |> equal(float4(0., 0., 0., 0.), jit_sign(float4(0., 0., 0., 0.)))
}






