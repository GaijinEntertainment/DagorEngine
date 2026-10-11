options gen2

require dastest/testing_boost
require daslib/ast
require daslib/lint
require strings
require daslib/strings_boost

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_fixture_nolint_lint003.das"
}

def hardening_fixture_path() : string {
    return "{get_das_root()}/tests/lint/_fixture_nolint_hardening.das"
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
def test_nolint_lint003_suppression(t : T?) {
    t |> run("nolint:LINT003 suppresses the matched variable") @@(t : T?) {
        compile_and_collect(t, fixture_path()) $(issues; count) {
            // Three `var` declarations are LINT003-eligible; two carry
            // `// nolint:LINT003`, so exactly one should remain.
            t |> equal(count, 1)
            t |> equal(length(issues), 1)
            t |> success(find(issues[0], "LINT003") >= 0)
            t |> success(find(issues[0], "should_be_const") >= 0)
        }
    }
}

[test]
def test_nolint_hardening(t : T?) {
    t |> run("string-literal, URL, and mid-comment spellings do not suppress") @@(t : T?) {
        compile_and_collect(t, hardening_fixture_path()) $(issues; count) {
            // Eight LINT003-eligible `var` declarations; five carry genuine
            // directives (incl. char-literal and escaped-quote lines), three
            // carry non-directive spellings that must NOT suppress.
            t |> equal(count, 3)
            let text = join(issues, "\n")
            t |> success(find(text, "in_string") >= 0, "string-literal case reported: {text}")
            t |> success(find(text, "in_url") >= 0, "URL case reported: {text}")
            t |> success(find(text, "mid_comment") >= 0, "mid-comment case reported: {text}")
            t |> success(find(text, "honored") < 0, "first-token directives still suppress: {text}")
            t |> success(find(text, "quote_") < 0, "quote-stress lines still suppress: {text}")
        }
    }
}
