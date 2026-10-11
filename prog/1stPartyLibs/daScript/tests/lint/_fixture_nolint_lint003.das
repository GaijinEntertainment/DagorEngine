options gen2

// Fixture for test_nolint_suppression.das — three LINT003-eligible `var`
// declarations; two carry `// nolint:LINT003` suppressions. Paranoid lint
// should report exactly one issue.

[export]
def fixture_function() : int {
    var should_be_const = 42
    var suppressed_a = 1            // nolint:LINT003
    var suppressed_b = 2            // nolint:LINT003
    return should_be_const + suppressed_a + suppressed_b
}
