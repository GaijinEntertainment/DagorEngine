options gen2

require dastest/testing_boost
require daslib/ast
require daslib/rtti
require daslib/lint
require daslib/perf_lint
require daslib/style_lint
require daslib/lint_config
require daslib/strings_boost
require strings

// The lint-dirty fixture lives as an embedded string, injected via
// set_file_source — an on-disk copy would (correctly) fail the repo lint
// gate, and nolint-ing it would suppress the very issues this test collects.
// Line numbers are index+1; the assertions below pin 9 / 11 / 14.
def fixture_source() : string {
    return [
        /*  1 */ "options gen2",
        /*  2 */ "",
        /*  3 */ "// virtual fixture for test_collect_issues.das — one LINT003",
        /*  4 */ "// (var-can-be-let), one PERF018 (index-only range loop), one",
        /*  5 */ "// STYLE024 (redundant unsafe wrap), at pinned line numbers",
        /*  6 */ "",
        /*  7 */ "[export]",
        /*  8 */ "def fixture_function() : int \{",
        /*  9 */ "    var can_be_const <- [1, 2, 3]",
        /* 10 */ "    var total = 0",
        /* 11 */ "    for (i in range(length(can_be_const))) \{",
        /* 12 */ "        total += can_be_const[i]",
        /* 13 */ "    \}",
        /* 14 */ "    let wrapped = unsafe(3 + 4)",
        /* 15 */ "    return total + wrapped",
        /* 16 */ "\}"
    ] |> join("\n")
}

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_collect_issues_fixture.das"  // virtual — not on disk
}

def check_issue(t : T?; issue : LintIssue; code : string; line : int; needle : string) {
    t |> equal(issue.code, code)
    t |> equal(issue.line, line)
    t |> success(issue.column >= 0)  // 0-based byte column — 0 is a legal start
    t |> success(issue.file |> ends_with("_collect_issues_fixture.das"))
    t |> success(find(issue.message, needle) >= 0)
}

[test]
def test_collect_issues(t : T?) {
    t |> run("*_collect_issues emit structured twins of the string collectors") @@(t : T?) {
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
                        t |> failure("fixture failed to compile: {issues}")
                        return
                    }
                    let empty_set : table<string>

                    var paranoid_issues : array<LintIssue>
                    let paranoid_n = paranoid_collect_issues(program, paranoid_issues, empty_set, empty_set)
                    t |> equal(paranoid_n, 1)
                    t |> equal(length(paranoid_issues), 1)
                    check_issue(t, paranoid_issues[0], "LINT003", 9, "can_be_const")

                    var perf_issues : array<LintIssue>
                    let perf_n = perf_lint_collect_issues(program, perf_issues, empty_set, empty_set)
                    t |> equal(perf_n, 1)
                    t |> equal(length(perf_issues), 1)
                    check_issue(t, perf_issues[0], "PERF018", 11, "direct iteration")

                    var style_issues : array<LintIssue>
                    let style_n = style_lint_collect_issues(program, style_issues, empty_set, empty_set)
                    t |> equal(style_n, 1)
                    t |> equal(length(style_issues), 1)
                    check_issue(t, style_issues[0], "STYLE024", 14, "redundant 'unsafe(...)' wrap")

                    // string collectors stay the source of truth — same finding,
                    // same order, message embedded in the display string
                    var paranoid_strings : array<string>
                    let strings_n = paranoid_collect(program, paranoid_strings, empty_set, empty_set)
                    t |> equal(strings_n, paranoid_n)
                    t |> success(paranoid_strings[0] |> starts_with(paranoid_issues[0].message))
                }
            }
        }
    }
}
