// main.das — file-path require integration test entry.
//
// Exercises all three forms in default (no-project) mode:
//   ./filepath_helpers.das         — relative to this file
//   ./sub/filepath_utility.das     — relative subdir
//   ../filepath/filepath_helpers.das as h_dotdot — same file via ..
//                                                   (must dedup with filepath_helpers)
//   %/tests/module_tests/_modules/filepath/filepath_helpers.das as h_root —
//                                    same file via dasroot (must dedup too)
//
// All three aliases of filepath_helpers.das must resolve to the SAME Module
// instance (no duplicate-symbol errors) thanks to getPrerequisits' moduleName
// + isSameFileName dedup.
//
// Module names are deliberately filepath_*-prefixed to avoid collisions with
// other "shared" modules in the codebase (e.g. dasPEG's `module helpers
// shared`).

options gen2

require ./filepath_helpers.das
require ./sub/filepath_utility.das
require ../filepath/filepath_helpers.das as h_dotdot
require %/tests/module_tests/_modules/filepath/filepath_helpers.das as h_root

[export]
def test {
    assert(filepath_helpers::get_helper_value() == 11)
    assert(filepath_utility::get_utility_value() == 22)
    assert(h_dotdot::get_helper_value() == 11)
    assert(h_root::get_helper_value() == 11)
    return true
}
