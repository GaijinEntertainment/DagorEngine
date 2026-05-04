// bar.das — Module providing get_test_value() by combining left + middle + right
//
// Loaded when a script does `require bar.bar`. The dotted name "bar.bar" is resolved
// by module_default in common.das_project: it splits by "." into ["bar","bar"],
// then resolves relative to the requiring file's directory → bar/bar.das.
//
// This module demonstrates multi-level dotted require paths:
//   - `require left`           → bar/left.das (returns 9)
//   - `require right`          → bar/right.das (returns 3)
//   - `require mid.dle.middle` → bar/mid/dle/middle.das (returns 1)
//
// get_test_value() = 9 + 1 + 3 = 13

options gen2
require left
require right
require mid.dle.middle

def get_test_value {
    return left::get_test_value() + middle::get_test_value() + right::get_test_value()
}
