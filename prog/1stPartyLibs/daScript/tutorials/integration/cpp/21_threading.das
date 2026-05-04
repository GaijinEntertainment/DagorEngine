options gen2

// Companion script for C++ integration tutorial 21 — Threading.
// Both threading demos (run-on-thread and compile-on-thread) call
// the compute() function and expect the result 4950.

[export]
def compute() : int {
    var total = 0
    for (i in range(100)) {
        total += i
    }
    return total
}
