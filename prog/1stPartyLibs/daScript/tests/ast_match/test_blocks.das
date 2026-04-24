options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Block as call argument — basic matching ─────────────────────────

[test]
def test_call_with_block_no_args(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); })
    let r = qmatch(expr, foo() $ { bar(); })
    t |> success(r.matched, "call with block should match")
}

[test]
def test_call_with_block_wrong_body(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); })
    let r = qmatch(expr, foo() $ { baz(); })
    t |> success(!r.matched, "wrong block body should fail")
}

[test]
def test_call_with_block_wrong_call_name(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); })
    let r = qmatch(expr, other() $ { bar(); })
    t |> success(!r.matched, "wrong call name should fail")
}

// ── Block with typed arguments ──────────────────────────────────────

[test]
def test_call_with_typed_block(t : T?) {
    var inscope expr <- qmacro(foo() $(a : int) { bar(a); })
    let r = qmatch(expr, foo() $(a : int) { bar(a); })
    t |> success(r.matched, "typed block should match")
}

[test]
def test_call_with_typed_block_wrong_arg_name(t : T?) {
    var inscope expr <- qmacro(foo() $(a : int) { bar(a); })
    let r = qmatch(expr, foo() $(x : int) { bar(a); })
    t |> success(!r.matched, "wrong block arg name should fail")
}

[test]
def test_call_with_typed_block_wrong_arg_type(t : T?) {
    var inscope expr <- qmacro(foo() $(a : int) { bar(a); })
    let r = qmatch(expr, foo() $(a : float) { bar(a); })
    t |> success(!r.matched, "wrong block arg type should fail")
}

[test]
def test_call_with_two_block_args(t : T?) {
    var inscope expr <- qmacro(foo() $(a : int, b : float) { bar(a, b); })
    let r = qmatch(expr, foo() $(a : int, b : float) { bar(a, b); })
    t |> success(r.matched, "two typed block args should match")
}

[test]
def test_call_with_two_block_args_wrong_second(t : T?) {
    var inscope expr <- qmacro(foo() $(a : int, b : float) { bar(a, b); })
    let r = qmatch(expr, foo() $(a : int, b : string) { bar(a, b); })
    t |> success(!r.matched, "wrong second block arg type should fail")
}

// ── Block with return type ──────────────────────────────────────────

[test]
def test_block_with_return_type(t : T?) {
    var inscope expr <- qmacro(invoke(cb) $(a : int) : float { return 1.0; })
    let r = qmatch(expr, invoke(cb) $(a : int) : float { return 1.0; })
    t |> success(r.matched, "block with return type should match")
}

[test]
def test_block_with_wrong_return_type(t : T?) {
    var inscope expr <- qmacro(invoke(cb) $(a : int) : float { return 1.0; })
    let r = qmatch(expr, invoke(cb) $(a : int) : int { return 1.0; })
    t |> success(!r.matched, "wrong return type should fail")
}

[test]
def test_block_no_return_pattern_matches_any(t : T?) {
    var inscope expr <- qmacro(invoke(cb) $(a : int) : float { return 1.0; })
    let r = qmatch(expr, invoke(cb) $(a : int) { return 1.0; })
    t |> success(r.matched, "omitted return type should match any")
}

// ── No-arg blocks ───────────────────────────────────────────────────

[test]
def test_block_no_arg(t : T?) {
    var inscope expr <- qmacro(defer() $ { print(); })
    let r = qmatch(expr, defer() $ { print(); })
    t |> success(r.matched, "no-arg block should match")
}

[test]
def test_block_no_arg_wrong_body(t : T?) {
    var inscope expr <- qmacro(defer() $ { print(); })
    let r = qmatch(expr, defer() $ { foo(); })
    t |> success(!r.matched, "wrong no-arg block body should fail")
}

// ── Block with multiple statements ──────────────────────────────────

[test]
def test_block_multi_stmts(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); baz(); })
    let r = qmatch(expr, foo() $ { bar(); baz(); })
    t |> success(r.matched, "multi-stmt block should match")
}

[test]
def test_block_multi_stmts_wrong_order(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); baz(); })
    let r = qmatch(expr, foo() $ { baz(); bar(); })
    t |> success(!r.matched, "wrong stmt order should fail")
}

[test]
def test_block_multi_stmts_extra(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar(); baz(); })
    let r = qmatch(expr, foo() $ { bar(); })
    t |> success(!r.matched, "fewer pattern stmts should fail")
}

// ── Call with args before block ─────────────────────────────────────

[test]
def test_call_with_args_and_block(t : T?) {
    var inscope expr <- qmacro(foo(1, 2) $ { bar(); })
    let r = qmatch(expr, foo(1, 2) $ { bar(); })
    t |> success(r.matched, "call with args + block should match")
}

[test]
def test_call_with_wrong_args_and_block(t : T?) {
    var inscope expr <- qmacro(foo(1, 2) $ { bar(); })
    let r = qmatch(expr, foo(1, 3) $ { bar(); })
    t |> success(!r.matched, "wrong call arg should fail")
}

// ── Block with var argument ─────────────────────────────────────────

[test]
def test_block_var_arg(t : T?) {
    var inscope expr <- qmacro(build_string() $(var writer) { write(writer, "hello"); })
    let r = qmatch(expr, build_string() $(var writer) { write(writer, "hello"); })
    t |> success(r.matched, "block with var arg should match")
}
