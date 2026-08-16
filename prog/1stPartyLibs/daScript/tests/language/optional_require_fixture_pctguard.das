options gen2

module optional_require_fixture_pctguard shared public

require dastest/testing_boost

// Same-directory fixture for optional_require.das: the target of a %/-rooted PATH-guarded
// require (`require ?%/tests/language/optional_require_fixture.das …`) — loads because the
// guard's own file resolves. `shared` so `typeinfo builtin_module_exists` sees it.

def public pctguard_marker : int {
    return 44

}

[test]
def test_fixture_pctguard_self(t : T?) {
    t |> equal(pctguard_marker(), 44)
}
