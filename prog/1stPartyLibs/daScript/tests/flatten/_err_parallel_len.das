options gen2
options indenting = 4

require daslib/flatten

[flatten]
def parallel_mismatch(x : int) : int {
    var acc = 0
    for (a, b in [1, 2, 3], [10, 20]) {
        acc += x + a * b
    }
    return acc
}

[export]
def main {
    print("{parallel_mismatch(3)}\n")
}
