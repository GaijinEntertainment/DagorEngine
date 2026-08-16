options gen2

require dastest/testing_boost
require daslib/ast
require daslib/rtti
require daslib/lint
require daslib/perf_lint
require daslib/lint_config
require daslib/strings_boost
require strings

// PERF030 false positive: a `delete v` releases the old contents, so a following `v <- ...` leaks
// nothing. One release covers one move - the second move in the fixture is reported, because the
// first one consumed it.
def fixture_source() : string {
    return [
        /*  1 */ "options gen2",
        /*  2 */ "",
        /*  3 */ "[export]",
        /*  4 */ "def fixture_function() : int \{",
        /*  5 */ "    var fblob <- [1, 2, 3]",
        /*  6 */ "    var nf <- [4, 5]",
        /*  7 */ "    var kept <- [6]",
        /*  8 */ "    delete fblob",
        /*  9 */ "    fblob <- nf",
        /* 10 */ "    fblob <- kept",
        /* 11 */ "    return length(fblob)",
        /* 12 */ "\}"
    ] |> join("\n")
}

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_perf030_delete_before_move_fixture.das"  // virtual - not on disk
}

[test]
def test_perf030_skips_move_after_delete(t : T?) {
    t |> run("PERF030 fires only for a move whose target was not released") @@(tt : T?) {
        var inscope access <- make_file_access("")
        set_file_source(access, fixture_path(), fixture_source())
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.export_all = true
                cop.lint_check = true
                cop.no_optimizations = true
                cop.no_infer_time_folding = true
                compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    if (!ok) {
                        tt |> failure("fixture failed to compile: {issues}")
                        return
                    }
                    let empty_set : table<string>
                    var perf_issues : array<LintIssue>
                    perf_lint_collect_issues(program, perf_issues, empty_set, empty_set)
                    var reported = 0
                    var reported_line = -1
                    for (i in perf_issues) {
                        if (i.code == "PERF030") {
                            reported++
                            reported_line = i.line
                        }
                    }
                    // line 9 is covered by the delete above it; line 10 is not - the move at 9
                    // consumed the release
                    tt |> equal(reported, 1)
                    tt |> equal(reported_line, 10)
                }
            }
        }
    }
}
