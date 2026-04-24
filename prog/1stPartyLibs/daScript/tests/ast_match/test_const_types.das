options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Target functions with various constant types ─────────────────────────

[export, marker(no_coverage)]
def target_int64(x : int64) : int64 {
    return x + 100l
}

[export, marker(no_coverage)]
def target_uint(x : uint) : uint {
    return x + 0xFFu
}

[export, marker(no_coverage)]
def target_double(x : double) : double {
    return x + 3.14lf
}

[export, marker(no_coverage)]
def target_uint64(x : uint64) : uint64 {
    return x + 1000ul
}

[export, marker(no_coverage)]
def target_null_check(x : int?) : bool {
    return x == null
}

[export, marker(no_coverage)]
def target_range() : range {
    return range(1, 10)
}

[export, marker(no_coverage)]
def target_enum(x : Type) : Type {
    return x
}

[export, marker(no_coverage)]
def target_float2() : float2 {
    return float2(1.0, 2.0)
}

[export, marker(no_coverage)]
def target_int2() : int2 {
    return int2(3, 4)
}

// ── Tests ────────────────────────────────────────────────────────────────

[test]
def test_int64_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_int64)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : int64) {
        return x + 100l
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_int64_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_int64)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : int64) {
        return x + 200l
    }
    t |> success(!r.matched, "wrong int64 value should not match")
}

[test]
def test_uint_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_uint)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : uint) {
        return x + 0xFFu
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_uint_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_uint)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : uint) {
        return x + 0x00u
    }
    t |> success(!r.matched, "wrong uint value should not match")
}

[test]
def test_double_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_double)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : double) {
        return x + 3.14lf
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_double_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_double)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : double) {
        return x + 2.71lf
    }
    t |> success(!r.matched, "wrong double value should not match")
}

[test]
def test_uint64_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_uint64)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : uint64) {
        return x + 1000ul
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_null_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_null_check)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : int?) {
        return x == null
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_range_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_range)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return range(1, 10)
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_range_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_range)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return range(1, 20)
    }
    t |> success(!r.matched, "wrong range should not match")
}

[test]
def test_float2_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_float2)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return float2(1.0, 2.0)
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_float2_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_float2)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return float2(1.0, 9.0)
    }
    t |> success(!r.matched, "wrong float2 should not match")
}

[test]
def test_int2_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_int2)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return int2(3, 4)
    }
    if (!r.matched && r.expr != null) {
        t |> success(false, "error={int(r.error)} rtti={qm_rtti(r.expr)}")
    } elif (!r.matched) {
        t |> success(false, "error={int(r.error)} expr=null")
    } else {
        t |> success(true)
    }
}

[test]
def test_int2_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_int2)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $ {
        return int2(3, 99)
    }
    t |> success(!r.matched, "wrong int2 should not match")
}

[test]
def test_identity_enum(t : T?) {
    // enum identity — just body match, no constant folding
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_enum)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : Type) {
        return x
    }
    t |> success(r.matched, "identity should match")
}

// ── Tag capture in constant constructors ─────────────────────────────────

[export, marker(no_coverage)]
def target_float4() : float4 {
    return float4(1.0, 2.0, 3.0, 4.0)
}

[test]
def test_float2_capture_v(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_float2)
    if (get_ptr(func) == null) {
        return
    }
    var x_val : float
    let r = qmatch_function(func) $ {
        return float2($v(x_val), 2.0)
    }
    t |> success(r.matched, "should match with $v capture")
    t |> success(x_val == 1.0, "captured x should be 1.0, got {x_val}")
}

[test]
def test_float2_capture_v_wrong_fixed(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_float2)
    if (get_ptr(func) == null) {
        return
    }
    var x_val : float
    let r = qmatch_function(func) $ {
        return float2($v(x_val), 9.0)
    }
    t |> success(!r.matched, "fixed component mismatch should fail")
}

[test]
def test_int2_capture_v(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_int2)
    if (get_ptr(func) == null) {
        return
    }
    var y_val : int
    let r = qmatch_function(func) $ {
        return int2(3, $v(y_val))
    }
    t |> success(r.matched, "should match with $v on second component")
    t |> success(y_val == 4, "captured y should be 4, got {y_val}")
}

[test]
def test_float4_capture_v(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_float4)
    if (get_ptr(func) == null) {
        return
    }
    var x_val, w_val : float
    let r = qmatch_function(func) $ {
        return float4($v(x_val), 2.0, 3.0, $v(w_val))
    }
    t |> success(r.matched, "should match with $v on first and fourth")
    t |> success(x_val == 1.0, "captured x should be 1.0, got {x_val}")
    t |> success(w_val == 4.0, "captured w should be 4.0, got {w_val}")
}

[test]
def test_range_capture_v(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_range)
    if (get_ptr(func) == null) {
        return
    }
    var lo, hi : int
    let r = qmatch_function(func) $ {
        return range($v(lo), $v(hi))
    }
    t |> success(r.matched, "should match with $v capture on both range components")
    t |> success(lo == 1, "lo should be 1, got {lo}")
    t |> success(hi == 10, "hi should be 10, got {hi}")
}
