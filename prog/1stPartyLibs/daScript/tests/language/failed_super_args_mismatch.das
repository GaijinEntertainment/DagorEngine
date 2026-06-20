// super(args) cannot skip an intermediate whose ctor takes different arguments.
options gen2
expect 30245

class A {
    a : int = 0
    def A {
        a = 1
    }
}

class B : A {
    b : int = 0
    def B(val : int) {
        super()
        b = val
    }
}

class C : B {
    c : int = 0
    def C {
        // 30245: super() can't match B(int), but B has a user ctor — silently
        // walking up to A's 0-arg ctor would skip B's invariants
        super()
        c = 3
    }
}

[export]
def main {
    let c = new C()
    feint("c.a={c.a}, c.b={c.b}, c.c={c.c}\n")
}
