options gen2

module optional_require_fixture_pctskip shared public

// Same-directory fixture for optional_require.das: the target of a %/-rooted path-guarded
// require whose guard does NOT resolve — this file exists on disk precisely so the test can
// prove the require was skipped by the guard, not by target resolvability.

def public pctskip_marker : int {
    return 45

}
