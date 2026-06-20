// Verifies issue #2571: type-mismatch messages print the alias short name and a
// single `where '<alias> = <def>'` clause instead of expanding the alias inline.
options gen2
options no_aot     // invokes the compiler at runtime (compile_file) — not AOT-linkable

require daslib/rtti
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
def test_alias_short_name_in_mismatch(t : T?) {
    t |> run("argument mismatch prints alias short name + where clause") @(t : T?) {
        let issues = compile_issues("{get_das_root()}/tests/typer_errors/_alias_arg_mismatch.das")
        // expecting-clause uses the short alias name, not the expanded tuple
        t |> success(find(issues, "expecting 'array<RttiValue> const'") >= 0,
            "short alias name in expecting clause")
        // the alias is defined once in a trailing where clause
        t |> success(find(issues, "where 'RttiValue = tuple<argname:string;") >= 0,
            "where clause defines the alias")
        // the short name must not be doubled with a stray ` aka `
        t |> success(find(issues, "RttiValue aka RttiValue") < 0,
            "no doubled `aka` after the short name")
    }
}
