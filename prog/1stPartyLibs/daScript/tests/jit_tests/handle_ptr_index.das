options gen2
require dastest/testing_boost

require UnitTest

//! A keyed handle (UnitTest::PtrSlots) indexes by string and yields `int?`, null for a missing key.
//! The jit's at`handle hook returns that pointer as the value of the expression: loading through it
//! hands back the pointee instead, and dereferences null on a miss. Every other annotation in tree
//! yields a ref from makeIndexType, so this is the only test that reaches the pointer arm.

def check_slots(t : T?; var slots : PtrSlots) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (t : T?; var slots : PtrSlots) : void > check_slots))
    let pa = slots["a"]
    let pb = slots["b"]
    t |> success(pa != null)
    t |> success(pb != null)
    t |> equal(7, *pa)
    t |> equal(9, *pb)
    // a missing key is a null pointer, and evaluating the expression must not read through it
    t |> success(slots["zz"] == null)
}

[test]
def test_handle_ptr_index(t : T?) {
    testPtrSlots() $(var slots : PtrSlots) {
        check_slots(t, slots)
    }
}
