options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct iVec2 {
    x, y : int
}

def iVec2(X, Y : int) {
    return iVec2(x = X, y = Y)
}

def operator -(a : iVec2) : iVec2 {
    return iVec2(x = -a.x, y = -a.y)
}

def operator +(a, b : iVec2) : iVec2 {
    return iVec2(x = a.x + b.x, y = a.y + b.y)
}

def operator ==(a, b : iVec2) : bool {
    return (a.x == b.x) && (a.y == b.y)
}

[test]
def test_operator_overload(t : T?) {
    t |> run("negate operator") @(t : T?) {
        let a = iVec2(4, 5)
        t |> success(-a == iVec2(-4, -5))
    }
    t |> run("add operator") @(t : T?) {
        let a = iVec2(4, 5)
        let b = iVec2(6, 7)
        t |> success(a + b == iVec2(10, 12))
    }
}
