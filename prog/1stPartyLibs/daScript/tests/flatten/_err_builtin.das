options gen2
options indenting = 4

require daslib/flatten

[flatten]
def withprint(x : int) : int {
    print("x={x}\n")
    return x
}

[export]
def main {
    print("{withprint(3)}\n")
}
