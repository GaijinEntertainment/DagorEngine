options gen2
expect 50503

// Explicitly-typed block params so daslang's pre-macro block typing
// succeeds; the macro then reaches its single-table-only rule.

require daslib/with_boost

struct A {
    f1 : int
}

[export]
def main {
    var t1 : table<string; A>
    var t2 : table<string; A>
    with_(t1["k"], t2["q"]) $(var a : A; var b : A) {  // refuse: max one table-keyed arg per call
        print("{a.f1} {b.f1}")
    }
}
