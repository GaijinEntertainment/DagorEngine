options gen2
expect 50503

// Explicitly-typed block param so pre-macro typing succeeds; the macro
// then catches the arity mismatch.

require daslib/with_boost

struct A {
    f1 : int
}

[export]
def main {
    var a <- [A(f1=1)]
    var b <- [A(f1=2)]
    with_(a[0], b[0]) $(var x : A) {  // refuse: 2 containers but only 1 block param
        print("{x.f1}")
    }
    delete a
    delete b
}
