options gen2
//! the daslang CLI runs a program on the persistent heap; this fixture reads the linear one, whose objects are bounded to the byte
options persistent_heap = false

struct Node {
    x : int
}

//! reads the int after a heap `Node`, past the bytes the heap handed out
[export, sideeffects, hint(noinline)]
def read_past_node(n : Node?; k : int) : int {
    var p = unsafe(reinterpret<int?>(n))
    return unsafe(p[k])
}

[export]
def main() {
    var n = new Node(x = 7)
    //! the command line keeps the index opaque to the optimizer
    let k = length(get_command_line_arguments()) > 1000 ? 2 : 1
    print("READ_PAST {read_past_node(n, k)}\n")
}
