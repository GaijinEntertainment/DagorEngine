class B {}

class A(B) {}

class D(class {}) {}

class E(class(class {}) {}) {}

class F(class(class(class{}) {}) {}) {}

class G(class { a = 1 }) { a = 2 } { println(1) } { println(2) }
