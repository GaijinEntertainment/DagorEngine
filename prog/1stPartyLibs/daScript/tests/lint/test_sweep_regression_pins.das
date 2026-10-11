options gen2

require dastest/testing_boost
require daslib/ast
require daslib/rtti
require daslib/lint
require daslib/style_lint
require daslib/lint_config
require daslib/strings_boost
require strings

// Pins for three behaviors the comment-sweep branch changed, none of which the
// fixture corpus can distinguish (the dirty sources live as embedded strings —
// on-disk copies would fail the repo lint gate).

def defer_prefix_source() : string {
    return [
        /*  1 */ "options gen2",
        /*  2 */ "",
        /*  3 */ "def each_item(items : array<int>; blk : block<(x : int) : void>) \{",
        /*  4 */ "    for (i in items) \{",
        /*  5 */ "        invoke(blk, i)",
        /*  6 */ "    \}",
        /*  7 */ "\}",
        /*  8 */ "",
        /*  9 */ "[export]",
        /* 10 */ "def deferred_consumer() \{",
        /* 11 */ "    var deferred_items <- [1, 2, 3]",
        /* 12 */ "    var total = 0",
        /* 13 */ "    deferred_items |> each_item() <| $(x : int) \{",
        /* 14 */ "        total += x",
        /* 15 */ "    \}",
        /* 16 */ "\}"
    ] |> join("\n")
}

def lint022_latch_source() : string {
    return [
        /*  1 */ "options gen2",
        /*  2 */ "options _unused_private = true",
        /*  3 */ "def private dead_helper(x : int) : int \{",
        /*  4 */ "    return x * 2",
        /*  5 */ "\}",
        /*  6 */ "",
        /*  7 */ "[no_lint, export]",
        /*  8 */ "def trailing_shape() : int \{",
        /*  9 */ "    return 7",
        /* 10 */ "\}"
    ] |> join("\n")
}

def compile_and_collect(t : T?; name, source : string; style : bool; blk : block<(issues : array<LintIssue>) : void>) {
    var inscope access <- make_file_access("")
    let path = "{get_das_root()}/tests/lint/{name}"
    set_file_source(access, path, source)
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
                let empty_set : table<string>
                var found : array<LintIssue>
                if (style) {
                    style_lint_collect_issues(program, found, empty_set, empty_set)
                } else {
                    paranoid_collect_issues(program, found, empty_set, empty_set)
                }
                invoke(blk, found)
            }
        }
    }
}

[test]
def test_defer_scan_word_boundary(t : T?) {
    t |> run("an identifier starting with 'defer' adds no second finding on its line") @@(t : T?) {
        compile_and_collect(t, "_defer_prefix_fixture.das", defer_prefix_source(), true) $(issues) {
            var on_pipe_line = 0
            for (issue in issues) {
                if (issue.line == 13) {
                    on_pipe_line++
                    t |> equal(issue.code, "STYLE001")
                }
            }
            t |> equal(on_pipe_line, 1)
        }
    }
}

[test]
def test_lint022_survives_trailing_no_lint(t : T?) {
    t |> run("a trailing [no_lint] function does not silence the module's LINT022") @@(t : T?) {
        compile_and_collect(t, "_lint022_latch_fixture.das", lint022_latch_source(), false) $(issues) {
            var lint022 = 0
            for (issue in issues) {
                if (issue.code == "LINT022" && find(issue.message, "dead_helper") >= 0) {
                    lint022++
                }
            }
            t |> equal(lint022, 1)
        }
    }
}
