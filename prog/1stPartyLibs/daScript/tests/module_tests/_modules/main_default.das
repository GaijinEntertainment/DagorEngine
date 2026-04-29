// main_default.das — Default module resolution test (no project file)
//
// This test validates that `require bar.bar` resolves correctly using the
// compiler's built-in default module resolution (no .das_project file).
// The default resolver splits "bar.bar" into path "bar/bar.das" relative
// to this file's directory.
//
// bar::get_test_value() = left(9) + middle(1) + right(3) = 13.
//
// Compiled with: NO project file (usePak=false, plain make_file_access(""))

options gen2
require bar.bar

[export]
def test {
    let btv = bar::get_test_value()
    assert(btv == 13)
    return true
}
