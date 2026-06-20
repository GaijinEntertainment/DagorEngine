// Walking past an empty intermediate: A has only def A(int) (no default),
// B is empty, C inherits from B. C cannot synth a chain that reaches A's
// 0-arg ctor (there isn't one). Error must name A, not B.
options gen2
expect 30322:2, 30318, 30341    // 30322 fires once for B and once for C — both name A as the offending ancestor

class A {
    a : int = 0
    def A(val : int) {
        a = val
    }
}

class B : A {
}

class C : B {
    c : int = 0
}

[export]
def main {
    let c = new C()
    feint("c.a={c.a}, c.c={c.c}\n")
}
