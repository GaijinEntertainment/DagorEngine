options gen2
options indenting = 4

require daslib/flatten

// max_unroll=8 lowers the cap below this 3x3 = 9 nest, proving the annotation arg is
// honored (the 4096 default would accept 9 fine). Tiny so the test stays fast.
[flatten(max_unroll=8)]
def small(x : int) : int {
    var acc = x
    for (a in range(3)) {
        for (b in range(3)) {
            acc += a + b
        }
    }
    return acc
}

[export]
def main {
    print("{small(1)}\n")
}
