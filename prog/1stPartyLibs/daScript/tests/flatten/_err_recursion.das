options gen2
options indenting = 4

require daslib/flatten

[flatten]
def recurse(n : int) : int {
    if (n <= 0) {
        return 0
    }
    return n + recurse(n - 1)
}

[export]
def main {
    print("{recurse(3)}\n")
}
