options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Constant matching ───────────────────────────────────────────────────

[test]
def test_const_int(t : T?) {
    var inscope expr <- qmacro(42)
    t |> success(qmatch(expr, 42).matched, "42 == 42")
    t |> success(!qmatch(expr, 99).matched, "42 != 99")
}

[test]
def test_const_float(t : T?) {
    var inscope expr <- qmacro(3.14)
    t |> success(qmatch(expr, 3.14).matched, "3.14 == 3.14")
    t |> success(!qmatch(expr, 2.71).matched, "3.14 != 2.71")
}

[test]
def test_const_bool(t : T?) {
    var inscope expr <- qmacro(true)
    t |> success(qmatch(expr, true).matched, "true == true")
    var inscope f <- qmacro(false)
    t |> success(!qmatch(f, true).matched, "false != true")
}

[test]
def test_const_string(t : T?) {
    var inscope expr <- qmacro("hello")
    t |> success(qmatch(expr, "hello").matched, "hello == hello")
    t |> success(!qmatch(expr, "world").matched, "hello != world")
}

// ── RTTI mismatch ───────────────────────────────────────────────────────

[test]
def test_rtti_mismatch(t : T?) {
    var inscope expr <- qmacro(42)
    let r = qmatch(expr, true)
    t |> success(!r.matched, "int vs bool")
    t |> equal(int(r.error), int(QMatchError.rtti_mismatch))
}

[test]
def test_cross_type_mismatches(t : T?) {
    var inscope i <- qmacro(42)
    t |> success(!qmatch(i, "hello").matched, "int vs string")
    var inscope b <- qmacro(true)
    t |> success(!qmatch(b, 42).matched, "bool vs int")
    var inscope op <- qmacro(a + b)
    t |> success(!qmatch(op, 42).matched, "op2 vs const")
    t |> success(!qmatch(i, a + b).matched, "const vs op2")
}

// ── Null expression ─────────────────────────────────────────────────────

[test]
def test_null(t : T?) {
    var inscope expr : ExpressionPtr
    let r = qmatch(expr, 42)
    t |> success(!r.matched, "null fails")
    t |> equal(int(r.error), int(QMatchError.null_expression))
}

// ── Operator matching ───────────────────────────────────────────────────

[test]
def test_op2(t : T?) {
    var inscope expr <- qmacro(a + b)
    t |> success(qmatch(expr, a + b).matched, "a+b == a+b")
    t |> success(!qmatch(expr, a - b).matched, "a+b != a-b")
    var inscope expr2 <- qmacro(x + b)
    t |> success(!qmatch(expr2, a + b).matched, "x+b != a+b")
}
