options gen2

require dastest/testing_boost
require daslib/ast
require daslib/rtti
require daslib/lint
require daslib/lint_config
require daslib/style_lint
require daslib/strings_boost
require strings

def private fixture_path() : string {
    return "{get_das_root()}/tests/lint/_unsafe_block_scope_fixture.das"
}

def private style_codes(t : T?; fold : bool) : array<string> {
    var codes : array<string>
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            cop.no_fold_unsafe = !fold
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    t |> failure("fixture failed to compile: {issues}")
                    return
                }
                let empty_set : table<string>
                var style_issues : array<LintIssue>
                style_lint_collect_issues(program, style_issues, empty_set, empty_set)
                for (issue in style_issues) {
                    codes |> push(issue.code)
                }
            }
        }
    }
    return <- codes
}

[test]
def test_unsafe_block_scope(t : T?) {
    t |> run("with no_fold_unsafe the block-form scope rules see the wrapper") @@(t : T?) {
        let codes <- style_codes(t, false)
        t |> equal(join(codes, ","), "STYLE025", "one unsafe op in the block narrows to unsafe(<sub-expr>)")
    }
    t |> run("without it foldUnsafe erases the wrapper before the walk, and the rules go silent") @@(t : T?) {
        let codes <- style_codes(t, true)
        t |> equal(join(codes, ","), "", "no scope left to report on")
    }
}
