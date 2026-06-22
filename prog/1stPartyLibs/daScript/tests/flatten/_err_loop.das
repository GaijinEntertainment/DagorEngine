options gen2
options indenting = 4

require daslib/flatten

[flatten]
def loopy(n : int) : int {
    var s = 0
    for (i in range(n)) {
        s = s + i
    }
    return s
}

[export]
def main {
    print("{loopy(3)}\n")
}
