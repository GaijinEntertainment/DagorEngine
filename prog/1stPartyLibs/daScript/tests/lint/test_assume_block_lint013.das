options gen2

require dastest/testing_boost
require daslib/ast
require daslib/lint
require strings

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_assume_block_lint013_fixture.das"
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
def test_assume_block_no_false_lint013(t : T?) {
    t |> run("LINT013 does not false-fire on assume-aliased block whose args are used") @@(t : T?) {
        compile_and_collect(t) $(issues; count) {
            var lint013 = 0
            for (issue in issues) {
                if (find(issue, "LINT013") >= 0) {
                    lint013 ++
                }
            }
            // The alias-definition blocks read every argument; none must be flagged.
            t |> equal(lint013, 0)
        }
    }
}
