// foo_random.das — Sub-module of "foo" providing the test value
//
// Loaded when a script does `require foo.random`.
// The foo.das_module resolver maps "foo.random" → this file with module name "foo_random".
//
// Returns the canonical test value 13, which is verified by main.das.

options gen2
module foo_random

def get_test_value() {
    return 13
}
