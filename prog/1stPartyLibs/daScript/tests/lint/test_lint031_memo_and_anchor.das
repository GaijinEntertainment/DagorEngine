options gen2

require dastest/testing_boost
require daslib/ast
require daslib/lint
require strings
require daslib/strings_boost

def fixture_path(name : string) : string {
    return "{get_das_root()}/tests/lint/{name}"
}

def compile_and_collect(t : T?; path : string; blk : block<(issues : array<string>; count : int) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
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
def test_lint031_anchor(t : T?) {
    t |> run("the finding anchors at the parameter's line and names the write's line") @@(t : T?) {
        compile_and_collect(t, fixture_path("_fixture_lint031_writer.das")) $(issues; count) {
            t |> equal(count, 1)
            t |> equal(length(issues), 1)
            t |> success(find(issues[0], "LINT031") >= 0, issues[0])
            t |> success(find(issues[0], "_fixture_lint031_writer.das:12:") >= 0, "anchored at `def route(dst : int?)`, line 12: {issues[0]}")
            t |> success(find(issues[0], "at line 14") >= 0, "the text names the write's line 14: {issues[0]}")
        }
    }
}

[test]
def test_lint031_callee_memo_is_per_program(t : T?) {
    t |> run("a callee's verdict does not outlive its program") @@(t : T?) {
        compile_and_collect(t, fixture_path("_fixture_lint031_writer.das")) $(issues; count) {
            t |> equal(count, 1, "the writing fill_one flags its caller")
        }
        compile_and_collect(t, fixture_path("_fixture_lint031_reader.das")) $(issues; count) {
            t |> equal(count, 0, "the same name with a reading body, compiled next in this process, flags nothing: {join(issues, "; ")}")
        }
    }
}
