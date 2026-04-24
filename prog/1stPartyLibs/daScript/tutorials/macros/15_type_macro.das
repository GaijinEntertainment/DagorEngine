options gen2
options no_aot

// Tutorial 15 — Type Macro (AstTypeMacro)
//
// This tutorial demonstrates how to create and use a custom type macro.
// The `padded` macro defined in type_macro_mod.das resolves the type
// expression `padded(type<T>, N)` into `T[N]`.
//
// Two usage patterns are shown:
//   1. Concrete — fully specified types (`padded(type<float>, 4)`)
//   2. Generic  — auto-deduced types (`padded(type<auto(TT)>, 4)`)

require type_macro_mod

// --- Concrete usage ---
// The compiler sees `padded(type<float>, 4)` and calls PaddedTypeMacro.visit().
// The macro returns float[4], so `data` is a fixed-size array of 4 floats.

def test_concrete() {
    var data : padded(type<float>, 4)
    data[0] = 1.0
    data[1] = 2.0
    data[2] = 3.0
    data[3] = 4.0
    print("concrete: data = {data}\n")
}

// --- Generic usage ---
// When the type parameter uses `auto(TT)` the compiler resolves TT from
// the call site.  The macro returns auto(TT)[4] so the compiler can match
// and deduce the element type.

def sum_padded(arr : padded(type<auto(TT)>, 4)) : TT {
    var total : TT
    for (i in range(4)) {
        total += arr[i]
    }
    return total
}

[export]
def main() {
    test_concrete()

    // sum_padded deduces TT = float from the argument type.
    var floats : padded(type<float>, 4)
    floats[0] = 10.0
    floats[1] = 20.0
    floats[2] = 30.0
    floats[3] = 40.0
    let float_sum = sum_padded(floats)
    print("generic: float sum = {float_sum}\n")

    // sum_padded deduces TT = int from the argument type.
    var ints : padded(type<int>, 4)
    ints[0] = 1
    ints[1] = 2
    ints[2] = 3
    ints[3] = 4
    let int_sum = sum_padded(ints)
    print("generic: int sum = {int_sum}\n")
}

// Expected output:
//   concrete: data = [[ 1; 2; 3; 4]]
//   generic: float sum = 100
//   generic: int sum = 10
