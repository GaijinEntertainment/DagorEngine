options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── $v int ──────────────────────────────────────────────────────────────

[test]
def test_v_int(t : T?) {
    var inscope expr <- qmacro(42)
    var val : int
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should extract int")
    t |> equal(val, 42)
}

[test]
def test_v_int_wrong_type(t : T?) {
    // $v(int) on a float expression should fail
    var inscope expr <- qmacro(3.14)
    var val : int
    let r = qmatch(expr, $v(val))
    t |> success(!r.matched, "float expr should not match int $v")
    t |> equal(int(r.error), int(QMatchError.const_type_mismatch))
}

[test]
def test_v_int_on_non_const(t : T?) {
    // $v(int) on a non-constant expression should fail
    var inscope expr <- qmacro(a + b)
    var val : int
    let r = qmatch(expr, $v(val))
    t |> success(!r.matched, "non-const expr should not match $v")
}

// ── $v float ────────────────────────────────────────────────────────────

[test]
def test_v_float(t : T?) {
    var inscope expr <- qmacro(3.14)
    var val : float
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should extract float")
    t |> success(val > 3.13 && val < 3.15, "val should be ~3.14")
}

[test]
def test_v_float_wrong_type(t : T?) {
    var inscope expr <- qmacro(42)
    var val : float
    let r = qmatch(expr, $v(val))
    t |> success(!r.matched, "int expr should not match float $v")
    t |> equal(int(r.error), int(QMatchError.const_type_mismatch))
}

// ── $v bool ─────────────────────────────────────────────────────────────

[test]
def test_v_bool(t : T?) {
    var inscope expr <- qmacro(true)
    var val : bool
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should extract bool")
    t |> equal(val, true)
}

[test]
def test_v_bool_false(t : T?) {
    var inscope expr <- qmacro(false)
    var val : bool
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should extract false")
    t |> equal(val, false)
}

[test]
def test_v_bool_wrong_type(t : T?) {
    var inscope expr <- qmacro(42)
    var val : bool
    let r = qmatch(expr, $v(val))
    t |> success(!r.matched, "int expr should not match bool $v")
    t |> equal(int(r.error), int(QMatchError.const_type_mismatch))
}

// ── $v string ───────────────────────────────────────────────────────────

[test]
def test_v_string(t : T?) {
    var inscope expr <- qmacro("hello world")
    var val : string
    let r = qmatch(expr, $v(val))
    t |> success(r.matched, "should extract string")
    t |> equal(val, "hello world")
}

[test]
def test_v_string_wrong_type(t : T?) {
    var inscope expr <- qmacro(42)
    var val : string
    let r = qmatch(expr, $v(val))
    t |> success(!r.matched, "int expr should not match string $v")
    t |> equal(int(r.error), int(QMatchError.const_type_mismatch))
}

// ── $v in compound patterns ─────────────────────────────────────────────

[test]
def test_v_in_op2(t : T?) {
    // Pattern: a + $v(val)  — extract the constant from the right side
    var inscope expr <- qmacro(a + 5)
    var val : int
    let r = qmatch(expr, a + $v(val))
    t |> success(r.matched, "should match a + $v")
    t |> equal(val, 5)
}

[test]
def test_v_in_block(t : T?) {
    // $v inside qmatch_block
    var inscope expr <- qmacro($ { return 42; })
    var val : int
    let r = qmatch_block(expr) $ {
        return $v(val)
    }
    t |> success(r.matched, "should extract return value in block")
    t |> equal(val, 42)
}
