options gen2

// This example uses C and C++ modules installed via daspkg from local paths.
// Run from this directory:
//
//   daslang ../../../utils/daspkg/main.das -- --root . install
//   daslang ../../../utils/daspkg/main.das -- --root . build
//   daslang main.das

require fastmath
require counter

[export]
def main() {
    // C module: fast math functions
    print("fast_inv_sqrt(4.0) = {fast_inv_sqrt(4.0)}\n")
    print("fast_lerp(0, 10, 0.3) = {fast_lerp(0.0, 10.0, 0.3)}\n")
    print("fast_clamp_int(15, 0, 10) = {fast_clamp_int(15, 0, 10)}\n")

    // C++ module: counter type
    unsafe {
        var c = make_counter(5)
        increment(c)
        increment(c)
        print("counter(step=5), 2x increment = {get(c)}\n")
    }
}
