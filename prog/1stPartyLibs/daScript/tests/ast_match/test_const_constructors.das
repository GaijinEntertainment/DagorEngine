options gen2
options indenting = 4
options rtti
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/ast_match
require dastest/testing_boost public

[export, marker(no_coverage)]
def target_range13() : range {
    return range(13)
}

[test]
def test_range_one_val_folded(t : T?) {
    // Compiled: range(13) folds to ExprConstRange(0, 13)
    var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_range13)
    if (get_ptr(func) == null) { t |> success(false, "not found"); return ; }
    var x_val : int
    let r = qmatch_function(func) $ {
        return range($v(x_val), 13)
    }
    t |> success(r.matched, "should match folded range(13): error={int(r.error)}")
    if (r.matched) {
        t |> success(x_val == 0, "x should be 0, got {x_val}")
    }
}

[test]
def test_range_one_val_raw(t : T?) {
    // Raw qmacro: range(13) stays as ExprCall with 1 arg
    // One-value expansion: range(13) = range(0, 13)
    var inscope expr <- qmacro(range(13))
    var x_val : int
    let r = qmatch(expr, range($v(x_val), 13))
    t |> success(r.matched, "raw range(13) should match: error={int(r.error)}")
    if (r.matched) {
        t |> success(x_val == 0, "x should be 0, got {x_val}")
    }
}

[test]
def test_range_two_val_raw(t : T?) {
    // Two-value form stays as ExprCall with 2 args — should match directly
    var inscope expr <- qmacro(range(5, 13))
    var x_val : int
    let r = qmatch(expr, range($v(x_val), 13))
    t |> success(r.matched, "raw range(5, 13) should match: error={int(r.error)}")
    if (r.matched) {
        t |> success(x_val == 5, "x should be 5, got {x_val}")
    }
}

[test]
def test_range_one_val_mismatch(t : T?) {
    // range(13) expanded is range(0, 13) — should not match range($v(x), 20)
    var inscope expr <- qmacro(range(13))
    var x_val : int
    let r = qmatch(expr, range($v(x_val), 20))
    t |> success(!r.matched, "range(13) should not match range(_, 20)")
}

[test]
def test_urange_one_val_raw(t : T?) {
    var inscope expr <- qmacro(urange(10u))
    var x_val : uint
    let r = qmatch(expr, urange($v(x_val), 10u))
    t |> success(r.matched, "raw urange(10u) should match: error={int(r.error)}")
    if (r.matched) {
        t |> success(x_val == 0u, "x should be 0u, got {x_val}")
    }
}

[test]
def test_float2_one_val_raw(t : T?) {
    // float2(3.0) = float2(3.0, 3.0) — broadcast
    var inscope expr <- qmacro(float2(3.0))
    var x_val, y_val : float
    let r = qmatch(expr, float2($v(x_val), $v(y_val)))
    t |> success(r.matched, "raw float2(3.0) should match: error={int(r.error)}")
    if (r.matched) {
        t |> success(x_val == 3.0, "x should be 3.0, got {x_val}")
        t |> success(y_val == 3.0, "y should be 3.0, got {y_val}")
    }
}
