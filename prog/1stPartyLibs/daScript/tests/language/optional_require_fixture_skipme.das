options gen2

module optional_require_fixture_skipme shared public

// Same-directory fixture for optional_require.das that must NEVER load: it is the target of
// a require whose PATH guard does not resolve. The file being resolvable on disk is the
// point — a path guard, unlike a plain-name guard, does NOT fall back to target-file
// resolvability (it witnesses a cross-package dependency the target can't express).
// optional_require.das asserts !builtin_module_exists(optional_require_fixture_skipme).

def public skipme_marker : int {
    return 44

}
