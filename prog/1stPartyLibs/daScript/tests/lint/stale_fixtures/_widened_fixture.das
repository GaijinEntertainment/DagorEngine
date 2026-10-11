options gen2

// Suffix-spelled fixture name: matched by the widened is_lint_fixture_name
// predicate (underscore-led + contains "fixture"), NOT by the old _fixture_
// prefix. The deliberate LINT003 below must be skipped, not reported.

[export]
def main {
    var should_be_const = 42
    print("{should_be_const}\n")
}
