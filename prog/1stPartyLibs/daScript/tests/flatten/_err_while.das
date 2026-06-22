options gen2
options indenting = 4

require daslib/flatten

// While loops have no compile-time iteration count — not unrollable.
[flatten]
def whiley(n : int) : int {
    var s = 0
    var i = 0
    while (i < n) {
        s = s + i
        i = i + 1
    }
    return s
}

[export]
def main {
    print("{whiley(3)}\n")
}
