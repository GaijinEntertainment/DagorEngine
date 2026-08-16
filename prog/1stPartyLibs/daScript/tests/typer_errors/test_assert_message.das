// The default message a failing assert supplies for itself used to name the OTHER construct:
// a bare static_assert reported "concept failed" and a bare concept_assert reported "static assert
// failed", while both carried the right error code. Codes are pinned by `expect` elsewhere; only a
// compile-and-read-the-text check can pin the wording, so it lives here.
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


[test]
def test_bare_assert_names_its_own_construct(t : T?) {
    t |> run("a bare static_assert reports 'static assert failed'") @(t : T?) {
        let issues = compile_issues("{get_das_root()}/tests/typer_errors/_bare_static_assert.das")
        t |> success(find(issues, "static assert failed") >= 0, "static_assert must name itself")
        t |> success(find(issues, "concept") < 0, "static_assert must not mention concepts at all")
    }
    t |> run("a bare concept_assert reports 'concept assert failed'") @(t : T?) {
        let issues = compile_issues("{get_das_root()}/tests/typer_errors/_bare_concept_assert.das")
        t |> success(find(issues, "concept assert failed") >= 0, "concept_assert must name itself")
        t |> success(find(issues, "static assert failed") < 0, "concept_assert must not claim to be a static assert")
    }
}
