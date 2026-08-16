options gen2

module optional_require_fixture shared public

require dastest/testing_boost

// Same-directory fixture for optional_require.das: a pure-das SHARED module with no C++
// module behind it. It is the target of a `require ?<absent-guard> optional_require_fixture`
// line, which must NOT load (plain-name guards are strict), and its FILE doubles as the
// %/-path guard of the pctguard case, which must resolve. `shared` matters: promotion into
// the global module registry is what `typeinfo builtin_module_exists` checks for das modules.

def public fixture_marker : int {
    return 42

}

[test]
def test_fixture_self(t : T?) {
    t |> equal(fixture_marker(), 42)
}
