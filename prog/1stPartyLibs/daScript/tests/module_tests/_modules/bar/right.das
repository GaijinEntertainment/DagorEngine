// right.das — Sub-module of bar, returns 3
//
// Part of the bar module dependency chain. Loaded by `require right` from bar.das.
// Resolved relative to bar.das's directory by module_default.

options gen2
def get_test_value() {
    return 3
}
