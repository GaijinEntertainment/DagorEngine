options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Generic functions ────────────────────────────────────────────────────

[sideeffects]
def generic_add(a, b : auto) : auto {
    return a + b
}

[sideeffects]
def generic_identity(x : auto) : auto {
    return x
}

[sideeffects]
def generic_branch(a : auto; threshold : int) : auto {
    if (a > threshold) {
        return a
    } else {
        return threshold
    }
}

// ── Target functions that call generics ──────────────────────────────────

[export, marker(no_coverage)]
def target_add_int(a, b : int) : int {
    return generic_add(a, b)
}

[export, marker(no_coverage)]
def target_add_float(a, b : float) : float {
    return generic_add(a, b)
}

[export, marker(no_coverage)]
def target_identity(x : int) : int {
    return generic_identity(x)
}

[export, marker(no_coverage)]
def target_branch(a : int; threshold : int) : int {
    return generic_branch(a, threshold)
}

// ── Tests ────────────────────────────────────────────────────────────────

[test]
def test_add_int_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    t |> success(get_ptr(func) != null, "should find target_add_int")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) {
        return generic_add(a, b)
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
def test_add_float_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_float)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : float) {
        return generic_add(a, b)
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
def test_identity_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_identity)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x : int) {
        return generic_identity(x)
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
def test_wrong_generic_name(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) {
        return generic_identity(a)
    }
    t |> success(!r.matched, "wrong generic name should not match")
}

[test]
def test_wrong_pattern(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) {
        return a + b
    }
    // target_add_int calls generic_add(a,b), not a+b directly
    t |> success(!r.matched, "inlined body should not match call pattern")
}

[test]
def test_capture_names(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    if (get_ptr(func) == null) {
        return
    }
    var call_name, lhs_name, rhs_name : string
    let r = qmatch_function(func) $(a, b : int) {
        return $c(call_name)($i(lhs_name), $i(rhs_name))
    }
    if (r.matched) {
        t |> success(true)
        t |> equal(call_name, "generic_add")
        t |> equal(lhs_name, "a")
        t |> equal(rhs_name, "b")
    } else {
        t |> success(false, "should match call with captures")
    }
}

[test]
def test_branch_match(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_branch)
    t |> success(get_ptr(func) != null, "should find target_branch")
    if (get_ptr(func) == null) {
        return
    }
    // After optimization, may fold to ternary or stay as call
    var inscope cond : ExpressionPtr
    let r_call = qmatch_function(func) $(a : int; threshold : int) {
        return generic_branch(a, threshold)
    }
    let r_ternary = qmatch_function(func) $(a : int; threshold : int) {
        return $e(cond) ? $e(cond) : $e(cond)
    }
    t |> success(r_call.matched || r_ternary.matched, "should match call or ternary")
}

[test]
def test_add_arg_types(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    if (get_ptr(func) == null) {
        return
    }
    // Verify arg types are int (not auto)
    let r = qmatch_function(func) $(a : int; b : int) : int {
        return generic_add(a, b)
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
def test_float_arg_types_dont_match_int(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add_int)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a : float; b : float) : float {
        return generic_add(a, b)
    }
    t |> success(!r.matched, "int function should not match float arg types")
}
