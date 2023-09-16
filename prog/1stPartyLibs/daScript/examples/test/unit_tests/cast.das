// options log = true, log_nodes = true, optimize=true, print_ref = true, print_var_access = true, log_optimization_passes = true

struct Foo
    x, y : int

struct Foo3D : Foo
    z : int

struct FooVision
    f3d : Foo3D
    color : float4

def setFooGeneric(var foo;x,y)
    foo.x = x
    foo.y = y

def setFoo(var foo:Foo;x,y:int)
    foo.x = x
    foo.y = y

def setFooPtr(var foo:Foo?;x,y:int)
    foo.x = x
    foo.y = y

[sideeffects]
def test_struct
    var f : Foo
    var f3d : Foo3D
    assert(f.x==0 && f.y==0)
    assert(f3d.x==0 && f3d.y==0)
    setFooGeneric(f,1,2)
    setFooGeneric(f3d,1,2)
    assert(f.x==1 && f.y==2)
    assert(f3d.x==1 && f3d.y==2)
    setFoo(f,3,4)
    setFoo(cast<Foo> f3d,3,4)
    assert(f.x==3 && f.y==4)
    assert(f3d.x==3 && f3d.y==4)
    (cast<Foo> f3d).y = 5
    assert(f3d.y==5)
    var fv : FooVision
    (cast<Foo> fv.f3d).y = 13
    assert((cast<Foo> fv.f3d).y==13)
    return true

[sideeffects]
def test_ptr
    var f = new Foo
    var f3d = new Foo3D
    assert(f.x==0 && f.y==0)
    assert(f3d.x==0 && f3d.y==0)
    setFooGeneric(f,1,2)
    setFooGeneric(f3d,1,2)
    assert(f.x==1 && f.y==2)
    assert(f3d.x==1 && f3d.y==2)
    setFooPtr(f,3,4)
    setFooPtr(cast<Foo?> f3d,3,4)
    assert(f.x==3 && f.y==4)
    assert(f3d.x==3 && f3d.y==4)
    (cast<Foo?> f3d).y = 5
    assert(f3d.y==5)
    var fv = new FooVision
    (cast<Foo> fv.f3d).y = 13
    assert((cast<Foo> fv.f3d).y==13)

struct Int2Float
    iv:uint
    fv:float

struct Float2Int
    fv:float
    iv:uint

[sideeffects]
def test_reinterpret()
    let f2i = [[Int2Float iv=0x3f400000,fv=1.0]]
    unsafe
        let i2f = reinterpret<Float2Int>(f2i)
        assert(i2f.fv==0.75 && i2f.iv==0x3f800000)
    return true

[export]
def test
    test_struct()
    test_ptr()
    test_reinterpret()
    return true

