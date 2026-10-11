options gen2

struct Node {
    x : int
}

[export]
def main() {
    var n = new Node(x = 7)
    //! the command line keeps the index opaque; the read and the allocation share one function, where the object size is known
    let k = length(get_command_line_arguments()) > 1000 ? 0 : 1
    let p = unsafe(reinterpret<int?>(n))
    print("READ_PAST {unsafe(p[k])}\n")
}
