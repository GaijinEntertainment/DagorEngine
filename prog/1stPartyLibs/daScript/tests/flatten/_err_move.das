options gen2
options indenting = 4

require daslib/flatten

// Move (`<-`) cannot be predicated — only copyable-value assignment is
// supported. `<-` survives inference as an ExprMove node (even for ints),
// so this trips the move/clone boundary cleanly.
[flatten]
def withmove(x : int) : int {
    var a = x
    var b = 0
    b <- a
    return b
}

[export]
def main {
    print("{withmove(3)}\n")
}
