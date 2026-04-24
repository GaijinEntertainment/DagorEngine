options gen2
require dastest/testing_boost

require UnitTest

[sideeffects]
def test_using(t : T?) {
    using() $(var f : FancyClass) {
        t |> equal(13, f.value)
    }
    using(1, 3) $(var f : FancyClass) {
        t |> equal(4, f.value)
    }
}

[sideeffects]
def test_placement_new(t : T?) {
    var fc0 = FancyClass()
    t |> equal(13, fc0.value)
    var fc = FancyClass(1, 2)
    t |> equal(3, fc.value)
}

class Foo {
    a : int
    b : int
    def Foo {
        a = 13
    }
}

[sideeffects]
def test_ascend_ctor(t : T?) {
    var foo = new Foo(b = 42)
    t |> equal(13, foo.a)
    t |> equal(42, foo.b)
}

[test]
def test_ref(t : T?) {
    t |> run("using") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_using))
        test_using(t)
    }
    t |> run("placement new") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_placement_new))
        test_placement_new(t)
    }
    t |> run("ascend constructor") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_ascend_ctor))
        test_ascend_ctor(t)
    }
}
