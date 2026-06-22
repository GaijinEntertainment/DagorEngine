// super.X() cannot skip an intermediate class whose own ctor would be bypassed.
options gen2
expect 30245, 30808, 30838    // 30245 from the rule; 30808/30838 cascade as super stays unresolved

class A {
    a : int = 0
    def A {
        a = 1
    }
}

class B : A {
    b : int = 0
    def B {
        super()
        b = 2
    }
}

class C : B {
    c : int = 0
    def C {
        // 30245: super.A() would silently skip B's invariants — rejected
        super.A()
        c = 3
    }
}

[export]
def main {
    let c = new C()
    feint("c.a={c.a}, c.b={c.b}, c.c={c.c}\n")
}
