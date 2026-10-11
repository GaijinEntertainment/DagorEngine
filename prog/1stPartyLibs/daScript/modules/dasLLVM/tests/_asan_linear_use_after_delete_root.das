options gen2
//! the daslang CLI runs a program on the persistent heap; this fixture reads the linear one, whose objects are bounded to the byte
options persistent_heap = false

struct Node {
    x : int
}

[export, sideeffects, hint(noinline)]
def read_through(p : int?) : int {
    return *p
}

[export]
def main() {
    var n = new Node(x = 7)
    let p = unsafe(reinterpret<int?>(n))
    unsafe {
        delete n
    }
    print("READ_AFTER_DELETE {read_through(p)}\n")
}
