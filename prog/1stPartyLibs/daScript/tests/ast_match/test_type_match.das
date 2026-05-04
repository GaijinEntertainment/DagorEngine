options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── ExprTypeDecl matching ────────────────────────────────────────────────

[test]
def test_type_int(t : T?) {
    var inscope expr <- qmacro(type<int>)
    let r = qmatch(expr, type<int>)
    t |> success(r.matched, "type<int> should match")
}

[test]
def test_type_float(t : T?) {
    var inscope expr <- qmacro(type<float>)
    let r = qmatch(expr, type<float>)
    t |> success(r.matched, "type<float> should match")
}

[test]
def test_type_wrong(t : T?) {
    var inscope expr <- qmacro(type<int>)
    let r = qmatch(expr, type<float>)
    t |> success(!r.matched, "type<int> should not match type<float>")
    t |> equal(int(r.error), int(QMatchError.type_mismatch))
}

[test]
def test_type_const_pointer(t : T?) {
    var inscope expr <- qmacro(type<int const?>)
    let r = qmatch(expr, type<int const?>)
    t |> success(r.matched, "type<int const?> should match")
}

[test]
def test_type_const_mismatch(t : T?) {
    var inscope expr <- qmacro(type<int?>)
    let r = qmatch(expr, type<int const?>)
    t |> success(!r.matched, "type<int?> should not match type<int const?>")
}

[test]
def test_type_pointer_mismatch(t : T?) {
    var inscope expr <- qmacro(type<int>)
    let r = qmatch(expr, type<int?>)
    t |> success(!r.matched, "type<int> should not match type<int?>")
}

[test]
def test_type_array(t : T?) {
    var inscope expr <- qmacro(type<array<int>>)
    let r = qmatch(expr, type<array<int>>)
    t |> success(r.matched, "type<array<int>> should match")
}

[test]
def test_type_array_wrong_elem(t : T?) {
    var inscope expr <- qmacro(type<array<int>>)
    let r = qmatch(expr, type<array<float>>)
    t |> success(!r.matched, "type<array<int>> should not match type<array<float>>")
}

[test]
def test_type_table(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<string; int>>)
    t |> success(r.matched, "type<table<string;int>> should match")
}

[test]
def test_type_table_wrong_value(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<string; float>>)
    t |> success(!r.matched, "type<table<string;int>> should not match type<table<string;float>>")
}

// ── $t capture ───────────────────────────────────────────────────────────

[test]
def test_capture_t(t : T?) {
    var inscope expr <- qmacro(type<int>)
    var inscope captured : TypeDeclPtr
    let r = qmatch(expr, type<$t(captured)>)
    t |> success(r.matched, "should capture type with $t")
    if (r.matched) {
        t |> equal(int(get_ptr(captured).baseType), int(Type.tInt))
    }
}

[test]
def test_capture_t_pointer(t : T?) {
    var inscope expr <- qmacro(type<float const?>)
    var inscope captured : TypeDeclPtr
    let r = qmatch(expr, type<$t(captured)>)
    t |> success(r.matched, "should capture pointer type with $t")
    if (r.matched) {
        let td = get_ptr(captured)
        t |> equal(int(td.baseType), int(Type.tPointer))
        t |> success(get_ptr(td.firstType) != null, "should have firstType")
        if (get_ptr(td.firstType) != null) {
            t |> equal(int(get_ptr(td.firstType).baseType), int(Type.tFloat))
            t |> success(get_ptr(td.firstType).flags.constant, "inner type should be const")
        }
    }
}

// ── Type matching in call arguments ──────────────────────────────────────

[test]
def test_type_in_call(t : T?) {
    var inscope expr <- qmacro(foo(type<int>))
    let r = qmatch(expr, foo(type<int>))
    t |> success(r.matched, "foo(type<int>) should match")
}

[test]
def test_type_in_call_wrong(t : T?) {
    var inscope expr <- qmacro(foo(type<int>))
    let r = qmatch(expr, foo(type<float>))
    t |> success(!r.matched, "foo(type<int>) should not match foo(type<float>)")
}

[test]
def test_capture_t_in_call(t : T?) {
    // Capture type from a call argument
    var inscope expr <- qmacro(foo(type<int>))
    var inscope captured : TypeDeclPtr
    let r = qmatch(expr, foo(type<$t(captured)>))
    t |> success(r.matched, "should capture type from call arg")
    if (r.matched) {
        t |> equal(int(get_ptr(captured).baseType), int(Type.tInt))
    }
}

// ── Type matching in blocks ──────────────────────────────────────────────

[test]
def test_type_in_block(t : T?) {
    var inscope expr <- qmacro($ { foo(type<int>); })
    let r = qmatch_block(expr) $ {
        foo(type<int>)
    }
    t |> success(r.matched, "type<int> in block should match")
}

[test]
def test_type_in_block_wrong(t : T?) {
    var inscope expr <- qmacro($ { foo(type<int>); })
    let r = qmatch_block(expr) $ {
        foo(type<float>)
    }
    t |> success(!r.matched, "type<int> in block should not match type<float>")
}

// ── auto (wildcard type) ─────────────────────────────────────────────

[test]
def test_auto_matches_int(t : T?) {
    var inscope expr <- qmacro(type<int>)
    let r = qmatch(expr, type<auto>)
    t |> success(r.matched, "type<auto> should match type<int>")
}

[test]
def test_auto_matches_float(t : T?) {
    var inscope expr <- qmacro(type<float>)
    let r = qmatch(expr, type<auto>)
    t |> success(r.matched, "type<auto> should match type<float>")
}

[test]
def test_auto_matches_string(t : T?) {
    var inscope expr <- qmacro(type<string>)
    let r = qmatch(expr, type<auto>)
    t |> success(r.matched, "type<auto> should match type<string>")
}

[test]
def test_auto_matches_pointer(t : T?) {
    var inscope expr <- qmacro(type<int?>)
    let r = qmatch(expr, type<auto>)
    t |> success(r.matched, "type<auto> should match type<int?>")
}

[test]
def test_array_auto_matches_array_int(t : T?) {
    var inscope expr <- qmacro(type<array<int>>)
    let r = qmatch(expr, type<array<auto>>)
    t |> success(r.matched, "type<array<auto>> should match type<array<int>>")
}

[test]
def test_array_auto_matches_array_float(t : T?) {
    var inscope expr <- qmacro(type<array<float>>)
    let r = qmatch(expr, type<array<auto>>)
    t |> success(r.matched, "type<array<auto>> should match type<array<float>>")
}

[test]
def test_array_auto_rejects_non_array(t : T?) {
    var inscope expr <- qmacro(type<int>)
    let r = qmatch(expr, type<array<auto>>)
    t |> success(!r.matched, "type<array<auto>> should not match type<int>")
}

[test]
def test_table_auto_value(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<string; auto>>)
    t |> success(r.matched, "type<table<string;auto>> should match type<table<string;int>>")
}

[test]
def test_table_auto_key(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<auto; int>>)
    t |> success(r.matched, "type<table<auto;int>> should match type<table<string;int>>")
}

[test]
def test_table_auto_both(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<auto; auto>>)
    t |> success(r.matched, "type<table<auto;auto>> should match type<table<string;int>>")
}

[test]
def test_table_auto_wrong_key(t : T?) {
    var inscope expr <- qmacro(type<table<string; int>>)
    let r = qmatch(expr, type<table<int; auto>>)
    t |> success(!r.matched, "type<table<int;auto>> should not match type<table<string;int>>")
}

[test]
def test_auto_in_call(t : T?) {
    var inscope expr <- qmacro(foo(type<array<int>>))
    let r = qmatch(expr, foo(type<array<auto>>))
    t |> success(r.matched, "foo(type<array<auto>>) should match foo(type<array<int>>)")
}

[test]
def test_auto_in_block(t : T?) {
    var inscope expr <- qmacro($ { foo(type<array<int>>); })
    let r = qmatch_block(expr) $ {
        foo(type<array<auto>>)
    }
    t |> success(r.matched, "type<array<auto>> in block should match")
}
