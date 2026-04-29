options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

[test]
def test_i_var(t : T?) {
    var inscope expr <- qmacro(my_variable)
    var name : string
    let r = qmatch(expr, $i(name))
    t |> success(r.matched, "should extract var name")
    t |> equal(name, "my_variable")
}

[test]
def test_i_from_op2(t : T?) {
    var inscope expr <- qmacro(a + b)
    var left_name : string
    var right_name : string
    let r = qmatch(expr, $i(left_name) + $i(right_name))
    t |> success(r.matched, "should extract both var names from op2")
    t |> equal(left_name, "a")
    t |> equal(right_name, "b")
}

[test]
def test_i_on_const(t : T?) {
    // $i on a constant expression — should still match (returns "")
    var inscope expr <- qmacro(42)
    var name : string
    let r = qmatch(expr, $i(name))
    t |> success(r.matched, "$i should match any expression")
    t |> equal(name, "")
}

[test]
def test_i_mixed_with_fixed(t : T?) {
    // Pattern: $i(name) + b  — capture left name, fixed right
    var inscope expr <- qmacro(foo + b)
    var name : string
    let r = qmatch(expr, $i(name) + b)
    t |> success(r.matched, "should match $i + fixed var")
    t |> equal(name, "foo")
}

// ── $i in let declarations ────────────────────────────────────────────────

[test]
def test_i_let_name(t : T?) {
    var inscope expr <- qmacro($ { var x = 5; })
    var name : string
    let r = qmatch_block(expr) $ {
        var $i(name) = 5
    }
    t |> success(r.matched, "should capture let var name")
    t |> equal(name, "x")
}

[test]
def test_i_let_name_fixed(t : T?) {
    // Fixed name match — var x should match var x
    var inscope expr <- qmacro($ { var x = 5; })
    let r = qmatch_block(expr) $ {
        var x = 5
    }
    t |> success(r.matched, "fixed let name should match")
}

[test]
def test_i_let_name_wrong(t : T?) {
    // Fixed name mismatch — var x should not match var y
    var inscope expr <- qmacro($ { var x = 5; })
    let r = qmatch_block(expr) $ {
        var y = 5
    }
    t |> success(!r.matched, "wrong let name should fail")
}

[test]
def test_i_let_with_type(t : T?) {
    // Match let with type annotation
    var inscope expr <- qmacro($ { var x : int = 5; })
    var name : string
    let r = qmatch_block(expr) $ {
        var $i(name) : int = 5
    }
    t |> success(r.matched, "should capture let name with type")
    t |> equal(name, "x")
}

// ── $i in for loops ──────────────────────────────────────────────────────

[test]
def test_i_for_iterator(t : T?) {
    var inscope expr <- qmacro($ { for (x in arr) { print(); }; })
    let r = qmatch_block(expr) $ {
        for (x in arr) {
            print()
        }
    }
    t |> success(r.matched, "for iterator name should match")
}

[test]
def test_i_for_wrong_iterator(t : T?) {
    var inscope expr <- qmacro($ { for (x in arr) { print(); }; })
    let r = qmatch_block(expr) $ {
        for (y in arr) {
            print()
        }
    }
    t |> success(!r.matched, "wrong for iterator name should fail")
}

// ── $i capture in for loops ───────────────────────────────────────────────

[test]
def test_i_for_capture(t : T?) {
    var inscope expr <- qmacro($ { for (x in arr) { print(); }; })
    var iter_name : string
    let r = qmatch_block(expr) $ {
        for ($i(iter_name) in arr) {
            print()
        }
    }
    t |> success(r.matched, "should capture for iterator name")
    t |> equal(iter_name, "x")
}
