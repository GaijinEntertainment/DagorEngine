options gen2
// options log_nodes=true

require UnitTest
require dastest/testing_boost public

[checkRange]
struct Foo {
    @min = 0@max = 3 a : int = 1
    @min = 10@max = 20 b : int = 15
}

struct TestObjectFooNative {
    fooData : int
}

struct TestObjectBarNative {
    fooPtr : TestObjectFooNative?
    barData : float
}

[sideeffects]
def test_native(t : T?) {
    var a : TestObjectFooNative?
    var b : TestObjectBarNative?
    var idummy : int
    var fdummy : float
    a?.fooData ?? idummy = 1
    t |> equal(idummy, 1)
    a = new TestObjectFooNative
    a?.fooData ?? idummy = 2
    t |> success(a.fooData == 2 && idummy == 1)
    b?.barData ?? fdummy = 1.0
    t |> equal(fdummy, 1.0)
    b = new TestObjectBarNative
    b?.barData ?? fdummy = 2.0
    t |> success(b.barData == 2.0 && fdummy == 1.0)
    b?.fooPtr?.fooData ?? idummy = 3
    t |> equal(idummy, 3)
    b.fooPtr = a
    b?.fooPtr?.fooData ?? idummy = 4
    t |> success(b.fooPtr.fooData == 4 && idummy == 3)
}

[sideeffects]
def test_handled(t : T?) {
    var a : TestObjectFoo?
    var b : TestObjectBar?
    var idummy : int
    var fdummy : float
    a?.fooData ?? idummy = 1
    t |> equal(idummy, 1)
    a = new TestObjectFoo
    a?.fooData ?? idummy = 2
    t |> success(a.fooData == 2 && idummy == 1)
    b?.barData ?? fdummy = 1.0
    t |> equal(fdummy, 1.0)
    b = new TestObjectBar
    b?.barData ?? fdummy = 2.0
    t |> success(b.barData == 2.0 && fdummy == 1.0)
    b?.fooPtr?.fooData ?? idummy = 3
    t |> equal(idummy, 3)
    b.fooPtr = a
    b?.fooPtr?.fooData ?? idummy = 4
    t |> success(b.fooPtr.fooData == 4 && idummy == 3)
    a.fooData = 7
    t |> equal(a.propAdd13, 20)
    t |> equal(b.getFoo.fooData, 7)
    t |> equal(deref(b.getFooPtr).fooData, 7)
    t |> success(deref(a) == deref(b.fooPtr))
    t |> success(!(deref(a) != deref(b.fooPtr)))
    // call member
    hit_me(*a, 1, 2)
    t |> equal(a.fooData, 3)
    unsafe {
        delete a
        delete b
    }
}

[export]
def test_fancy_container(var a : array<FancyClass>) {
    feint("a = {a}\n")
}

def test_fancy(t : T?) {
    var fc0 = FancyClass()
    t |> equal(fc0.value, 13)
    var fc = FancyClass(1, 2)
    t |> equal(fc.value, 3)
    using() <| $(var f : FancyClass) {
        t |> equal(f.value, 13)
    }
    using(1, 3) <| $(var f : FancyClass) {
        t |> equal(f.value, 4)
    }
    // otherwise we leak hidden std::string inside fancy class
    deleteFancyClass(fc0)
    deleteFancyClass(fc)
}

def test_nc_alias(t : T?) {
    var lookingAt = new TestObjectFoo
    var lookPos = lookingAt.lookAt ?? float3()
    t |> equal(lookPos, float3())
    unsafe {
        delete lookingAt
    }
}

[test]
def test_handle(t : T?) {
    test_native(t)
    test_handled(t)
    test_fancy(t)
    test_nc_alias(t)
}
