options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[safe_when_uninitialized]
struct NwiBall {
    center : float3 = float3(7, 8, 9)
    radius : float = 1.234f
}

def NwiBall(center : float3; radius : float) : NwiBall {
    var b : NwiBall
    b.center = center
    b.radius = radius
    return b
}

[test]
def test_new_with_init(t : T?) {
    t |> run("new without parens — zeroed") @(t : T?) {
        var b0 = new NwiBall
        t |> equal(b0.center, float3())
        t |> equal(b0.radius, 0.0f)
        unsafe { delete b0; }
    }
    t |> run("new with parens — defaults") @(t : T?) {
        var b1 = new NwiBall()
        t |> equal(b1.center, float3(7, 8, 9))
        t |> equal(b1.radius, 1.234f)
        unsafe { delete b1; }
    }
    t |> run("new with custom constructor") @(t : T?) {
        var b2 = new NwiBall(float3(1, 2, 3), 4.0f)
        t |> equal(b2.center, float3(1, 2, 3))
        t |> equal(b2.radius, 4.0f)
        unsafe { delete b2; }
    }
    t |> run("new array with constructor") @(t : T?) {
        var b3 = new <NwiBall[3]>(float3(3, 2, 1), 0.5f)
        for (b in b3) {
            t |> equal(b.center, float3(3, 2, 1))
            t |> equal(b.radius, 0.5f)
        }
        unsafe { delete b3; }
    }
}
