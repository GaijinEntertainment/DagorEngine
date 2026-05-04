options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct MethodPoint {
    x, y = 0.0f
    magnitude = @@method_point_magnitude
    dot = @@method_point_dot
}

def MethodPoint(X, Y) {
    return MethodPoint(x = X, y = Y)
}

def method_point_magnitude(that : MethodPoint) {
    return that.x * that.x + that.y * that.y
}

def method_point_dot(that, p : MethodPoint) {
    return that.x * p.x + that.y * p.y
}

[test]
def test_method_semantic(t : T?) {
    t |> run("function pointer as method") @(t : T?) {
        let p = MethodPoint(2., 3.)
        let mag = p->magnitude()
        let d = p->dot(MethodPoint(1., 1.))
        t |> equal(mag, 13.)
        t |> equal(d, 5.)
    }
}
