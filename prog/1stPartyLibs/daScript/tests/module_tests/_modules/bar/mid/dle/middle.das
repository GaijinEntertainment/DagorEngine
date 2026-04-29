// middle.das — Deeply nested sub-module of bar, returns 1
//
// Part of the bar module dependency chain. Loaded by `require mid.dle.middle`
// from bar.das. The dotted path "mid.dle.middle" is resolved by module_default:
// splits into ["mid","dle","middle"], appended to bar.das's directory →
// bar/mid/dle/middle.das.
//
// This tests that deeply nested dotted require paths resolve correctly.

options gen2
def get_test_value() {
    return 1
}
