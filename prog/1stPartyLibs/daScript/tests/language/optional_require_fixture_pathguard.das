options gen2

module optional_require_fixture_pathguard shared public

require dastest/testing_boost

// Same-directory fixture for optional_require.das: the target of a PATH-guarded require
// (`require ?daslib/fio optional_require_fixture_pathguard`) — loads because the guard's
// own file resolves. `shared` so `typeinfo builtin_module_exists` sees it.

def public pathguard_marker : int {
    return 43

}

[test]
def test_fixture_pathguard_self(t : T?) {
    t |> equal(pathguard_marker(), 43)
}
