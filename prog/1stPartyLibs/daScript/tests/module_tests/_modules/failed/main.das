// failed/main.das — Script that should FAIL to compile
//
// This script requires daslib/strings_boost, but the project's
// module_allowed() blocks "strings_boost". The compiler should reject
// this require and report an error.
//
// The test_modules.das test_module_failed() function verifies that
// compile_file returns ok=false for this script.

options gen2
require daslib/strings_boost

[export]
def test {
    feint("hello, world!\n")
    return true
}
