options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ── Nested blocks ───────────────────────────────────────────────────

[test]
def test_nested_blocks(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar() $(x : int) { baz(x); }; })
    let r = qmatch(expr, foo() $ { bar() $(x : int) { baz(x); }; })
    t |> success(r.matched, "nested blocks should match")
}

[test]
def test_nested_blocks_wrong_inner_body(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar() $(x : int) { baz(x); }; })
    let r = qmatch(expr, foo() $ { bar() $(x : int) { other(x); }; })
    t |> success(!r.matched, "wrong inner block body should fail")
}

[test]
def test_nested_blocks_wrong_inner_arg(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar() $(x : int) { baz(x); }; })
    let r = qmatch(expr, foo() $ { bar() $(y : int) { baz(x); }; })
    t |> success(!r.matched, "wrong inner block arg name should fail")
}

[test]
def test_nested_blocks_wrong_inner_arg_type(t : T?) {
    var inscope expr <- qmacro(foo() $ { bar() $(x : int) { baz(x); }; })
    let r = qmatch(expr, foo() $ { bar() $(x : float) { baz(x); }; })
    t |> success(!r.matched, "wrong inner block arg type should fail")
}

// ── Three-level nesting ─────────────────────────────────────────────

[test]
def test_three_level_nesting(t : T?) {
    var inscope expr <- qmacro(a() $ { b() $ { c() $ { print(); }; }; })
    let r = qmatch(expr, a() $ { b() $ { c() $ { print(); }; }; })
    t |> success(r.matched, "three-level nested blocks should match")
}

[test]
def test_three_level_nesting_wrong_deepest(t : T?) {
    var inscope expr <- qmacro(a() $ { b() $ { c() $ { print(); }; }; })
    let r = qmatch(expr, a() $ { b() $ { c() $ { foo(); }; }; })
    t |> success(!r.matched, "wrong deepest block body should fail")
}

// ── Blocks inside qmatch_block ──────────────────────────────────────

[test]
def test_block_in_qmatch_block(t : T?) {
    var inscope expr <- qmacro($ { foo() $ { bar(); }; })
    let r = qmatch_block(expr) $ {
        foo() $ {
            bar()
        }
    }
    t |> success(r.matched, "block inside qmatch_block should match")
}

[test]
def test_block_in_qmatch_block_wrong(t : T?) {
    var inscope expr <- qmacro($ { foo() $ { bar(); }; })
    let r = qmatch_block(expr) $ {
        foo() $ {
            baz()
        }
    }
    t |> success(!r.matched, "wrong block inside qmatch_block should fail")
}

[test]
def test_typed_block_in_qmatch_block(t : T?) {
    var inscope expr <- qmacro($ { foo() $(x : int) { bar(x); }; })
    let r = qmatch_block(expr) $ {
        foo() $(x : int) {
            bar(x)
        }
    }
    t |> success(r.matched, "typed block inside qmatch_block should match")
}

// ── Block after wildcard ────────────────────────────────────────────

[test]
def test_block_after_wildcard(t : T?) {
    var inscope expr <- qmacro($ { setup(); foo() $(x : int) { bar(x); }; })
    let r = qmatch_block(expr) $ {
        _wildcard()
        foo() $(x : int) {
            bar(x)
        }
    }
    t |> success(r.matched, "block after wildcard should match")
}

// ── invoke() ────────────────────────────────────────────────────────

[test]
def test_invoke_with_block(t : T?) {
    var inscope expr <- qmacro(invoke(cb) $ { print(); })
    let r = qmatch(expr, invoke(cb) $ { print(); })
    t |> success(r.matched, "invoke with block should match")
}

[test]
def test_invoke_wrong_first_arg(t : T?) {
    var inscope expr <- qmacro(invoke(cb) $ { print(); })
    let r = qmatch(expr, invoke(other) $ { print(); })
    t |> success(!r.matched, "invoke with wrong first arg should fail")
}

// ── Sibling blocks at same level ────────────────────────────────────

[test]
def test_sibling_blocks(t : T?) {
    var inscope expr <- qmacro($ { foo() $ { bar(); }; baz() $(x : int) { qux(x); }; })
    let r = qmatch_block(expr) $ {
        foo() $ {
            bar()
        }
        baz() $(x : int) {
            qux(x)
        }
    }
    t |> success(r.matched, "sibling blocks should match")
}

[test]
def test_sibling_blocks_wrong_second(t : T?) {
    var inscope expr <- qmacro($ { foo() $ { bar(); }; baz() $(x : int) { qux(x); }; })
    let r = qmatch_block(expr) $ {
        foo() $ {
            bar()
        }
        baz() $(x : float) {
            qux(x)
        }
    }
    t |> success(!r.matched, "wrong second sibling block arg type should fail")
}
