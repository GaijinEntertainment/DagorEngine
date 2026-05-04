options gen2

// Minimal daslang program used by the C++ integration tutorial.
// The C++ host compiles this file, finds the "test" function, and calls it.

[export]
def test() {
    print("Hello from daslang!\n")
}
