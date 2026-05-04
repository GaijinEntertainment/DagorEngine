options gen2
options no_aot
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/ast_boost
require daslib/ast_cursor
require daslib/rtti
require strings

def fixture_path() : string {
    return "{get_das_root()}/tests/daslib/ast_cursor_fixture.das"
}

def compile_fixture(blk : block<(program : smart_ptr<Program>) : void>) {
    var inscope access <- make_file_access("")
    using() <| $(var mg : ModuleGroup) {
        using() <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.export_all = true
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) <| $(ok; program; issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; ctx; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    invoke(blk, program)
                }
            }
        }
    }
}

[test]
def test_find_var_at_cursor(t : T?) {
    t |> run("finds ExprVar at variable reference") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprVar(a) at 10:11-10:12
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(length(hits) > 0, "should find at least one hit")
            t2 |> success(hits[0].rtti == "ExprVar", "first hit should be ExprVar, got {hits[0].rtti}")
            t2 |> success(hits[0].name == "a", "should be named 'a', got '{hits[0].name}'")
        }
    }
}

[test]
def test_find_call_at_cursor(t : T?) {
    t |> run("finds ExprCall at function call") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprCall(print) at 19:4-19:9 (line 19 after macro expansion of string builder)
            let hits = find_at_cursor(program, "ast_cursor_fixture", 19, 5)
            var found_call = false
            for (h in hits) {
                if (h.rtti == "ExprCall" && h.name == "print") {
                    found_call = true
                    break
                }
            }
            t2 |> success(found_call, "should find ExprCall(print)")
        }
    }
}

[test]
def test_innermost_first(t : T?) {
    t |> run("innermost expression comes first") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprOp2(+) at 10:13-10:14 — contains just the '+', inside ExprBlock(9:26-11:1)
            // Hitting col 13 should find ExprOp2 inside ExprBlock
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 13)
            t2 |> success(length(hits) >= 2, "should find multiple hits, got {length(hits)}")
            // first should be the innermost ExprOp2
            t2 |> success(hits[0].rtti == "ExprOp2", "first should be ExprOp2, got {hits[0].rtti}")
            // ExprBlock should be further out
            var found_block = false
            for (i in range(1, length(hits))) {
                if (hits[i].rtti == "ExprBlock") {
                    found_block = true
                    break
                }
            }
            t2 |> success(found_block, "should find ExprBlock after ExprOp2")
        }
    }
}

[test]
def test_enclosing_function(t : T?) {
    t |> run("reports enclosing function") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprVar(a) at 10:11 is inside function 'add'
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(length(hits) > 0, "should find hits")
            t2 |> success(hits[0].func != null, "should have enclosing function")
            t2 |> success(string(hits[0].func.name) == "add", "should be in function 'add', got '{hits[0].func.name}'")
        }
    }
}

[test]
def test_field_access(t : T?) {
    t |> run("finds ExprField for field access") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprField(x) at 21:13-21:14
            let hits = find_at_cursor(program, "ast_cursor_fixture", 21, 13)
            var found_field = false
            for (h in hits) {
                if (h.rtti == "ExprField" && h.name == "x") {
                    found_field = true
                    break
                }
            }
            t2 |> success(found_field, "should find ExprField(x)")
        }
    }
}

[test]
def test_no_hits_outside(t : T?) {
    t |> run("returns empty for position outside code") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            let hits = find_at_cursor(program, "ast_cursor_fixture", 999, 1)
            t2 |> success(length(hits) == 0, "should find no hits")
        }
    }
}

[test]
def test_describe_hit(t : T?) {
    t |> run("describe produces readable output") <| @(t2 : T?) {
        compile_fixture() <| $(program) {
            // ExprVar(a) at 10:11
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(length(hits) > 0, "should find hits")
            let desc = describe(hits[0])
            t2 |> success(find(desc, "ExprVar") >= 0, "describe should contain ExprVar")
            t2 |> success(find(desc, "a") >= 0, "describe should contain name")
        }
    }
}
