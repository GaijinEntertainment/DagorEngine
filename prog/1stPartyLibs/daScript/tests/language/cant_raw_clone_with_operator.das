// the built-in clone of a non-copyable struct is the generated field-wise clone; an operator := on the pair
// replaces it, so the raw form has nothing to fall back to and is a compile error, not a silent recursion
options gen2
expect 30913:1

struct Bag {
    items : array<int>
}

def operator :=(var dst : Bag&; src : Bag) {
    dst !:= src
}

[export]
def main {
    var a : Bag
    var b : Bag
    a := b
}
