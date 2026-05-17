options gen2
require dastest/testing_boost public
require fio

//! Regression: iterating a vector-like container of a value-annotation handle
//! (ManagedValueAnnotation<T>, e.g. `clock` / `das::Time`) used to emit a value
//! variable declaration in AOT C++ but pointer-dereference usage, breaking the
//! AOT build. The AOT emitter must keep the `*` suffix when substitute_ref is
//! requested, even for value handles.

def sum_clocks(clocks : array<clock>) : int64 {
    var total = 0l
    for (c in clocks) {
        total += int64(c)
    }
    return total
}

def sum_clocks_const(clocks : array<clock> const) : int64 {
    var total = 0l
    for (c in clocks) {
        total += int64(c)
    }
    return total
}

def first_clock(clocks : array<clock>) : clock {
    for (c in clocks) {
        return c
    }
    return mktime(1970, 1, 1, 0, 0, 0)
}

[test]
def test_iter_value_handle(t : T?) {
    t |> run("for over array<clock>") @(t : T?) {
        var clocks : array<clock>
        clocks |> push(mktime(2024, 1, 1, 0, 0, 0))
        clocks |> push(mktime(2024, 1, 2, 0, 0, 0))
        clocks |> push(mktime(2024, 1, 3, 0, 0, 0))
        let expected = int64(clocks[0]) + int64(clocks[1]) + int64(clocks[2])
        t |> equal(sum_clocks(clocks), expected)
    }
    t |> run("for over array<clock> const") @(t : T?) {
        var clocks : array<clock>
        clocks |> push(mktime(2024, 1, 1, 0, 0, 0))
        clocks |> push(mktime(2024, 1, 2, 0, 0, 0))
        let expected = int64(clocks[0]) + int64(clocks[1])
        t |> equal(sum_clocks_const(clocks), expected)
    }
    t |> run("return element from iteration") @(t : T?) {
        var clocks : array<clock>
        clocks |> push(mktime(2024, 1, 1, 0, 0, 0))
        clocks |> push(mktime(2024, 1, 2, 0, 0, 0))
        t |> equal(int64(first_clock(clocks)), int64(clocks[0]))
    }
}
