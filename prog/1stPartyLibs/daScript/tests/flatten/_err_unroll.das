options gen2
options indenting = 4

require daslib/flatten

// Nested constant loops unroll multiplicatively (20^3 = 8000 > the 4096 default cap):
// the twin must bail with a clean max_unroll error instead of expanding to a
// multi-gigabyte body and crashing the macro.
[flatten]
def blowup(x : int) : int {
    var acc = x
    for (a in range(20)) {
        for (b in range(20)) {
            for (c in range(20)) {
                acc += a + b + c
            }
        }
    }
    return acc
}

[export]
def main {
    print("{blowup(1)}\n")
}
