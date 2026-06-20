// Parent has only def A(int) -- no 0-arg-callable ctor. Derived class with no
// explicit ctor cannot synthesize a chain that calls A`A(self). Hard error.
options gen2
expect 30322, 30318, 30341    // 30322 from the rule; 30318/30341 cascade from `new B()` without an initializer

class A {
    a : int = 0
    def A(val : int) {
        a = val
    }
}

class B : A {
    b : int = 0
}

[export]
def main {
    let b = new B()
    feint("b.a={b.a}, b.b={b.b}\n")
}
