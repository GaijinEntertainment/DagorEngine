options gen2
// options log_infer_passes=true

require UnitTest
require dastest/testing_boost public

struct S1 {
    a : int64 = 1l
    b : int64 = 2l
    c : int = 3
}

struct S2 : S1 {
    d : int = 4
}

[cpp_layout(pod=false)]
struct CppS1 {
    vtable : void?
    b : int64 = 2l
    c : int = 3
}

[cpp_layout(pod=false)]
struct CppS2 : CppS1 {
    d : int = 4
}

[test]
def test_cpp_layout(t : T?) {
    t |> equal(typeinfo sizeof(type<S1>), 24)
    t |> equal(typeinfo sizeof(type<S2>), 24)
    t |> equal(typeinfo offsetof < d > (type<S2>), 20)
    t |> equal(typeinfo sizeof(type<CppS1>), CppS1Size())
    t |> equal(typeinfo sizeof(type<CppS2>), CppS2Size())
    t |> equal(typeinfo offsetof < d > (type<CppS2>), CppS2DOffset())
}
