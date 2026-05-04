options gen2

// Demonstrates installing a specific version (v1.0) of a package.
// Run from this directory:
//
//   daslang ../../../utils/daspkg/main.das -- --root . install
//   daslang main.das
//
// v1.0 only has get_version(). The new_in_v2() function does NOT exist here.

require test_versions/test_versions

[export]
def main() {
    print("version = {get_version()}\n")
    // Expected output: version = 1.0
}
