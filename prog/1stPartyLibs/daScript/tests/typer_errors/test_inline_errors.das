// [inline] is fail-closed: a shape the inliner can't splice, an address-taken
// [inline] function, recursion through the inline graph, or a call position with
// no lowering must each fail compilation with a targeted message (ast_inline.cpp)
options gen2
options no_aot     // invokes the compiler at runtime (compile_file) — not AOT-linkable

require daslib/ast
require strings
require dastest/testing_boost public


def compile_issues(file_path : string) : string {
    var issuesText = ""
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(_ok, _program, issues) {
                issuesText := issues
            }
        }
    }
    return issuesText
}

def expect_issue(t : T?; fixture, message : string) {
    let issues = compile_issues("{get_das_root()}/tests/typer_errors/{fixture}")
    t |> success(find(issues, message) >= 0,
        "expected '{message}' compiling {fixture}, got: {issues}")
}


[test]
def test_inline_contract_errors(t : T?) {
    t |> run("taking the address of an [inline] function") @(t : T?) {
        t |> expect_issue("_inline_addr.das",
            "can't take the address of [inline] function")
    }
    t |> run("recursion through the inline graph") @(t : T?) {
        t |> expect_issue("_inline_recursion.das",
            "[inline] function is recursive through the inline graph")
    }
    t |> run("[inline] call in a global initializer") @(t : T?) {
        t |> expect_issue("_inline_global_init.das",
            "in a global initializer")
    }
    t |> run("[inline] call in a field initializer") @(t : T?) {
        t |> expect_issue("_inline_field_init.das",
            "in a field initializer")
    }
    t |> run("[inline] on a non-ExprOp punctuation function") @(t : T?) {
        t |> expect_issue("_inline_operator.das",
            "[inline] does not support operator '[]'")
    }
    t |> run("recursion through an operator site") @(t : T?) {
        t |> expect_issue("_inline_operator_recursion.das",
            "[inline] function is recursive through the inline graph")
    }
    t |> run("statement-needing call in a ?? default") @(t : T?) {
        t |> expect_issue("_inline_coalesce.das",
            "inside ?? or a safe-navigation suffix")
    }
    t |> run("[inline] combined with [never_inline]") @(t : T?) {
        t |> expect_issue("_inline_never_inline_conflict.das",
            "[never_inline] conflicts with [inline]")
    }
    t |> run("early return with a tail inside a finally-carrying block") @(t : T?) {
        t |> expect_issue("_inline_finally_hazard.das",
            "early returns conflict with a finally section")
    }
    t |> run("[inline] call in a mixed-type ternary arm") @(t : T?) {
        t |> expect_issue("_inline_mixed_ternary.das",
            "inside a mixed-type ternary")
    }
    t |> run("cross-module body with user-spelled _:: dispatch") @(t : T?) {
        t |> expect_issue("_inline_scope_dispatch.das",
            "body dispatches '_::help_scale' at the call site's module")
    }
    t |> run("[inline] call site inside a different module's with scope") @(t : T?) {
        t |> expect_issue("_inline_scope_in_with.das",
            "the call site resolves inside with (module _inline_scope_dispatch_helper)")
    }
}
