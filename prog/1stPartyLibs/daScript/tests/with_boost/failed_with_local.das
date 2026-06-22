options gen2
expect 50503

require daslib/with_boost

struct A {
    f1 : int
}

[export]
def main {
    var local = A(f1 = 1)
    with_(local) {  // refuse: not ExprAt
        print("body")
    }
}
