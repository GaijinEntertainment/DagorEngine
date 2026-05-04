options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/rtti
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Target functions ─────────────────────────────────────────────────────

[export, marker(no_coverage)]
def target_add(a, b : int) : int {
    return a + b
}

[export, marker(no_coverage)]
def target_three(x, y, z : float) : float {
    return x + y + z
}

// ── qmatch_function argument names ───────────────────────────────────────

[test]
def test_func_arg_names(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    t |> success(get_ptr(func) != null, "should find target_add")
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) : int {
        return a + b
    }
    if (!r.matched) {
        t |> success(false, "error={int(r.error)} expr={r.expr != null ? qm_rtti(r.expr) : "null"}")
    } else {
        t |> success(true)
    }
}

[test]
def test_func_arg_name_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x, y : int) : int {
        return x + y
    }
    t |> success(!r.matched, "wrong arg names should fail")
}

[test]
def test_func_arg_count_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a : int) : int {
        return a
    }
    t |> success(!r.matched, "wrong arg count should fail")
}

[test]
def test_func_return_type_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : int) : float {
        return a + b
    }
    t |> success(!r.matched, "wrong return type should fail")
}

[test]
def test_func_no_return_matches_any(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    // No return type specified — should match any
    let r = qmatch_function(func) $(a, b : int) {
        return a + b
    }
    t |> success(r.matched, "no return type in pattern should match any")
}

[test]
def test_func_no_args_fails_when_target_has_args(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    // No args in pattern — target has args, should fail
    let r = qmatch_function(func) $ {
        return a + b
    }
    t |> success(!r.matched, "no args in pattern should fail when target has args")
}

[test]
def test_func_arg_type_mismatch(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(a, b : float) : int {
        return a + b
    }
    t |> success(!r.matched, "wrong arg type should fail")
}

[test]
def test_func_three_args(t : T?) {
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_three)
    if (get_ptr(func) == null) {
        return
    }
    let r = qmatch_function(func) $(x, y, z : float) : float {
        _wildcard()
    }
    t |> success(r.matched, "three float args should match")
}

// ── qmatch_block argument matching ───────────────────────────────────────

[test]
def test_block_args(t : T?) {
    var inscope expr <- qmacro($(a : int; b : float) : int { return a; })
    let r = qmatch_block(expr) $(a : int; b : float) : int {
        return a
    }
    t |> success(r.matched, "block args should match")
}

[test]
def test_block_args_wrong_name(t : T?) {
    var inscope expr <- qmacro($(a : int; b : float) : int { return a; })
    let r = qmatch_block(expr) $(x : int; b : float) : int {
        return x
    }
    t |> success(!r.matched, "wrong block arg name should fail")
}

[test]
def test_block_args_wrong_type(t : T?) {
    var inscope expr <- qmacro($(a : int; b : float) : int { return a; })
    let r = qmatch_block(expr) $(a : float; b : float) : int {
        return a
    }
    t |> success(!r.matched, "wrong block arg type should fail")
}

[test]
def test_block_no_args_fails_when_target_has_args(t : T?) {
    var inscope expr <- qmacro($(a : int) : int { return a; })
    // No args in pattern — target has args, should fail
    let r = qmatch_block(expr) $ {
        return a
    }
    t |> success(!r.matched, "no args in block pattern should fail when target has args")
}

// ── $a(var) — capture remaining arguments ────────────────────────────────

[export, marker(no_coverage)]
def target_five(a, b : int; c : float; d : string; e : bool) : int {
    return a
}

[test]
def test_a_capture_all(t : T?) {
    // $a(rest) with no fixed args — captures all arguments
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $($a(rest)) {
        _wildcard()
    }
    t |> success(r.matched, "should match with $a capture")
    t |> equal(length(rest), 2, "should capture 2 args")
}

[test]
def test_a_capture_remainder(t : T?) {
    // Match first arg, capture rest
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_three)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(x : float; $a(rest)) {
        _wildcard()
    }
    t |> success(r.matched, "should match first arg and capture rest")
    t |> equal(length(rest), 2, "should capture 2 remaining args")
    if (length(rest) == 2) {
        t |> equal(string(get_ptr(rest[0]).name), "y", "first remaining should be y")
        t |> equal(string(get_ptr(rest[1]).name), "z", "second remaining should be z")
    }
}

[test]
def test_a_capture_one_remaining(t : T?) {
    // Match all but last, capture last via $a
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(a : int; $a(rest)) : int {
        _wildcard()
    }
    t |> success(r.matched, "should match first arg and capture last")
    t |> equal(length(rest), 1, "should capture 1 remaining arg")
    if (length(rest) == 1) {
        t |> equal(string(get_ptr(rest[0]).name), "b", "remaining should be b")
    }
}

[test]
def test_a_capture_empty(t : T?) {
    // Match all fixed args, $a captures zero remaining
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(a, b : int; $a(rest)) : int {
        _wildcard()
    }
    t |> success(r.matched, "should match all fixed args with empty remainder")
    t |> equal(length(rest), 0, "should capture 0 remaining args")
}

[test]
def test_a_too_few_args(t : T?) {
    // More fixed args in pattern than actual — should fail
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(a, b, c : int; $a(rest)) {
        _wildcard()
    }
    t |> success(!r.matched, "too few actual args should fail")
}

[test]
def test_a_capture_five_args(t : T?) {
    // Capture many remaining args
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_five)
    if (get_ptr(func) == null) {
        return
    }
    var inscope rest : array<VariablePtr>
    let r = qmatch_function(func) $(a : int; $a(rest)) {
        _wildcard()
    }
    t |> success(r.matched, "should match first and capture 4 remaining")
    t |> equal(length(rest), 4, "should capture 4 remaining args")
}

[test]
def test_a_block_capture(t : T?) {
    // $a on block arguments
    var inscope expr <- qmacro($(a : int; b : float; c : string) { return a; })
    var inscope rest : array<VariablePtr>
    let r = qmatch_block(expr) $(a : int; $a(rest)) {
        return a
    }
    t |> success(r.matched, "block $a should match")
    t |> equal(length(rest), 2, "block should capture 2 remaining args")
}
