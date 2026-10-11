// hand-written (NOT generated): user-defined operators involving lattice types must
// resolve to the user function on every tier — under JIT they route to the function
// path, never the native lattice op lowering (which only knows the builtin signatures)
options gen2
require dastest/testing_boost public

def operator ==(a : short4; b : int4) : bool {
    return int4(a) == b
}

def operator +(a : half4; b : float) : half4 {
    return a + half4(float16(b))
}

def use_eq(a : short4; b : int4) : bool {
    return a == b
}

def use_add(a : half4; b : float) : half4 {
    return a + b
}

[test]
def test_lattice_user_operators(t : T?) {
    t |> run("mixed equality routes to the user operator") @(t : T?) {
        t |> success(use_eq(short4(1, 2, 3, 4), int4(1, 2, 3, 4)), "equal")
        t |> success(!use_eq(short4(1, 2, 3, 4), int4(1, 2, 3, 5)), "not equal")
    }
    t |> run("mixed add routes to the user operator") @(t : T?) {
        let r = use_add(half4(1h, 2h, 3h, 4h), 0.5)
        t |> success(r == half4(1.5h, 2.5h, 3.5h, 4.5h), "sum")
    }
}
