options gen2

module require_as_fixture shared public

require dastest/testing_boost

// Same-directory fixture for require_as.das: the target of a bare same-dir
// `require require_as_fixture as fx` (importName was never set on this rail).

def public require_as_marker : int => 45

[test]
def test_fixture_self(t : T?) {
    t |> equal(require_as_marker(), 45)
}
