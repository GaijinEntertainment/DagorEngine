// options log_nodes=true

require UnitTest

[checkRange]
struct Foo
    [[min=0,max=3]] a:int = 1
    [[min=10,max=20]] b:int = 15

struct TestObjectFooNative
    fooData : int

struct TestObjectBarNative
    fooPtr:TestObjectFooNative?
    barData:float

[sideeffects]
def test_native
    var a:TestObjectFooNative?
    var b:TestObjectBarNative?
    var idummy:int
    var fdummy:float
    a?.fooData ?? idummy = 1
    assert(idummy==1)
    a = new TestObjectFooNative
    a?.fooData ?? idummy = 2
    assert(a.fooData==2 && idummy==1)
    b?.barData ?? fdummy = 1.0
    assert(fdummy==1.0)
    b = new TestObjectBarNative
    b?.barData ?? fdummy = 2.0
    assert(b.barData==2.0 && fdummy==1.0)
    b?.fooPtr?.fooData ?? idummy = 3
    assert(idummy==3)
    b.fooPtr = a
    b?.fooPtr?.fooData ?? idummy = 4
    assert(b.fooPtr.fooData==4 && idummy==3)

[sideeffects]
def test_handled
    var a:TestObjectFoo?
    var b:TestObjectBar?
    var idummy:int
    var fdummy:float
    a?.fooData ?? idummy = 1
    assert(idummy==1)
    a = new TestObjectFoo
    a?.fooData ?? idummy = 2
    assert(a.fooData==2 && idummy==1)
    b?.barData ?? fdummy = 1.0
    assert(fdummy==1.0)
    b = new TestObjectBar
    b?.barData ?? fdummy = 2.0
    assert(b.barData==2.0 && fdummy==1.0)
    b?.fooPtr?.fooData ?? idummy = 3
    assert(idummy==3)
    b.fooPtr = a
    b?.fooPtr?.fooData ?? idummy = 4
    assert(b.fooPtr.fooData==4 && idummy==3)
    a.fooData = 7
    assert(a.propAdd13==20)
    assert(b.getFoo.fooData==7)
    assert(deref(b.getFooPtr).fooData==7)
    assert(deref(a)==deref(b.fooPtr))
    assert(!(deref(a)!=deref(b.fooPtr)))
    // call member
    hit_me(*a,1,2)
    assert(a.fooData==3)
    unsafe
        delete a
        delete b

[export]
def test_fancy_container(var a : array<FancyClass>)
    print("a = {a}\n")

def test_fancy
    var fc0 = FancyClass()
    assert(fc0.value==13)
    var fc = FancyClass(1,2)
    assert(fc.value==3)
    using() <| $( var f : FancyClass )
        assert(f.value==13)
    using(1,3) <| $( var f : FancyClass )
        assert(f.value==4)
    // otherwise we leak hidden std::string inside fancy class
    deleteFancyClass(fc0)
    deleteFancyClass(fc)

def test_nc_alias
    var lookingAt = new TestObjectFoo
    var lookPos = lookingAt.lookAt ?? float3()
    assert(lookPos==float3())
    unsafe
        delete lookingAt

[export]
def test:bool
    test_native()
    test_handled()
    test_fancy()
    test_nc_alias()
    return true
