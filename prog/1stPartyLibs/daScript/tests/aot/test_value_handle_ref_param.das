options gen2
require dastest/testing_boost public

//! Regression: a writable reference parameter of a value-annotation handle
//! (ManagedValueAnnotation<T>, e.g. ``clock`` / ``das::Time``, or ECS
//! ``EntityId``) must emit ``T &`` in AOT C++, not ``T`` by value. The
//! over-broad ``valueHandle`` exclusion in ``describeCppTypeEx`` skipped the
//! ``&`` suffix for every ``tHandle`` with ``isRefType == false``, which
//! silently turned RW lambda/function parameters into by-value copies --
//! mutations done by AOT code were lost on the C++ caller side.

def set_clock_to_2030(var c : clock&) {
    c = mktime(2030, 1, 1, 0, 0, 0)
}

def swap_clocks(var a : clock&; var b : clock&) {
    let tmp = a
    a = b
    b = tmp
}

[test]
def test_value_handle_ref_param(t : T?) {
    t |> run("writable ref to value-handle preserves mutation") @(t : T?) {
        var c = mktime(2024, 1, 1, 0, 0, 0)
        set_clock_to_2030(c)
        t |> equal(int64(c), int64(mktime(2030, 1, 1, 0, 0, 0)))
    }
    t |> run("two writable refs swap independently") @(t : T?) {
        var a = mktime(2024, 1, 1, 0, 0, 0)
        var b = mktime(2030, 6, 15, 0, 0, 0)
        let a0 = int64(a)
        let b0 = int64(b)
        swap_clocks(a, b)
        t |> equal(int64(a), b0)
        t |> equal(int64(b), a0)
    }
}
