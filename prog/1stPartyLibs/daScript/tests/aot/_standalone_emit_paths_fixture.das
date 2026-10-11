options gen2

// safe navigation on a this-module struct, and a literal whose `??>` is a trigraph to a C
// compiler

struct Node {
    next : Node?
}

[export]
def has_next(n : Node?) : bool {
    return (n?.next) != null
}

[export]
def puzzled() : string {
    return "what??>"
}
