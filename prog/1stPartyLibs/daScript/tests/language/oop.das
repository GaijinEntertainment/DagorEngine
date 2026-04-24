// Tests OOP: classes, constructors, finalizers, inheritance, virtual dispatch, RTTI
options gen2
require dastest/testing_boost public
require daslib/rtti

var delFoo = 0
var delFoo3D = 0

class Foo {
    x, y : int = 0
    [export]
    def Foo {
        Foo`set(self, 1, 1)
    }
    def Foo(T : int) {
        self->set(T, T)
    }
    def Foo(X, Y : int) {
        Foo`set(self, X, Y)
    }
    def set(X, Y : int) {
        x = X
        y = Y
    }
    def ups() {
        x = -1
        y = -1
    }
    def operator delete {
        delFoo ++
    }
}

class Foo3D : Foo {
    z : int = 13
    def Foo3D {
        assert(x == 0 && y == 0)
        assert(z == 13)
        Foo`Foo(self)
        assert(x == 1 && y == 1)
        z = 3
    }
    def override set(X, Y : int) {
        Foo`set(self, X, Y)
        z = 0
    }
    def override ups() {
        x = -2
        y = -2
        z = -2
    }
    def operator delete {
        delete cast<Foo> self
        delFoo3D ++
    }
}

class Goo {
    a : int
}

def test_foo(tt : T?) {
    unsafe {
        var f = Foo()
        tt |> equal(f.x, 1)
        tt |> equal(f.y, 1)
        f->set(1, 2)
        tt |> equal(f.x, 1)
        tt |> equal(f.y, 2)
        f = Foo(11, 12)
        tt |> equal(f.x, 11)
        tt |> equal(f.y, 12)
        f = Foo(13)
        tt |> equal(f.x, 13)
        tt |> equal(f.y, 13)
        var f3d = Foo3D()
        tt |> equal(f3d.x, 1)
        tt |> equal(f3d.y, 1)
        tt |> equal(f3d.z, 3)
        f3d->set(1, 2)
        tt |> equal(f3d.x, 1)
        tt |> equal(f3d.y, 2)
        tt |> equal(f3d.z, 0)
        f3d.z = 13
        f3d->set(3, 4)
        tt |> equal(f3d.x, 3)
        tt |> equal(f3d.y, 4)
        tt |> equal(f3d.z, 0)
        tt |> equal(delFoo, 0)
        tt |> equal(delFoo3D, 0)
        delete f
        tt |> equal(delFoo, 1)
        tt |> equal(delFoo3D, 0)
        delete f3d
        tt |> equal(delFoo, 2)
        tt |> equal(delFoo3D, 1)
        delFoo = 0
        delFoo3D = 0
    }
}

def test_foo_ptr(tt : T?) {
    var f = new Foo()
    f->ups()
    tt |> equal(f.x, -1)
    tt |> equal(f.y, -1)
    tt |> equal(delFoo, 0)
    tt |> equal(delFoo3D, 0)
    unsafe {
        delete f
    }
    tt |> equal(delFoo, 1)
    tt |> equal(delFoo3D, 0)
    f = cast<Foo?> new Foo3D()
    f->ups()
    tt |> equal(f.x, -2)
    tt |> equal(f.y, -2)
    unsafe {
        delete f
    }
    tt |> equal(delFoo, 2)
    tt |> equal(delFoo3D, 1)
    delFoo = 0
    delFoo3D = 0
}

def test_rtti(tt : T?) {
    unsafe {
        var g = Goo()
        var f = Foo()
        var f2 = Foo()
        var f3d = Foo3D()
        var f3d2 = Foo3D()
        tt |> success(f.__rtti != g.__rtti)
        tt |> success(f3d.__rtti != g.__rtti)
        tt |> equal(f.__rtti, f2.__rtti)
        tt |> equal(f3d.__rtti, f3d2.__rtti)
        tt |> success(f.__rtti != f3d.__rtti)
        tt |> success(class_info(f) != class_info(g))
        tt |> success(class_info(f3d) != class_info(g))
        tt |> equal(class_info(f), class_info(f2))
        tt |> equal(class_info(f3d), class_info(f3d2))
        tt |> success(class_info(f) != class_info(f3d))
        tt |> success(is_compatible_cast(class_info(f), class_info(f)))
        tt |> success(is_compatible_cast(class_info(f), class_info(f3d)))
        tt |> success(!is_compatible_cast(class_info(f3d), class_info(f)))
        tt |> success(!is_compatible_cast(class_info(g), class_info(f)))
        tt |> success(!is_compatible_cast(class_info(f), class_info(g)))
        tt |> success(!is_compatible_cast(class_info(g), class_info(f3d)))
        tt |> success(!is_compatible_cast(class_info(f3d), class_info(g)))
        tt |> equal(class_info(f).name, "Foo")
        tt |> equal(class_info(f3d).name, "Foo3D")
        tt |> equal(class_info(g).name, "Goo")
    }
}

[test]
def test_oop(t : T?) {
    t |> run("class constructors and methods") @(t : T?) { test_foo(t); }
    t |> run("virtual dispatch via pointer") @(t : T?) { test_foo_ptr(t); }
    t |> run("RTTI class_info and cast") @(t : T?) { test_rtti(t); }
}
