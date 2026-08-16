// static_if must resolve identically in every compile profile. Under the lint/IDE
// profile (no_optimizations + no_infer_time_folding) a const global's init expression
// is never folded in place, so getConstExpr computes the value on the side
// (ast_infer_type_helper.cpp): composed initializers compile, genuinely non-constant
// conditions still refuse.
options gen2
options no_aot     // invokes the compiler at runtime (compile_file) — not AOT-linkable

require daslib/ast
require strings
require dastest/testing_boost public

def compile_lint_profile(file_path : string) : tuple<ok : bool; issues : string> {
    var okc = false
    var issuesText = ""
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.export_all = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(ok, _program, issues) {
                okc = ok
                issuesText := issues
            }
        }
    }
    return (ok = okc, issues = issuesText)
}

[test]
def test_static_if_lint_profile(t : T?) {
    t |> run("composed const-global initializers resolve") @(t : T?) {
        let r = compile_lint_profile("{get_das_root()}/tests/typer_errors/_static_if_composed_init.das")
        t |> success(r.ok, "expected a clean compile, got: {r.issues}")
    }
    t |> run("mutually recursive initializers report the init loop, not a crash") @(t : T?) {
        let r = compile_lint_profile("{get_das_root()}/tests/typer_errors/_static_if_mutual_global.das")
        t |> success(!r.ok, "expected the compile to refuse")
        t |> success(find(r.issues, "initialization loop") >= 0, "wrong error: {r.issues}")
    }
    t |> run("a genuinely non-constant condition still refuses") @(t : T?) {
        let r = compile_lint_profile("{get_das_root()}/tests/typer_errors/_static_if_nonconst.das")
        t |> success(!r.ok, "expected the compile to refuse")
        t |> success(find(r.issues, "static_if must resolve to constant") >= 0, "wrong error: {r.issues}")
    }
}
