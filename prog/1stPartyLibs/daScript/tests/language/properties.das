options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Custom property operator: `.res :=` dispatches to a setter
struct Foo {
    resz : string
}

def operator . res :=(var f : Foo; st : string#) {
    f.resz := st
}

// Class with getter/setter property pair
class sealed UIFrame {
    _size : float2
    def const operator . sizezz() : float2 {
        return _size
    }
    def operator . sizezz :=(value : float2) {
        _size = value
    }
}

[test]
def test_property_clone_assign(t : T?) {
    t |> run("clone-assign string via block") @(t : T?) {
        var res : string
        unsafe {
            res := reinterpret<string#> "foo"
        }
        t |> equal(res, "foo")
    }
    t |> run("clone-assign to struct field directly") @(t : T?) {
        var f : Foo
        unsafe {
            f.resz := reinterpret<string#> "bar"
        }
        t |> equal(f.resz, "bar")
    }
    t |> run("clone-assign via property operator") @(t : T?) {
        var f : Foo
        unsafe {
            f.res := reinterpret<string#> "baz"
        }
        t |> equal(f.resz, "baz")
    }
}

[test]
def test_class_property_operators(t : T?) {
    t |> run("setter assigns value") @(t : T?) {
        var frame = new UIFrame()
        frame.sizezz := float2(100.0, 200.0)
        t |> equal(frame.sizezz, float2(100.0, 200.0))
        unsafe { delete frame; }
    }
    t |> run("getter returns current value") @(t : T?) {
        var frame = new UIFrame()
        frame.sizezz := float2(3.0, 4.0)
        let sz = frame.sizezz
        t |> equal(sz.x, 3.0)
        t |> equal(sz.y, 4.0)
        unsafe { delete frame; }
    }
    t |> run("multiple assignments overwrite") @(t : T?) {
        var frame = new UIFrame()
        frame.sizezz := float2(1.0, 1.0)
        frame.sizezz := float2(5.0, 6.0)
        t |> equal(frame.sizezz, float2(5.0, 6.0))
        unsafe { delete frame; }
    }
}
