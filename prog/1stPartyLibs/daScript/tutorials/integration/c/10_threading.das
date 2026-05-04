options gen2

// Companion script for C integration tutorial 10 — Threading.
// Returns the sum 0 + 1 + ... + 99 = 4950.

[export]
def compute() : int {
    var total = 0
    for (i in range(100)) {
        total += i
    }
    return total
}
