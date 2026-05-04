// Helper script for Tutorial 44.
// This file is compiled and executed by the host program.
options gen2

var result : int = 0

[export]
def add(a, b : int) : int {
    return a + b
}

[export]
def compute(x : int) {
    result = x * x + 1
}

[export]
def store_via_ptr(val : int; var dst : int?) {
    unsafe {
        *dst = val * 10
    }
}

[export]
def main() {
    print("hello from 44_helper!\n")
}
