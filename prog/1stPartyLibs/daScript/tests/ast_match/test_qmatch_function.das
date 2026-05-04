options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Target functions (compiled with optimizations) ──────────────────────

[export, marker(no_coverage)]
def target_simple(a, b : int) : int {
    return a + b
}

[export, marker(no_coverage)]
def target_if_return(a, b, c : int) : int {
    if (a < 5) {
        return b
    } else {
        return c
    }
}

// ── Tests ───────────────────────────────────────────────────────────────

[test]
def test_function_simple(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_simple)
    t |> success(get_ptr(func) != null, "should find target_simple")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) {
        return a + b
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
def test_function_if_return(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_if_return)
    t |> success(get_ptr(func) != null, "should find target_if_return")
    if (get_ptr(func) == null) {
        return
    }
    // After optimization, if/else return may fold into ternary
    var inscope cond : ExpressionPtr
    let r_ternary = qmatch_function(func) $(a, b, c : int) {
        return $e(cond) ? $e(cond) : $e(cond)
    }
    let r_if = qmatch_function(func) $(a, b, c : int) {
        if (a < 5) {
            return b
        } else {
            return c
        }
    }
    t |> success(r_ternary.matched || r_if.matched, "should match ternary or if-else")
}

[test]
def test_function_wrong_pattern(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_simple)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) {
        return a - b
    }
    t |> success(!r.matched, "wrong pattern should not match")
}
