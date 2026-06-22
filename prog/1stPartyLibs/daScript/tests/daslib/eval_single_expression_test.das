options gen2
options no_aot

require dastest/testing_boost public
require daslib/ast_boost
require daslib/rtti
require strings
require math

def fixture_path() : string {
    return "{get_das_root()}/tests/daslib/eval_single_expression_fixture.das"
}

def compile_fixture(t : T?; blk : block<(program : smart_ptr<Program>; ctx : smart_ptr<Context>) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.export_all = true
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    t |> success(false, "fixture compile failed: {issues}")
                    return
                }
                simulate(program) $(sok; ctx; serrors) {
                    if (!sok) {
                        t |> success(false, "fixture simulate failed: {serrors}")
                        return
                    }
                    invoke(blk, program, ctx)
                }
            }
        }
    }
}

def find_foo_field(program : smart_ptr<Program>; field_name : string) : ExpressionPtr {
    var found : ExpressionPtr = null
    for_each_module(get_ptr(program)) $(mod) {
        for_each_structure(mod) $(st) {
            if (st.name == "Foo") {
                for (f in st.fields) {
                    if (f.name == field_name) {
                        found = f.init
                    }
                }
            }
        }
    }
    return found
}

[test]
def test_eval_literal_no_context(t : T?) {
    t |> run("literal init evaluates without a context") <| @(t2 : T?) {
        compile_fixture(t2) $(program; ctx) {
            let init = find_foo_field(program, "bar")
            t2 |> success(init != null, "found Foo.bar init expression")
            var ok = true
            unsafe {
                let v = eval_single_expression(init, ok)
                t2 |> success(ok, "eval ok")
                t2 |> equal(v.x, 3.14, "literal value matches")
            }
        }
    }
}

[test]
def test_eval_literal_with_context(t : T?) {
    t |> run("literal init evaluates via context-aware overload") <| @(t2 : T?) {
        compile_fixture(t2) $(program; ctx) {
            let init = find_foo_field(program, "bar")
            var ok = true
            unsafe {
                let v = eval_single_expression(init, ctx, ok)
                t2 |> success(ok, "eval ok")
                t2 |> equal(v.x, 3.14, "literal value matches")
            }
        }
    }
}

// Regression: `PI` is a `addConstant`-style global with index=-2 (folded away at
// compile time, no runtime slot). Struct-field-default ASTs preserve the unfolded
// ExprVar(PI) reference. Before the SimulateVisitor::visit(ExprVar) fallback,
// re-simulating this generated a SimNode_GetSharedMnh whose mangled-name lookup
// missed -> undefined deref -> access violation.
[test]
def test_eval_folded_away_constexpr_var_no_context(t : T?) {
    t |> run("ExprVar(folded constexpr global) no longer crashes (2-arg overload)") <| @(t2 : T?) {
        compile_fixture(t2) $(program; ctx) {
            let init = find_foo_field(program, "qux")
            t2 |> success(init != null, "found Foo.qux init expression")
            t2 |> success(init.__rtti == "ExprVar", "qux init stays as ExprVar (got {init.__rtti})")
            var ok = true
            unsafe {
                let v = eval_single_expression(init, ok)
                t2 |> success(ok, "eval ok (regression: would crash before fix)")
                t2 |> equal(v.x, PI, "PI value matches math::PI")
            }
        }
    }
}

[test]
def test_eval_folded_away_constexpr_var_with_context(t : T?) {
    t |> run("ExprVar(folded constexpr global) no longer crashes (3-arg overload)") <| @(t2 : T?) {
        compile_fixture(t2) $(program; ctx) {
            let init = find_foo_field(program, "qux")
            var ok = true
            unsafe {
                let v = eval_single_expression(init, ctx, ok)
                t2 |> success(ok, "eval ok (regression: would crash before fix)")
                t2 |> equal(v.x, PI, "PI value matches math::PI")
            }
        }
    }
}

[test]
def test_eval_null_expr_with_context(t : T?) {
    t |> run("null expression sets ok=false safely") <| @(t2 : T?) {
        compile_fixture(t2) $(program; ctx) {
            var ok = true
            unsafe {
                let nullExpr : ExpressionPtr = null
                eval_single_expression(nullExpr, ctx, ok)
                t2 |> success(!ok, "ok set to false for null expression")
            }
        }
    }
}
