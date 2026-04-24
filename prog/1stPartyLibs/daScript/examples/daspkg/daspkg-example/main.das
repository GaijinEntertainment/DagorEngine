options gen2

// This example uses packages installed by daspkg.
// Run from this directory:
//
//   daslang ../../../utils/daspkg/main.das -- --root . install
//   daslang main.das

require test_pure/test_pure

[export]
def main() {
    print("{greet("daspkg")}\n")
    print("2 + 3 = {add(2, 3)}\n")
}
