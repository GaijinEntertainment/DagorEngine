options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct OvFoo {
    x, y : int = 0
    set = @@OvFoo_setXY
    set2 = @@OvFoo_setXY
}

def OvFoo_setXY(var that : OvFoo; x, y : int) {
    that.x = x
    that.y = y
}

struct OvFoo3D : OvFoo {
    z : int = 3
    override set = cast<auto> @@OvFoo3D_setXY
    override set2 = @@OvFoo3D_setXY2
}

def OvFoo3D_setXY(var that : OvFoo3D; x, y : int) {
    that.x = x
    that.y = y
    that.z = 0
}

def OvFoo3D_setXY2(var _that : OvFoo; x, y : int) {
    unsafe {
        var that : OvFoo3D& = upcast<auto> _that
        that.x = x
        that.y = y
        that.z = 0
    }
}

[test]
def test_override(t : T?) {
    t |> run("base struct function pointer") @(t : T?) {
        var f = OvFoo()
        f->set(1, 2)
        t |> equal(f.x, 1)
        t |> equal(f.y, 2)
    }
    t |> run("derived struct override with cast") @(t : T?) {
        var f3d = OvFoo3D()
        t |> equal(f3d.x, 0)
        t |> equal(f3d.y, 0)
        t |> equal(f3d.z, 3)
        f3d->set(1, 2)
        t |> equal(f3d.x, 1)
        t |> equal(f3d.y, 2)
        t |> equal(f3d.z, 0)
    }
    t |> run("derived struct override with upcast") @(t : T?) {
        var f3d = OvFoo3D()
        f3d.z = 13
        f3d->set2(3, 4)
        t |> equal(f3d.x, 3)
        t |> equal(f3d.y, 4)
        t |> equal(f3d.z, 0)
    }
}
