// options log_infer_passes=true

require UnitTest

struct S1
    a : int64 = 1l
    b : int64 = 2l
    c : int = 3

struct S2 : S1
    d : int = 4

[cpp_layout (pod=false)]
struct CppS1
    vtable : void?
    b : int64 = 2l
    c : int = 3

[cpp_layout (pod=false)]
struct CppS2 : CppS1
    d : int = 4

[export]
def test()
    assert(typeinfo(sizeof type<S1>)==24)
    assert(typeinfo(sizeof type<S2>)==24)
    assert(typeinfo(offsetof<d> type<S2>)==20)
    verify(typeinfo(sizeof type<CppS1>)==CppS1Size())
    verify(typeinfo(sizeof type<CppS2>)==CppS2Size())
    verify(typeinfo(offsetof<d> type<CppS2>)==CppS2DOffset())
    return true
