options gen2

// Demonstrates installing a specific version (v2.0) of a package.
// Run from this directory:
//
//   daslang ../../../utils/daspkg/main.das -- --root . install
//   daslang main.das
//
// v2.0 has get_version() AND new_in_v2() — an API that doesn't exist in v1.0.

require test_versions/test_versions

[export]
def main() {
    print("version = {get_version()}\n")
    print("new in v2 = {new_in_v2()}\n")
    // Expected output:
    //   version = 2.0
    //   new in v2 = hello from v2
}
