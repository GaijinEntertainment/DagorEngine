class B {}

class A extends B {}

class C (B) { }

class D (class {}) {}

class E (class extends class {} {}) {}

class F (class extends class (class {}) {} {}) {}

class G extends class { a = 1 } { a = 2 } { println(1) } { println(2) }