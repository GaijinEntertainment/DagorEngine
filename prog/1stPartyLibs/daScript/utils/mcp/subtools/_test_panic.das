options gen2

require ../tools/common.das public

//! Test fixture for run_mcp_subtool. Prints a marker to stdout, then panics
//! to drive a non-zero exit code through the wrapper.

[export]
def main {
    print("STDOUT_BEFORE_PANIC")
    panic("intentional panic from _test_panic subtool")
}
