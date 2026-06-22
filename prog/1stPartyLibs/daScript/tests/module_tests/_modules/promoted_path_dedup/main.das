options gen2

// Regression: same on-disk `something.das` (declared `module something shared`)
// required from two different files using two different path expressions
// (`./something.das` vs `../something.das`) must dedup to the SAME promoted
// Module via requireEx's path-match (ast_module.cpp). After resolver
// normalization both expressions canonicalize to the same absolute path, so
// the second requireEx hits the already-promoted entry and reuses it.
// Pre-fix this also worked (name-only alias was lax). Post-fix it still must
// work — the test asserts the path-match doesn't over-reject.

require ./from_root.das
require ./sub/from_sub.das

[export]
def test {
    assert(from_root_value() == 42)
    assert(from_sub_value() == 42)
    return true
}
