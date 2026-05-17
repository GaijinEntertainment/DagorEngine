// main_missing.das — verifies that a missing file-path require produces a
// clean compilation error (not a crash). Used by test_module_filepath_missing,
// which asserts that compile_file returns ok == false.

options gen2

require ./does_not_exist.das

[export]
def test {
    return true
}
