// main_gen1.das — smoke test for the gen1 (indented) parser.
// The require_module_name grammar lives in src/parser/ds_parser.ypp;
// this file just verifies it accepts the same ./, ../, %/ forms.

options gen2 = false

require ./filepath_helpers.das
require %/tests/module_tests/_modules/filepath/filepath_helpers.das as h_root

[export]
def test
    assert(filepath_helpers::get_helper_value() == 11)
    assert(h_root::get_helper_value() == 11)
    return true
