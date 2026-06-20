options gen2

require dastest/testing_boost
require daslib/rtti

[test]
def test_extract_lint_code(t : T?) {
    t |> run("strips prefix before first colon") @@(t : T?) {
        t |> equal(extract_lint_code("LINT001: foo bar"), "LINT001")
        t |> equal(extract_lint_code("PERF018: nested loop"), "PERF018")
        t |> equal(extract_lint_code("STYLE005: early-exit braces"), "STYLE005")
    }
    t |> run("empty string when no colon present") @@(t : T?) {
        t |> equal(extract_lint_code("no colon here"), "")
        t |> equal(extract_lint_code(""), "")
    }
    t |> run("first colon wins") @@(t : T?) {
        t |> equal(extract_lint_code("LINT003: foo: bar: baz"), "LINT003")
    }
}
