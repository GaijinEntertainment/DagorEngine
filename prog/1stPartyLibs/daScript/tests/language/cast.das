options gen2
require dastest/testing_boost public

// Tests cast<> for struct inheritance and reinterpret<> for bitwise struct reinterpretation.

struct Foo {
    x, y : int
}

struct Foo3D : Foo {
    z : int
}

struct FooVision {
    f3d : Foo3D
    color : float4
}

def setFooGeneric(var foo; x, y) {
    foo.x = x
    foo.y = y
}

def setFoo(var foo : Foo; x, y : int) {
    foo.x = x
    foo.y = y
}

def setFooPtr(var foo : Foo?; x, y : int) {
    foo.x = x
    foo.y = y
}

[test]
def test_cast(t : T?) {
    t |> run("cast value struct") @(t : T?) {
        var f : Foo
        var f3d : Foo3D
        t |> equal(f.x, 0)
        t |> equal(f.y, 0)
        t |> equal(f3d.x, 0)
        t |> equal(f3d.y, 0)
        setFooGeneric(f, 1, 2)
        setFooGeneric(f3d, 1, 2)
        t |> equal(f.x, 1)
        t |> equal(f.y, 2)
        t |> equal(f3d.x, 1)
        t |> equal(f3d.y, 2)
        setFoo(f, 3, 4)
        setFoo(cast<Foo> f3d, 3, 4)
        t |> equal(f.x, 3)
        t |> equal(f.y, 4)
        t |> equal(f3d.x, 3)
        t |> equal(f3d.y, 4)
        (cast<Foo> f3d).y = 5
        t |> equal(f3d.y, 5)
        var fv : FooVision
        (cast<Foo> fv.f3d).y = 13
        t |> equal((cast<Foo> fv.f3d).y, 13)
    }
    t |> run("cast pointer struct") @(t : T?) {
        var f = new Foo
        var f3d = new Foo3D
        t |> equal(f.x, 0)
        t |> equal(f.y, 0)
        t |> equal(f3d.x, 0)
        t |> equal(f3d.y, 0)
        setFooGeneric(f, 1, 2)
        setFooGeneric(f3d, 1, 2)
        t |> equal(f.x, 1)
        t |> equal(f.y, 2)
        t |> equal(f3d.x, 1)
        t |> equal(f3d.y, 2)
        setFooPtr(f, 3, 4)
        setFooPtr(cast<Foo?> f3d, 3, 4)
        t |> equal(f.x, 3)
        t |> equal(f.y, 4)
        t |> equal(f3d.x, 3)
        t |> equal(f3d.y, 4)
        (cast<Foo?> f3d).y = 5
        t |> equal(f3d.y, 5)
        var fv = new FooVision
        (cast<Foo> fv.f3d).y = 13
        t |> equal((cast<Foo> fv.f3d).y, 13)
        unsafe {
            delete f
            delete f3d
            delete fv
        }
    }
    t |> run("reinterpret struct") @(t : T?) {
        let f2i = Int2Float(iv = uint(0x3f400000), fv = 1.0)
        unsafe {
            let i2f = reinterpret<Float2Int>(f2i)
            t |> equal(i2f.fv, 0.75)
            t |> equal(i2f.iv, uint(0x3f800000))
        }
    }
}

struct Int2Float {
    iv : uint
    fv : float
}

struct Float2Int {
    fv : float
    iv : uint
}
