options gen2

// Companion script for the "Unaligned ABI & Advanced" C integration tutorial.
// This script is loaded from an INLINE string in the C host (no .das file on
// disk is needed at runtime — useful for embedded/self-contained applications).

require tutorial_c_05

[export]
def test() {
    print("Hello from inline script!\n")
    c_greet(42)
}

[export]
def add(a : int; b : int) : int {
    return a + b
}
