// C : B : A where A has user ctor and B is empty. C's user ctor must still
// call super() — otherwise A's invariants are silently skipped. The lint
// walks ancestors so the empty intermediate B doesn't hide the requirement.
options gen2
expect 30308

class A {
    a : int = 0
    def A {
        a = 1
    }
}

class B : A {
}

class C : B {
    c : int = 0
    def C {
        // No super() — A's ctor never runs through the chain.
        c = 3
    }
}

[export]
def main {
    let c = new C()
    feint("c.a={c.a}, c.c={c.c}\n")
}
