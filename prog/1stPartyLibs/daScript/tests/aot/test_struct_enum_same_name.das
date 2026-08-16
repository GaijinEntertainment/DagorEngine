options gen2
require dastest/testing_boost public

// Coverage: AOT codegen when a struct and an enum share a name (#3065, fuzzer).
// daslang keeps structs and enums in separate symbol tables, so this compiles; AOT
// used to emit `struct var1` and `enum class var1` into the same namespace -- a hard
// C++ redefinition. The emitter now disambiguates by prepending `_S`/`_E` (`_Svar1` / `_Evar1`).
//
// The struct cannot be referenced by its (now ambiguous) bare name, but it is still
// emitted as a this-module type, and that emission is what forces the collision in the
// generated C++. The test below exercises the enum at runtime (its access sites take
// the renamed `_Evar1::...` form); the regression itself is the generated C++ compiling.

struct var1 {
    a : int
}

enum var1 {
    x
    y
}

[test]
def test_struct_enum_same_name(t : T?) {
    t |> run("aot enum shadowed by same-named struct") @(t : T?) {
        let e = var1.x
        t |> equal(e == var1.x, true)
        t |> equal(e == var1.y, false)
        t |> equal(int(var1.y), 1)
    }
}
