// foo_main.das — Main entry point for the "foo" module
//
// This is the module file loaded when a script does `require foo`.
// The foo.das_module resolver maps "foo" → this file.
//
// It re-exports get_test_value() by delegating to foo_random::get_test_value().
// The `require foo.random` is resolved by the same foo.das_module resolver
// to foo/foo_random.das.

options gen2
module foo

require foo.random

def get_test_value {
    return foo_random::get_test_value()
}
