options gen2

require dastest/testing_boost
require daslib/lint_config

[test]
def test_fixture_name_predicate(t : T?) {
    t |> run("underscore-led names containing 'fixture' are fixtures") @@(t : T?) {
        t |> success(is_lint_fixture_name("_fixture_nolint_lint003.das"))
        t |> success(is_lint_fixture_name("_dim_expr_folding_fixture.das"))
        t |> success(is_lint_fixture_name("_fixtures.das"))
    }
    t |> run("no underscore lead or no 'fixture' - not a fixture") @@(t : T?) {
        t |> success(!is_lint_fixture_name("fixtures.das"))
        t |> success(!is_lint_fixture_name("x_fixture.das"))
        t |> success(!is_lint_fixture_name("_fix.das"))
        t |> success(!is_lint_fixture_name("test_fixture_name_predicate.das"))
    }
}
