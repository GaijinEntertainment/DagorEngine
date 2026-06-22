options gen2

require dastest/testing_boost
require daslib/ast
require daslib/lint
require strings

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_nolint_lint003_fixture.das"
}

def compile_and_collect(t : T?; blk : block<(issues : array<string>; count : int) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    t |> failure("fixture failed to compile: {issues}")
                    return
                }
                var collected : array<string>
                let count = paranoid_collect(program, collected)
                invoke(blk, collected, count)
            }
        }
    }
}

[test]
def test_nolint_lint003_suppression(t : T?) {
    t |> run("// nolint:LINT003 suppresses the matched variable") @@(t : T?) {
        compile_and_collect(t) $(issues; count) {
            // Three `var` declarations are LINT003-eligible; two carry
            // `// nolint:LINT003`, so exactly one should remain.
            t |> equal(count, 1)
            t |> equal(length(issues), 1)
            t |> success(find(issues[0], "LINT003") >= 0)
            t |> success(find(issues[0], "should_be_const") >= 0)
        }
    }
}
