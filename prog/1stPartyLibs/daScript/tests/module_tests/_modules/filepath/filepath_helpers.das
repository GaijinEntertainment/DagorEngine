// filepath_helpers.das — fixture for file-path require tests.
// Same file is required via several paths to exercise dedup.
// Module name is filepath_-prefixed to avoid collision with other
// modules in the codebase (e.g. dasPEG's `module helpers shared`).

options gen2
module filepath_helpers

[export]
def get_helper_value() : int {
    return 11
}
