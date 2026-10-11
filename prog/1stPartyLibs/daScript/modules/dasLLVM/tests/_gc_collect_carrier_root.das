options gen2
options gc
options persistent_heap

struct Node {
    x : int
}

def do_collect {
    unsafe(heap_collect(true, false))
}

//! `n` lives in this frame across the collect, then its slot is the first a churn would reuse
def worker : int {
    var n = new Node(x = 7)
    do_collect()
    let churn <- [for (i in range(64)); new Node(x = -i - 1)]
    return n.x
}

[export]
def main {
    print("WORKER {worker()}\n")
}
