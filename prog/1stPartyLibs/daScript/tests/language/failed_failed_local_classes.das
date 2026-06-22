options gen2
expect 31030
options no_local_class_members

class A {
    a : int = 12
}

class B {
    a : A  // OK?
    arr : array<A> // Ok?
}
